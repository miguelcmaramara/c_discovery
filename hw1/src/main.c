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

/*void printBuffers(){
//TODO: REMOVE

    printf("--------------------buffer\n");
    char * buffChar = hunk_additions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        printf("%d ",  *(buffChar++));

    printf("\n--------------------\n");
    buffChar = hunk_deletions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        printf("%d ",  *(buffChar++));
    printf("\n--------------------bufferfin\n");
}*/

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

   

    FILE * diff;
    diff = fopen(diff_filename, "r"); // handle file close

    patch(stdin, stdout, diff);


    /* working tests:
    FILE * filePtr;
    filePtr = fopen(diff_filename, "r"); // handle file close
    HUNK hunk;

    // first hunk:
    printf("hunk_next result: %d\n", hunk_next(&hunk, filePtr));
    char hgc_tester = 'z';
    printf("\n Line 1: ");
    while(hgc_tester > 0){
        printf("%c", hgc_tester = hunk_getc(&hunk, filePtr));
    }



    printf("\n Line 2: ");
    do{
        printf("%d", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);


    printf("\n Line 3: ");
    do{
        printf("%d", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);
    printf("\n Line 4: ");
    do{
        printf("%d", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);


    printf("hunk_show:\n");
    hunk_show(&hunk, stdout);


    // second hunk:
    printf("hunk_next result: %d\n", hunk_next(&hunk, filePtr));
    do{
        printf("%c", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);
    do{
        printf("%c", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);
    do{
        printf("%c", hgc_tester = hunk_getc(&hunk, filePtr));
    }while(hgc_tester > 0);

    printf("hunk_show:\n");
    hunk_show(&hunk, stdout);
   */
    
    /*
     * File testing
     */
    /*
    printf("\n------------------DUMPING THE REST");
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
    */



    /* read characters in buffer
    char * buffChar = hunk_additions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        printf("a%c",  *(buffChar++));

    buffChar = hunk_deletions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        printf("d%c",  *(buffChar++));
        */
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
