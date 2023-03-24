#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    // sf_show_heap();
    // double* ptr = sf_malloc(sizeof(double));
    // *ptr = 320320320e-320;

    // printf("result ptr: %p\n", ptr);
    // printf("headerLocation ptr: %p\n", (void*)((long) ptr - 8));
    // printf("%.10e\n", *ptr);
    // printf("%.10e\n", 320320320e-320);

    // sf_show_heap();
    // sf_free(ptr);
    // sf_show_heap();


    double *ptrs[10];
    for( int i = 0; i < 10; i++){
        ptrs[i] = sf_malloc(sizeof(double));
        *ptrs[i] = 320320320e-320;
    }

    // for( int i = 0; i < 6; i++){
        // sf_free(ptrs[i]);
        // // ptrs[i] = sf_malloc(sizeof(double));
        // // *ptrs[i] = 320320320e-320;
    // }

    sf_free(ptrs[1]);
    sf_free(ptrs[2]);
    sf_free(ptrs[4]);
    sf_free(ptrs[5]);
    sf_free(ptrs[6]);
    sf_free(ptrs[8]);
    // sf_free(ptrs[7]);
    // sf_free(ptrs[6]);

    sf_show_heap();
    return EXIT_SUCCESS;
}
