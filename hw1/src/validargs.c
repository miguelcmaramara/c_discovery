#include <stdlib.h>
#include <stdbool.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    if(argc == 1) // Case of no flags
        return 0;
    
    char** stringPtr = argv;
    char* charPtr = *stringPtr;
    global_options = 0;
    
    /*
     * HANDLE ARGUMENTS
     */

    bool lastFlag = true;
    for(int i = 1; i < argc; i++){
        stringPtr++; 
        charPtr = *stringPtr;
        
        if(false == lastFlag){ // if last argument was not a flag, bad ordering
            global_options = 0;
            return -1;
        }

        if(*charPtr == '-') // current option is a flag
            switch(*(++charPtr)){
                case 'h':
                    if(i == 1){
                        global_options |= 0x1;
                        return 0;
                    }
                    global_options = 0;
                    return -1;
                case 'n':
                    global_options |= 0x2;
                    break;
                case 'q':
                    global_options |= 0x4;
                    break;
                default:
                    global_options = 0;
                    return -1; // invalid flag
            }
        else {// current option is not a flag
            lastFlag = false;
            diff_filename = charPtr;
        }
    }

    if(true == lastFlag){ // Last argument should not be a flag
        global_options = 0;
        diff_filename = NULL;
        return -1;
    }
    return 0; // All options passed

    
    /* print all characters
    char** words = argv;
    for(int i = 0; i < argc; i++){
        char* word = *(words++);
        while(*word != 0)
            printf("%c,", *(word++));
        printf("\n");
    }
    */
}
