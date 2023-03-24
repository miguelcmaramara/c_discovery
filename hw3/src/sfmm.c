/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    sf_footer * footerPtr = offsetPtr(headerPtr, offset - 8);
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

sf_block* peerBlock(sf_block* in, int forward){
    if(forward)
        return offsetPtr(in, getLength(in));

    long flags = getFlags(in);
    if((flags & PREV_BLOCK_ALLOCATED) == 0){
        sf_header *footer = offsetPtr(in, -8);
        return offsetPtr(in, -1 * getHeaderLen(footer));

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
    long newFlags = maskLong(getFlags(in), 0x010, 0);
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
    in->body.links.prev->body.links.next = in->body.links.next;
    in->body.links.next->body.links.prev = in->body.links.prev;
    in->body.links.next = 0;
    in->body.links.prev = 0;
    return in;
}

/*
 * splits the block into two parts, the first part of specified size
 */
sf_block* splitBlock(sf_block* in, long size){
    if(getLength(in) - size < 32) return NULL;  // first portion cannot be < 32
    if(size < 32) return NULL;  // second portion cannot be < 32
    if((getFlags(in) & THIS_BLOCK_ALLOCATED) != 0) return NULL; // must be free
    if((size & 0b111) != 0) return NULL; //must be 8-aligned

    removeFromFreeList(in);
    sf_block *res = offsetPtr(in, size);
    long prevSize = getLength(in);
    // create first portion header & footer
    createHeaderAndFooter(&(in->header), size, getFlags(in));
    // create second portion header & footer
    createHeaderAndFooter(&(res->header), prevSize - size, 0b000);
    addToFreeList(in);
    addToFreeList(res);
    return res;
}


void* initHeap(){
    // request more memory
    sf_mem_grow();

    
    // create initial pointers
    sf_block* prologue = sf_mem_start();
    sf_block* epilogue = offsetPtr(sf_mem_end(), -8);
    sf_block* freeBlock = offsetPtr(prologue, 32);


    // create blocks for data
    updateBlock(prologue, 32, 0b001);
    updateBlock(epilogue, 8, 0b001);
    updateBlock(freeBlock, (long)epilogue- (long)freeBlock, 0b010);
    
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
    // initialize heap
    if(sf_mem_start() == sf_mem_end()){
        sf_block* freeBlock =
            initHeap();
        
        sf_show_heap();
        sf_block* res = splitBlock(freeBlock, 4056/2 - ((4056/2)%8));
        
        sf_show_heap();
        updateBlockFlags(res, 0, 1);
        // create prologue
    }
    sf_show_heap();
    
    // TO BE IMPLEMENTED
    abort();
}

void sf_free(void *pp) {
    // TO BE IMPLEMENTED
    abort();
}

void *sf_realloc(void *pp, size_t rsize) {
    // TO BE IMPLEMENTED
    abort();
}

void *sf_memalign(size_t size, size_t align) {
    // TO BE IMPLEMENTED
    abort();
}
