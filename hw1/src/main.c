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
    /* File
    printf("argc: %d\n", argc);

    char** words = argv;
    for(int i = 0; i < argc; i++){
        char* word = *(words++);
        while(*word != 0)
            printf("%c,", *(word++));
        printf("\n");
    }
    */
        

    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    // TO BE IMPLEMENTED

    FILE * filePtr;
    filePtr = fopen(diff_filename, "r"); // handle file close
    HUNK hunk;
   
    printf("hunk_next result: %d\n", hunk_next(&hunk, filePtr));
    printf("hunk_next result: %d\n", hunk_next(&hunk, filePtr));
    printf("hunk_next result: %d\n", hunk_next(&hunk, filePtr));

    

    /*
     * File testing
     */
    printf("\n\n\n------------------DUMPING THE REST");
    FILE * testPtr = filePtr;
    char ch1;
    do{
        ch1 = fgetc(testPtr);
        if(ch1 == '\n')
            printf("NEWLINE");
        printf("%c,", ch1);

    }while(ch1 >= 0);
    
    printf("\nNEXT VERSION\n");

    FILE * testPtr2 = filePtr;
    char ch2;
    do{
        ch2 = fgetc(testPtr2);
        if(ch2 == '\n')
            printf("NEWLINE");
        printf("%c,", ch2);

    }while(ch2 >= 0);

    fclose(filePtr);
    return EXIT_FAILURE; 
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
