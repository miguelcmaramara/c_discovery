#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    double* ptr = sf_malloc(sizeof(double));

    *ptr = 320320320e-320;

    printf("result ptr: %p\n", ptr);
    printf("headerLocation ptr: %p\n", (void*)((long) ptr - 8));
    printf("%.10e\n", *ptr);
    printf("%.10e\n", 320320320e-320);
    sf_show_heap();

    sf_free(ptr);

    return EXIT_SUCCESS;
}
