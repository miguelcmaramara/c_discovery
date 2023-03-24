/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "debug.h"
#include "sfmm.h"


long maskLong(long in, long mask, int invert){
    if(invert)
        return in & ~mask;
    return in & mask;
}

void* maskPtr(void* in, long mask, int invert){
    if(invert)
        return (void*)((long)in & ~mask);
    return (void*)((long)in & mask);
}

long offsetLong(long in, long offset){
    return in + offset;
}
void * offsetPtr(void * in, long offset){
    return (void*) offsetLong((long) in, offset);
}

/*
 * checks to see that flags are in valid range
 * checks to see that header ptr is in valid range,
 *
 */
sf_header * createHeader(sf_header* in, long length, long flags){
    if(flags > 8 || flags < 0) return NULL;
    if(length < 0) return NULL;
    // check for divisible by 8
    if(((long)in & 0b111) != 0) return NULL;

    *in = length | flags;
    return in;
}

sf_footer* createFooter(sf_header *headerPtr){
    long offset = maskLong(*headerPtr, 0b111, 1);
    sf_footer * footerPtr = offsetPtr(headerPtr, offset - sizeof(size_t));
    *footerPtr = *headerPtr;
    return footerPtr;
}

void* createHeaderAndFooter(sf_header* in, long length, long flags){
    sf_header* header = createHeader(in, length, flags);
    if(header == NULL)
        return NULL;
    createFooter(header);
    return header;
}

long getHeaderFlags(sf_header *in){
    return maskLong(*in, 0b111, 0);
}
long getFlags(sf_block *in){
    return getHeaderFlags(&(in->header));
}

long getHeaderLen(sf_header *in){
    return maskLong(*in, 0b111, 1);
}
long getLength(sf_block *in){
    return getHeaderLen(&(in->header));
}

int isAlloc(sf_block *in){
    return (getFlags(in) & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED;
}

int prevAlloc(sf_block *in){
    return (getFlags(in) & PREV_BLOCK_ALLOCATED) == PREV_BLOCK_ALLOCATED;
}

int inQuickLst(sf_block *in){
    return (getFlags(in) & IN_QUICK_LIST) == IN_QUICK_LIST;
}

sf_block* peerBlock(sf_block* in, int forward){
    if(forward)
        return offsetPtr(in, getLength(in));

    long flags = getFlags(in);
    if((flags & PREV_BLOCK_ALLOCATED) == 0){
        sf_header *footer = offsetPtr(in, -1 * sizeof(size_t));
        if((getHeaderFlags(footer) & THIS_BLOCK_ALLOCATED) == THIS_BLOCK_ALLOCATED)
            return NULL;    // check for if block is allocated

        sf_header *header = offsetPtr(in, -1 * getHeaderLen(footer));
        if((long)header < (long)sf_mem_start()) return NULL;    //lower bound
        if((long)header >= (long)sf_mem_end()) return NULL;     //lower bound
        if(*header != *footer) return NULL; //header must match footer
        return (sf_block*) header;

    } else
        // NOT IMPLEMENTED
        return NULL;
}

sf_header *updateHeader(sf_header *in, long length, long flags, long targetPos){
    if((long)in >= (long)sf_mem_end())
        return NULL;
            
    if(flags >= 0 && targetPos >= 0) //update flags
        *in = maskLong(*in, targetPos, 1) | flags;
    if(length >= 0) //update length
        *in = maskLong(*in, 0b111, 0) | length;
    // update footer
    if((*in & THIS_BLOCK_ALLOCATED) == 0){
        createFooter(in);
    }
    return in;
}

sf_block *updateBlock(sf_block* in, long length, long flags){
    if((flags & THIS_BLOCK_ALLOCATED) == 1){
        createHeader(&(in->header), length, flags);
        updateHeader(&(peerBlock(in, 1)->header), -1, 0b010, 0b010);
    }else{
        createHeaderAndFooter(&(in->header), length, flags);
        updateHeader(&(peerBlock(in, 1)->header), -1, 0b000, 0b010);
    }
    return in;
}

sf_block *updateBlockFlags(sf_block* in, int inQuickLst, int thisBlockAllocated){
    long newFlags = maskLong(getFlags(in), 0b010, 0);
    if(inQuickLst)
        newFlags |= IN_QUICK_LIST;
    if(thisBlockAllocated)
        newFlags |= THIS_BLOCK_ALLOCATED;
    updateBlock(in, getLength(in), newFlags);
    return in;
}
int fitsInFreeList(long size, int i){
    if( i < 0) return 0;
    if(i == 0 && size == 32) return 2;

    long min = (2 << (i-1)) * 32;
    long max = (2 << (i)) * 32;
    if(size <= max){
        if(size > min)
            return 2;
        return 1;
    }
    return 0;

}

sf_block *addToFreeList(sf_block *in){
    sf_block *head = NULL;
    for (int i = 0; i < NUM_FREE_LISTS; i++){
        if(fitsInFreeList(getLength(in), i) == 2)
            head = &sf_free_list_heads[i];
    }
    if(head == NULL){
        return NULL;
    }
    in->body.links.next = head->body.links.next;
    in->body.links.prev = head;
    head->body.links.next->body.links.prev = in;
    head->body.links.next = in;
    return in;
}

sf_block* removeFromFreeList(sf_block *in){
    if(in->body.links.prev != NULL)
        in->body.links.prev->body.links.next = in->body.links.next;
    if(in->body.links.next != NULL)
        in->body.links.next->body.links.prev = in->body.links.prev;
    in->body.links.next = NULL;
    in->body.links.prev = NULL;
    return in;
}

/* 
 * checks if forward block can be coalesced and backward block can be coalesced
 * and coalescese them together.
 *
 * coalesce also handles any free lists
 * does not add to freeList if 
 */
sf_block *coalesce(sf_block *in){
    sf_block * next = peerBlock(in, 1);

    // should never hit this
    if(next >= (sf_block *)sf_mem_end())
        return in;
    removeFromFreeList(in);

    // coalesce forward
    while(!isAlloc(next)){
        removeFromFreeList(next);
        createHeaderAndFooter(&(in->header), getLength(in) + getLength(next), getFlags(in));
        next = peerBlock(in, 1);
    }

    // coalesce backward
    while(!prevAlloc(in)){
        sf_block *prev = peerBlock(in, 0);
        createHeaderAndFooter(&(prev->header), getLength(prev) + getLength(in), getFlags(prev));
        in = prev;
        removeFromFreeList(in);
    }
    addToFreeList(in);
    return in;
}

/*
 * returns 1 if the size fits into the quicklist, 0 if not
 */
int getQuickListNum(long size){
    // if(size < 32) return 0;
    // if(size > 32 + (NUM_QUICK_LISTS - 1) * 8) return 0;
    long i = (size - 32) / 8;
    if(((size - 32) & 0b111)!= 0) return -1; // must be factor of 8
    if(i < 0 || i > NUM_QUICK_LISTS - 1) return -1;
    if(i != (int)i) return -1; // must fit in size of int
    return (int)i;
}

/*
 * Just adds a block to a quick list. Does change block flags;
 */
sf_block *addToQuickList(sf_block *in){
    // find correct quicklist
    int i = getQuickListNum(getLength(in));

    if( i < 0 || i >= NUM_QUICK_LISTS)
        return NULL;

    // TODO: CHECK FOR COALESCE
    if(sf_quick_lists[i].length > QUICK_LIST_MAX){
        sf_block *curr = sf_quick_lists[i].first;
        while(curr != NULL){
            updateBlock(curr, getLength(curr), 0b000);
            addToFreeList(coalesce(curr));
        }

        sf_quick_lists[i].length = 0;
        sf_quick_lists[i].first = NULL;
    }

    
    // add onto the end of the quicklist
    sf_quick_lists[i].length += 1;
    if(sf_quick_lists[i].first == NULL){
        sf_quick_lists[i].first = in;
    }else{
        sf_block* curr = sf_quick_lists[i].first;
        while(curr->body.links.next != NULL){
            curr = curr->body.links.next;
        }
        curr->body.links.next = in;
    }

    in->body.links.next = NULL;
    updateBlockFlags(in, 1, 1);



    return in;
}

/*
 * splits the block into two parts, the first part of specified size
 */
sf_block* splitBlock(sf_block* in, long size){
    if(getLength(in) - size < 32) return NULL;  // first portion cannot be < 32
    if(size < 32) return NULL;  // second portion cannot be < 32
    // if((getFlags(in) & THIS_BLOCK_ALLOCATED) != 0) return NULL; // must be free
    if((size & 0b111) != 0) return NULL; //must be 8-aligned

    sf_block *res = offsetPtr(in, size);
    long prevSize = getLength(in);
    if(isAlloc(in)){
        removeFromFreeList(in);
        // create first portion header & footer
        createHeaderAndFooter(&(in->header), size, getFlags(in));
        addToFreeList(in);
    } else {
        createHeader(&(in->header), size, getFlags(in));
    }
    // create second portion header & footer
    createHeaderAndFooter(&(res->header), prevSize - size, 0b000);
    addToFreeList(res);
    return res;
}

size_t roundUpSize(size_t size){
    if((long)size < 0){
        return -1;
    }
    
    if(maskLong((long) size, 0b111, 0) == 0) return size;
    return size + maskLong((long)~size, 0b111, 0) + 1;
}

/*
 * Makes heap larger, expands epilogue and coalesces the former epilogue
 */
sf_block *expandHeap(){
    sf_block *oldEp = offsetPtr(sf_mem_end(), -1 * sizeof(size_t));

    // make heap larger
    if(sf_mem_grow() == NULL)
        return NULL;
    sf_block *newEp = offsetPtr(sf_mem_end(), -1 * sizeof(size_t));

    //expand epilogue
    createHeaderAndFooter(
            &(oldEp->header),
            (long)newEp - (long)oldEp,
            maskLong(getFlags(oldEp), 0b010 , 0) | 0b000);

    // put headers on new epilogue
    updateBlock(newEp, sizeof(size_t), 0b001);

    sf_block *res = coalesce(oldEp);
    // coalesce epilogue
    return res;
    // return coalesce(oldEp);
}


void* initHeap(){
    // request more memory
    sf_mem_grow();

    
    // create initial pointers
    sf_block* prologue = sf_mem_start();
    sf_block* epilogue = offsetPtr(sf_mem_end(), -1 * sizeof(size_t));
    sf_block* freeBlock = offsetPtr(prologue, 32);


    // create blocks for data
    updateBlock(prologue, 32, 0b001);
    updateBlock(epilogue, sizeof(size_t), 0b001);
    updateBlock(freeBlock, (long)epilogue- (long)freeBlock, 0b010);

    // initialize quicklist
    for(int i = 0; i < NUM_QUICK_LISTS; i++){
        sf_quick_lists[i].length = 0;
        sf_quick_lists[i].first = NULL;
    }
    
    // initialize freelist
    for(int i = 0; i < NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
    // add large block to freelist
    addToFreeList(freeBlock);
    
    return freeBlock;
        
}

void *sf_malloc(size_t size) {
    if(size == 0)
        return NULL;

    // initialize heap
    if(sf_mem_start() == sf_mem_end()){
        sf_block* freeBlock =
        initHeap();
        
        // sf_show_heap();
        // sf_block* q2 = splitBlock(freeBlock, 4056/2 - ((4056/2)%8));
        // sf_show_heap();

        /*sf_block* q1 = */splitBlock(freeBlock, 56);
        // splitBlock(freeBlock, 32);
        // splitBlock(q2, 184);

        // sf_show_heap();
        removeFromFreeList(freeBlock);
        addToQuickList(freeBlock);
        // sf_show_heap();
        // addToQuickList(q1);
        // addToQuickList(q2);
    }

    size_t test = 85912;
    size = roundUpSize(test); // switch to size later on
    size_t blockSize =  // round up to minimum block size
        size + sizeof(size_t) < sizeof(size_t) * 2 + sizeof(sf_block*) * 2
        ? sizeof(size_t) * 2 + sizeof(sf_block*) * 2
        : size + sizeof(size_t);

    // Search quick lists
    int i = getQuickListNum(blockSize);
    if(i >= 0 && sf_quick_lists[i].length > 0){
        sf_block *res = sf_quick_lists[i].first;
        sf_quick_lists[i].first = res->body.links.next;

        updateBlockFlags(res, 0, 1);
        return offsetPtr(res, sizeof(size_t));
    }
    
    // Search free lists
    // search through all free lists
    for(int i = 0; i < NUM_FREE_LISTS; i++){
        if(fitsInFreeList(blockSize, i)){
            sf_block *curr = sf_free_list_heads[i].body.links.next;
            // search though every block in free list
            while(curr != &sf_free_list_heads[i]){
                if(getLength(curr) >= blockSize){
                    splitBlock(curr, blockSize); //attempt to trim block

                    removeFromFreeList(curr);
                    updateBlockFlags(curr, 0, 1);
                    return offsetPtr(curr, sizeof(size_t));
                }
                curr = curr->body.links.next;
            }
        }
    }

    
    // expandHeap heap until you find necesary size, or you can't anymore

    while(true){
        sf_block* newMem = expandHeap();
        if(newMem == NULL)
            break;
        if(getLength(newMem) >= blockSize){
            removeFromFreeList(newMem);
            updateBlockFlags(newMem, 0, 1);
            return offsetPtr(newMem, sizeof(size_t));
       }
    }
    
    // TO BE IMPLEMENTED
    // abort();
    sf_errno = ENOMEM;
    fprintf(stderr, "TOOOOO BIG\n");
    return NULL;
}

int validBlock(sf_block *in){
    if(in == NULL) return 0;                            // NULLPTR
    if(maskPtr(in, 0b111, 0) != 0) return 0;            // pointer not align
    if(getLength(in) < 32) return 0;                    // min size
    if(maskLong(getLength(in), 0b111, 0) !=0) return 0; // size algin
    if(in < (sf_block*)sf_mem_start()) return 0;        // low & up mem bound
    if((long)offsetPtr(in, getLength(in)) > (long)sf_mem_end()) return 0;
    if(!isAlloc(in)) return 0;                          // must be alloc
    if(inQuickLst(in)) return 0;                        // cant be in quicklst

    if(!prevAlloc(in) && peerBlock(in, 0) == NULL)      // verify prevAlloc
        return 0;
    return 1;
}

int validFreePtr(void* in){
    if(in == NULL) return 0;
    if((long)in <= (long)sf_mem_start()) return 0;      // low mem 
    if(maskPtr(in, 0b111, 0) != 0) return 0;            // pointer not align
    return validBlock(offsetPtr(in, -1*sizeof(sf_header*)));
}

void sf_free(void *pp) {
    if(!validFreePtr(pp))
        abort();
    sf_block* blockToFree = offsetPtr(pp, -8);
    if(addToQuickList(blockToFree) == NULL)
        addToFreeList(blockToFree);
}

void *sf_realloc(void *pp, size_t rsize) {
    if(!validFreePtr(pp)){
        sf_errno = EINVAL;
        return NULL;
    }

    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }

    sf_block* reAllocBlock = offsetPtr(pp, -8);
    if(getLength(reAllocBlock) <= (long) rsize){
        splitBlock(reAllocBlock, rsize);            // handles splintering
        return reAllocBlock;
    }

    // Malloc
    sf_block* resBlock = sf_malloc(rsize);
    if(resBlock == NULL) return resBlock;
    //copy
    memcpy(
            offsetPtr(resBlock, sizeof(sf_header)),
            offsetPtr(reAllocBlock, sizeof(sf_header)),
            getLength(reAllocBlock) - 8);
    updateBlock(resBlock, getLength(reAllocBlock), getFlags(reAllocBlock));
    //free
    sf_free(pp);

    return offsetPtr(resBlock, sizeof(sf_header));
}

void *sf_memalign(size_t size, size_t align) {
    // TO BE IMPLEMENTED
    abort();
}
