#include <stdio.h>
#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);

   

    FILE * diff;
    diff = fopen(diff_filename, "r");

    int res = patch(stdin, stdout, diff);
    if(res == 0){
        fclose(diff);
        return EXIT_SUCCESS;
    } else {
        if(diff) fclose(diff);
        return EXIT_FAILURE;
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
