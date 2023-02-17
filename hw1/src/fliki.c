
#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

#include <stdbool.h>

/*
 * State variables
 */
enum FLIKI_FXN_STATE {
    FXN_NEXT,
    FXN_GETC,
    FXN_SHOW,
    FXN_NONE
};
static int counter = 1; // current hunk number
static char lastChar; // last character read by the 
static int errStatus; // 0 if successful, can be EOF, EOS, ERR
static int remainingAdds = 0; // num lines to be added remaining
static int remainingDels = 0; // num lines to be deleted remaining
static enum FLIKI_FXN_STATE lastUsed = FXN_NONE;
static HUNK_TYPE addDelStatus = HUNK_NO_TYPE; // which buffer to add to
static int file_finished = 0; // 2 if finished, 0/1, is not finished
static int addsTrunc = 0; // num lines to be added remaining
static int delsTrunc = 0; // num lines to be deleted remaining

// buffer pointer variables 
static char * lastAddPtr = hunk_additions_buffer;
static char * lastAddCount = hunk_additions_buffer;
static char * lastDelPtr = hunk_deletions_buffer;
static char * lastDelCount = hunk_deletions_buffer;

/*
 * Adds char c to buffer specified by isAddition
 * returns char if successful, -1 if buffer is full
 */
static char addToBuffer(char c, bool isAddition){
    if(c < 0)
        return c;
    if(isAddition){
        // cannot add to ptr when within 2 of ending
        if( hunk_additions_buffer + HUNK_MAX - lastAddPtr <= 2){
            addsTrunc = 1;
            return -1;
        }
        // cannot add to counter within 4 of ending
        if( hunk_additions_buffer + HUNK_MAX - lastAddCount <= 4){
            addsTrunc = 1;
            return -1;
        }

        // handling newline found
        if(lastAddCount == lastAddPtr)
            lastAddPtr +=2;

        // increment value of beginning
        if(*lastAddCount == (char)0xFF)
            *(lastAddCount + 1) += 1;
        else
            *(lastAddCount) += 1;

        if(c == '\n'){
            *lastAddPtr = c;
            lastAddCount = ++lastAddPtr;
        }
        else 
            *(lastAddPtr++) = c;
        //printf("aadded: %c", *(lastAddPtr - 1));
        return c;
    } else {
        // cannot add to ptr when within 2 of ending
        if( hunk_deletions_buffer + HUNK_MAX - lastDelPtr <= 2){
            delsTrunc = 1;
            return -1;
        }
        // cannot add to counter within 4 of ending
        if( hunk_deletions_buffer + HUNK_MAX - lastDelCount <= 4){
            delsTrunc = 1;
            return -1;
        }
        // handling newline found
        if(lastDelCount == lastDelPtr)
            lastDelPtr +=2;

        // increment value of beginning
        if(*lastDelCount == (char)0xFF)
            *(lastDelCount + 1) += 1;
        else
            *(lastDelCount) += 1;

        if(c == '\n'){
            *lastDelPtr = c;
            lastDelCount = ++lastDelPtr;
        } else {
            *(lastDelPtr++) = c;
            //printf("|dadded: %c|", *(lastDelPtr - 1));
        }
        return c;
    }
}

/*
 * returns the output of fgetc, while logging it as the lastCharacter read
 * @param in Input stream that will be read
 */
static char fgetcLogged(FILE*in){
    char curr = fgetc(in);
    lastChar = curr;
    return curr;
}

/*
 * advanceUntil will iterate through stream until character in range (inclusive)
 * is found
 *
 * @param in Input stream that will be advanced until reached
 * @param begRange beginning range to advance until
 * @param endRange end of range to advance until
 * @return character stopped at if successful, -1 if EOF file reached
 */
static int advanceUntil(FILE *in, char begRange, char endRange){
    // TODO: test EOF
    //printf("Advancing: \n");
    char curr;
    do{
        curr = fgetcLogged(in);
        // printf("%c", curr);
    } while((curr < begRange || curr > endRange) && curr > 0);

    return curr;
}

/*
 * advanceUntil will iterate through stream until character in range (inclusive)
 * is found
 *
 * @param in Input stream that will be advanced until reached
 * @param chars string of chars of interest
 * @param shouldMatch 1 if stop at first char in chars found, 0 if stop
 *                    at first char not in chars found
 * @return character stopped at if successful, -1 if EOF file reached
 */
static char advanceUntilFound(FILE *in, char * chars, bool shouldMatch){
    // TODO: test EOF
    //printf("Advancing: \n");
    char curr;
    char *charPtr = chars;
    do{
        do{
            curr = fgetcLogged(in);
            if((curr == *charPtr) == shouldMatch)
                return curr;
        } while (*(charPtr++) != 0);
        charPtr = chars;

        // printf("%c", curr);
    } while(curr > 0);

    return curr;
}

/*
 * Function shifts over decimal 10 and ads probided char
 *
 * @param prev is the number to be decimal bit shifted
 * @return 0 if unsuccessful, the decimal shifted number when successful
 */
static int deciShiftLeft(int prev, char next){
    if(next < '0' || next > '9')
        return 0;
    prev *= 10;
    return prev + next - 48;
}

/*
 * Advances file and modifies integer provided until a non-number is provided.
 * Then returns the next non-digit character
 */
static char parseInt(FILE *in, int * intToChange){
    // TODO: test EOF
    char curr = fgetcLogged(in);

    //printf("Parsed Int- Before: %d", *intToChange);

    while(curr >= '0' &&  curr <= '9'){
        *intToChange = deciShiftLeft(*intToChange, curr);
        curr = fgetcLogged(in);
    }
    //printf(" After: %d", *intToChange);
    //printf(" Stopper: %c\n", curr);

    return curr;
}

/*
 * Assumes that header begins at char pointed at and modifies the HUNK
 * Allows for input of the first integer for peeking. input 0 if not utilized
 *
 * returns 1 if successful
 * returns 0 if unsuccessful by bad format
 * returns -1 if unsuccessful by EOF
 */
static int parseHeader(HUNK *hp, FILE *in, int firstInt){
    HUNK_TYPE type;
    int old_start = firstInt;
    int old_end = 0;
    int new_start = 0;
    int new_end = 0;
    char stopper;

    stopper = parseInt(in, &old_start);
    if(stopper == ',')
        stopper = parseInt(in, &old_end);
    else
        old_end = old_start;

    switch(stopper) {
        case 'a':
            type = HUNK_APPEND_TYPE;
            break;
        case 'c':
            type = HUNK_CHANGE_TYPE;
            break;
        case 'd':
            type = HUNK_DELETE_TYPE;
            break;
        case -1: // EOF case
            return -1;
        default: //bad format case
            return 0;
    }
    // Parse second set of numbers
    stopper = parseInt(in, &new_start);
    if(stopper == ',')
        stopper = parseInt(in, &new_end);
    else
        new_end = new_start;

    if(stopper <= 0) // make sure that stopper was new line
        return -1;
    if(stopper != '\n')
        return 0;
    
    hp->type = type;
    hp->old_start = old_start;
    hp->old_end = old_end;
    hp->new_start = new_start;
    hp->new_end = new_end;

    remainingDels = old_end - old_start + 1;
    remainingAdds = new_end - new_start + 1;
    switch(hp->type)
    {
        case HUNK_DELETE_TYPE:
            remainingAdds = 0;
            break;
        case HUNK_APPEND_TYPE:
            remainingDels = 0;
            break;
        case HUNK_CHANGE_TYPE:
            break;
        case HUNK_NO_TYPE:
            // this should not be hit ever
            return 0;
    }
    return 1;
}

/*
 * returns integer of character is a digit
 * else returns -1
 */
static int isDigit(char c){
    if(c >= '0' && c <= '9')
        return (int)c - 48;
    return -1;
}

/*
 * pass pointer to char of the first value and it will return string length
 * by reading the two
 */
static int parseCharNumber(char * first){
    int num = *(first + 1);
    num = num << 8;
    num = num | (int) *first;
    return num;
}

/*
 * prints all characters in string, curr should point to 
 * leader = '<' or '>'
 */
static char * outputLine(char leader, char * curr, FILE * out){
    int len = parseCharNumber(curr);
    curr += 2; // get to actual stirng portion
    fprintf(out, "%c ", leader);
    for(int i = 0; i < len; i++){
        fprintf(out, "%c", *(curr++));
    }
    return curr;
}

/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

int hunk_next(HUNK *hp, FILE *in) {
    // TODO: test end of file right after header
    // TODO: enforce what happens when this is the first function being used
    // TODO: implement @91
    // TODO: initialize hunk variables (buffers + file_finished)

    int headerParseStatus = 0;
    bool firstRun = true;
    char curr;
    lastUsed = FXN_NEXT;
    do{
        curr = fgetcLogged(in);
        // printf("curr: %c\n", curr);
        if(isDigit(curr) >= 0) // hunk header must start with digit
            headerParseStatus = parseHeader(hp, in, isDigit(curr));
        else if (curr != '<' && curr != '>' && curr != '-' && !firstRun){
            return errStatus = ERR; // malformed hunk
        }

        if(headerParseStatus == 0)
            if(advanceUntil(in, '\n', '\n') < 0){ // advances until next newline
                return errStatus = EOF; // return EOF if EOF is reached
            }

        firstRun = false;

    } while(headerParseStatus == 0);

    if(headerParseStatus == -1){
        return errStatus =EOF;
    }

    // TODO: check bogus lines numbers like 15,4d5
    // TODO: get rid of hunk
    hp->serial = counter++;

    // refresh buffers
    char * buffChar = hunk_additions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        *(buffChar++) = 0;

    buffChar = hunk_deletions_buffer;
    for(int i = 0; i < HUNK_MAX; i++)
        *(buffChar++) = 0;

    // initializing variables
    file_finished = 0;
    lastAddPtr = hunk_additions_buffer;
    lastAddCount = hunk_additions_buffer;
    lastDelPtr = hunk_deletions_buffer;
    lastDelCount = hunk_deletions_buffer;
    /*
    printf("Hunk Parsed!\n");
    printf("    serial: %d\n", hp->serial);
    printf("    old_start: %d\n", hp->old_start);
    printf("    old_end: %d\n", hp->old_end);
    printf("    new_start: %d\n", hp->new_start);
    printf("    new_end: %d\n", hp->new_end);
    */
    /*
    if(advanceUntil(in, '\n', '\n') < 0) // advances until next newline
        return ERR; // if EOF reached before newline, then malformed hunk
                    // */
    // TO BE IMPLEMENTED
    return errStatus = 0;
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

int hunk_getc(HUNK *hp, FILE *in) {
    // Test Cases:
    // 1. normal action
    // 2. empty file
    // 3. file pointer reached the end of the file
    if(lastUsed == FXN_GETC && errStatus == ERR)
        return errStatus = ERR; // further calls result in error
    if(lastUsed == FXN_NEXT && errStatus < 0)
        return errStatus = ERR; // requires next to be successful
    if(lastUsed == FXN_NONE) // next function has not been used at all
        return errStatus = ERR;
    if(lastChar < 0) // case EOF was reached before this
        return errStatus = ERR;
    if(file_finished == 2)
        return errStatus = ERR;
    lastUsed = FXN_GETC;
    // TODO check for finished EOS condition
    // TODO reoganize ho this file finds EOS
    /*
     * 3 cases:
     * 1. called right after hunk_getc
     * 2. not 1 && reads newLine
     * 2. 
     */
    char lastCharTemp = lastChar;
    char curr = fgetcLogged(in);

    // if newline, then handle it was if pointing right after newLine
    /* newline should be returned
    if(curr == '\n'){
        lastCharTemp = lastChar;
        curr = fgetcLogged(in);
    }
    */


    // Checking for malformed hunk in following ways:
    // 1. '< ', '> ', '---\n' formats must follow '\n'
    // 2. '< ', '> ', '---\n' must occur at app1opriate points by matching
    //                        current level of del/add
    // 3. '< ', '> ', '---\n' must occur in corrent type
    // 4. side effect: determines which stage in (add, delete, or change)
    if(lastCharTemp == '\n'){
        if(curr != '<' && curr != '>' && curr != '-' && (curr < '0' || curr > '9'))
            return errStatus = ERR; // malformed hunk \n should be followed by '<>-'

        if(curr == '-'){
            /*
            // All deletes should have been read.
            if(remainingDels != 0)
                return errStatus = ERR;
                */

            if(hp->type != HUNK_CHANGE_TYPE) // \n- only occur in change hunks
                return errStatus = ERR;

            // check for '---\n'
            if( fgetcLogged(in) != '-' ||
                fgetcLogged(in) != '-' ||
                fgetcLogged(in) != '\n')
                return errStatus = ERR;
            //return('?');
            file_finished = 1;
            return errStatus = EOS;
        }

        if(curr == '<'){
            /*
            // Deletes must remain
            if(remainingDels <= 0);
                return errStatus =ERR;
                */
            if(hp->type == HUNK_APPEND_TYPE)
                return errStatus = ERR;
            
            // checks for '< '
            if(fgetcLogged(in) != ' ')
                return errStatus = ERR;

            curr = fgetcLogged(in); // first actual character
            if(curr < 0) //file did not end in \n
                return errStatus = ERR;
            //remainingDels--; // TODO: should this be here?
            addDelStatus = HUNK_DELETE_TYPE;
            errStatus = 0;
            addToBuffer(curr, addDelStatus == HUNK_APPEND_TYPE);
            return curr;
        }

        if(curr == '>'){
            /*
            // adds must remain
            if(remainingAdds <= 0);
                return ERR;
                */
            
            if(hp->type == HUNK_DELETE_TYPE) // \n- only occur in change hunks
                return errStatus = ERR;

            // checks for '< '
            if(fgetcLogged(in) != ' ')
                return errStatus = ERR;


            curr = fgetcLogged(in);
            if(curr < 0) //file did not end in \n
                return errStatus = ERR;
            //remainingAdds--; // TODO: should this also be here?
            addDelStatus = HUNK_APPEND_TYPE;
            errStatus = 0;
            addToBuffer(curr, addDelStatus == HUNK_APPEND_TYPE);
            return curr;
        }

        if(curr >= '0' && curr <= '9'){
            if(file_finished == 2) return ERR; // EOS ALREADY RETURNED
            ungetc(curr, in);
            lastChar = '\n';
            file_finished = 2;
            return EOS; // runs into a new hunk indication
        }
    }

    if(curr < 0){ //end of file reached
        if(lastChar != '\n') // files must end with \n
            return errStatus = ERR;
        return errStatus = EOS; //reaching \nEOF returns EOS
    }
    errStatus = 0;

    addToBuffer(curr, addDelStatus == HUNK_APPEND_TYPE);
    return curr;
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.

 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    // TO BE IMPLEMENTED
    // @90
    // Check if hunk is available
    
    // HACK: make sure to sync with conditions in HUNK-C
    if(lastUsed == FXN_NEXT && errStatus < 0)
        return; // requires next to be successful
    if(lastUsed == FXN_NONE) // next function has not been used at all
        return;

    // Formatting the 1-3d4 section of the hunk
    fprintf(out, "%d", hp->old_start);
    if(hp->old_start != hp->old_end)
        fprintf(out, "-%d", hp->old_end);
    fprintf(out, "%c",
                hp->type == HUNK_APPEND_TYPE ? 'a' :
                hp->type == HUNK_CHANGE_TYPE ? 'c' :
                hp->type == HUNK_DELETE_TYPE ? 'd' :
                '?'
           );

    fprintf(out, "%d", hp->new_start);
    if(hp->new_start != hp->new_end)
        fprintf(out, "-%d", hp->new_end);
    fprintf(out, "\n");

    // return if not completed, all printing is finished
    if(file_finished < 2)
        return;
    

    // NEXT figure out how to print the output
    char * curr;
    char * start;
    char leader = '?';
    int trunc = 0;

    // print out addition / deletion / first section of change
    if( hp->type == HUNK_DELETE_TYPE || hp->type == HUNK_CHANGE_TYPE){
        start = curr = hunk_deletions_buffer;
        leader = '<';
        trunc = delsTrunc;
    } else { 
        start = curr = hunk_additions_buffer;
        leader = '>';
        trunc = addsTrunc;
    }

    // if curr < HUNK_MAX or 0x0, 0x0 not found
    while(curr - start < HUNK_MAX && (*curr != 0x0  ||  *(curr + 1) != 0x0)){
        curr = outputLine(leader, curr, out);
    }

    if(1 == trunc)
        fprintf(out ,"...\n");
    
    if(hp -> type != HUNK_CHANGE_TYPE)
        return;

    // Print addition section of change
    fprintf(out ,"---\n");
    start = curr = hunk_additions_buffer;
    leader = '>';
    trunc = addsTrunc;

    // if curr < HUNK_MAX or 0x0, 0x0 not found
    while(curr - start < HUNK_MAX && (*curr != 0x0  ||  *(curr + 1) != 0x0)){
        curr = outputLine(leader, curr, out);
    }

    if(1 == trunc)
        fprintf(out ,"...\n");
    return;
}


/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    // TO BE IMPLEMENTED
    // Errors to detect:
    // lines are invalid order @94
    // too many additions or deletions @87
    // empty diff file

    /*
    HUNK hunk;
    int lineCount = 0;
    int hunkNextStatus = 0;

    do{

    } while(hunkNextStatus > 0);
    */


    abort();
}











