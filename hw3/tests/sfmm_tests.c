#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0x7))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

/*
 * Assert the total number of quick list blocks of a specified size.
 * If size == 0, then assert the total number of all quick list blocks.
 */
void assert_quick_list_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_QUICK_LISTS; i++) {
	sf_block *bp = sf_quick_lists[i].first;
	while(bp != NULL) {
	    if(size == 0 || size == (bp->header & ~0x7))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of quick list blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(4024, 1);
	assert_free_list_size(7, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header and the link pointers.
	void *x = sf_malloc(16336);
	cr_assert_not_null(x, "x is NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(86100);

	cr_assert_null(x, "x is not NULL!");
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(85976, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_quick, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 32, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(40, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(3952, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(3784, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 2);
	assert_free_block_count(520, 1);
	assert_free_block_count(3472, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 500, sz_y = 200, sz_z = 700;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 4);
	assert_free_block_count(208, 3);
	assert_free_block_count(1896, 1);
	assert_free_list_size(3, 3);
	assert_free_list_size(6, 1);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x7) == 88, "Realloc'ed block size not what was expected!");

	assert_quick_list_block_count(0, 1);
	assert_quick_list_block_count(32, 1);
	assert_free_block_count(0, 1);
	assert_free_block_count(3904, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)y - sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x7) == 88, "Realloc'ed block size not what was expected!");

	// There should be only one free block.
	assert_quick_list_block_count(0, 0);
	assert_free_block_count(0, 1);
	assert_free_block_count(3968, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)y - sizeof(sf_header));
	cr_assert(bp->header & THIS_BLOCK_ALLOCATED, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x7) == 32, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size ADJUSTED_BLOCK_SIZE(sz_x) - ADJUSTED_BLOCK_SIZE(sz_y)
	// to the freelist.  This block will go into the main freelist and be coalesced.
	// Note that we don't put split blocks into the quick lists because their sizes are not sizes
	// that were requested by the client, so they are not very likely to satisfy a new request.
	assert_quick_list_block_count(0, 0);	
	assert_free_block_count(0, 1);
	assert_free_block_count(4024, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

Test(sfmm_student_suite, student_test_1_memalign, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
    size_t sz_x = sizeof(double);
	void *x1 = sf_memalign(sz_x, 8);
	void *x2 = sf_memalign(sz_x, 16);
	void *x3 = sf_memalign(sz_x, 64);
	// void *x4 = sf_memalign(sz_x, 65);
	// void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(x1, "x1 is NULL!");
	cr_assert_not_null(x2, "x2 is NULL!");
	cr_assert_not_null(x3, "x3 is NULL!");
	// cr_assert_null(x4, "x3 is NOT NULL!");


    cr_assert(((long)x1 & 7) == 0 , "x1 is not aligned to 8!");
	sf_block *x1b = (sf_block *)((char *)x1 - sizeof(sf_header));
	cr_assert(x1b->header & THIS_BLOCK_ALLOCATED, "x2 Allocated bit is not set!");
	cr_assert((x1b->header & ~0x7) == 32, "Realloc'ed block size not what was expected!");

    cr_assert(((long)x2 & 15) == 0 , "x2 is not aligned to 16!");
	sf_block *x2b = (sf_block *)((char *)x2 - sizeof(sf_header));
	cr_assert(x2b->header & THIS_BLOCK_ALLOCATED, "x2 Allocated bit is not set!");
	cr_assert((x2b->header & ~0x7) >= 32, "x2 Size is too small");
	cr_assert((x2b->header & ~0x7) < 64, "x2 Size is too large");

    cr_assert(((long)x3 & 63) == 0 , "x3 is not aligned to 64!");
	sf_block *x3b = (sf_block *)((char *)x3 - sizeof(sf_header));
	cr_assert(x3b->header & THIS_BLOCK_ALLOCATED, "x3 Allocated bit is not set!");
	cr_assert((x3b->header & ~0x7) >= 32, "x3 Size is too small");
	cr_assert((x3b->header & ~0x7) < 64, "x3 Size is too large");

    assert_quick_list_block_count(0, 0);	
    cr_assert(sf_errno == 0, "sf_errno CHANGED");
}

Test(sfmm_student_suite, student_test_2_bad_memalign, .timeout = TEST_TIMEOUT) {
    size_t sz_x = sizeof(double);
    double *init = sf_malloc(sz_x);
    *init = 0;

    
	sf_errno = 0;
	void *x1 = sf_memalign(sz_x, -1);
    cr_assert(sf_errno == EINVAL, "x1: sf_errno is not EINVAL");
    cr_assert_null(x1, "x1 is NOT NULL!");

	sf_errno = 0;
	void *x2 = sf_memalign(sz_x, 7);
    cr_assert(sf_errno == EINVAL, "x2: sf_errno is not EINVAL");
    cr_assert_null(x2, "x2 is NOT NULL!");

	sf_errno = 0;
	void *x3 = sf_memalign(sz_x, 67);
    cr_assert(sf_errno == EINVAL, "x3: sf_errno is not EINVAL");
    cr_assert_null(x3, "x3 is NOT NULL!");


    assert_quick_list_block_count(0, 0);	
    assert_free_block_count(0, 1);  
}

Test(sfmm_student_suite, student_test_3_out_of_order, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
    double *ptrs[10];
    for( int i = 0; i < 10; i++){
        ptrs[i] = sf_malloc(sizeof(double));
        *ptrs[i] = 320320320e-320;
    }

    sf_free(ptrs[1]);
    sf_free(ptrs[2]);
    sf_free(ptrs[4]);
    sf_free(ptrs[5]);
    sf_free(ptrs[6]);
    sf_free(ptrs[8]);


	assert_quick_list_block_count(0, 1);	
	assert_free_block_count(0, 3);  
    assert_free_block_count(32 * 2, 1);
    assert_free_block_count(32 * 3, 1);
}

Test(sfmm_student_suite, student_test_4_bad_reAlloc, .timeout = TEST_TIMEOUT) {
    size_t sz1 = sizeof(double);
    size_t sz2 = sizeof(double);
    double *init = sf_malloc(sz1);
    *init = 0;

    
	sf_errno = 0;
	void *x1 =  sf_realloc(sf_mem_start(), sz2);
    cr_assert(sf_errno == EINVAL, "x1: sf_errno is not EINVAL");
    cr_assert_null(x1, "x1 is NOT NULL!");

	sf_errno = 0;
	void *x2 =  sf_realloc(sf_mem_end(), sz2);
    cr_assert(sf_errno == EINVAL, "x2: sf_errno is not EINVAL");
    cr_assert_null(x2, "x2 is NOT NULL!");

	sf_errno = 0;
	void *x3 = sf_realloc((void*)((long)init + 1), sz2);
    cr_assert(sf_errno == EINVAL, "x3: sf_errno is not EINVAL");
    cr_assert_null(x3, "x3 is NOT NULL!");


    assert_quick_list_block_count(0, 0);	
    assert_free_block_count(0, 1);  
}

Test(sfmm_student_suite, student_test_5_reAlloc_contents, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
    size_t sz1 = sizeof(double) * 20;
    size_t sz2 = sizeof(double) * 40;
    double *arr = sf_malloc(sz1);

    for(int i = 0; i < 20; i++)
        arr[i] = i;


    arr = sf_realloc(arr, sz2);
    

    for(int i = 0; i < 20; i++)
        cr_assert(arr[i] == i, "MEM DIFFERS AT INDEX %d", i);

    assert_quick_list_block_count(0, 1);	
    assert_free_block_count(0, 1);  
}
