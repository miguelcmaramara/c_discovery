#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#include "watcher.h"
#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"


// int cleanupCrew(WATCHER *wp){
    // pid_t pid= fork();

    // if(pid == 0){
        // printf("cleanupCrew employed for: , ");
        // printWatcher(wp);
        // waitpid(wp->pid, int *stat_loc, int options);
    // }
    
// }

char USR_CMD_START[] = "start";
char USR_CMD_STOP[] = "stop";
char USR_CMD_TRACE[] = "trace";
char USR_CMD_UNTRACE[] = "untrace";
char USR_CMD_SHOW[] = "show";
char USR_CMD_QUIT[] = "quit";
char USR_CMD_WATCHERS[] = "watchers";

char *USR_CMD_ALL[] = {
    "start",
    "stop",
    "trace",
    "untrace",
    "show",
    "quit",
    "watchers"
};
const int USR_CMD_NUM = 7;

/*
 * Helper functions
 */
int compareStrings(char *str1, char *str2){
    // compares that string 2 is the same as the first characters of string1
    char *s1, *s2;
    for(s1 = str1, s2 = str2; *s1 && *s2; s1++, s2++)
        if(*s1 != *s2)  // mid-string difference
            return 0;
    if(!*s1 && *s2) // string 1 can't continue, string 2 can
        return 0; 
    return 1; //subset
    // if(*s1 && !*s2) // string 1 can continue, string 2 cannot
    // return 0; 
}

int lenString(char *str){
    char* s;
    for(s = str; *s; s++);
    return s - str;

}


WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    WATCHER *cwatcher = newWatcher();
    // printf("cwatcher: %p\n", cwatcher);
    cwatcher->type = CLI_WATCHER_TYPE;
    cwatcher->typ = &watcher_types[CLI_WATCHER_TYPE];
    cwatcher->pid = -1;
    cwatcher->fileIn = stdin;
    cwatcher->fileOut = stdout;
    cwatcher->serNum = 0;
    cwatcher->tracing = 0;

    /*
    // test code for concurency
    pid_t msPid = fork();
    if(msPid != 0){
        // create cwatcher ds
        cwatcher->fileIn = stdin;
        cwatcher->fileOut = stdout;


        // cleanupcrew fork
        pid_t ccPid = fork();
        if(ccPid != 0){
            printf("pid: %d, cleanup crew employed for: pid %d\n", getpid(), ccPid);
            printWatchers();
            // wait for child to finish
            waitpid(ccPid, NULL, WUNTRACED);

            // cleanup
            cleanWatcher(cwatcher);
            // see status afterwards
            printf("pid: %d, inner finished\n", getpid());
            printWatchers();

            // abort itself
            exit(0);
        } 

        if( ccPid == 0){
            // do rest of the shenanigans here
            cwatcher->pid = -1;
            printf("pid: %d\n", cwatcher->pid);
            printf("truepid: %d\n", ccPid);
            for(long i = 0; i <= 3000000000; i++){
                if(i % 100000000 == 0)
                    printf("pid: %d, i: %ld, ccPid\n", getpid(), i);

            }
            exit(1);
            
        }
    }
    */
    return cwatcher;
}

int cli_watcher_stop(WATCHER *wp) {
    if(wp->pid != -1)
        removeWatcher(wp);
    else
        cleanWatcher(wp);
    return 0;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    fprintf(wp->fileOut, "%s", (char *)arg);
    fflush(wp ->fileOut);
    return 0;

    // abort();
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    wp->serNum++;
    if(wp->tracing){
        printTrace(wp, txt);
    }

    // handle each case
    if(compareStrings(txt, USR_CMD_START))
        printf("  arg matches USR_CMD_START\n");
        for(struct watcher_type *wtp = watcher_types; *wtp; wtp++){
            if(compareStrings(txt + 5, wtp->name)){

                int sz = 0;
                char *args[] = malloc(sizeof(char*));
                char *tok = strtok(txt, " ")
                while( tok!= 0){
                    args = realloc(args, (++sz) * sizeof(char*))
                    args[sz - 1] = tok;
                    tok = strtok(txt, " ");
                }
                
                wtp->start(wtp, args);
                free(*args);
            }

        }
        
    else if(compareStrings(txt, USR_CMD_STOP))
        printf("  arg matches USR_CMD_STOP\n");
    else if(compareStrings(txt, USR_CMD_TRACE)){
        // Handle a trace
        printf("  arg matches USR_CMD_TRACE\n");
        WATCHER *wp_trace = getWatcherByWid(atoi(txt + 5));

        if(wp_trace == NULL)
            cli_watcher_send(wp, "???\n"); //error
        else{
            wp_trace->tracing = 1;
        }

    } else if(compareStrings(txt, USR_CMD_UNTRACE))
        // Handle an untrace
        printf("  arg matches USR_CMD_UNTRACE\n");
        WATCHER *wp_trace = getWatcherByWid(atoi(txt + 7));

        if(wp_trace == NULL)
            cli_watcher_send(wp, "???\n"); //error
        else{
            wp_trace->tracing = 0;
        }
    else if(compareStrings(txt, USR_CMD_QUIT))
        printf("  arg matches USR_CMD_QUIT\n");
    else if(compareStrings(txt, USR_CMD_WATCHERS)){
        // printf("  arg matches USR_CMD_WATCHERS\n");
        char* temp;
        // printWatchers();
        watchersString(&temp);
        wp->typ->send(wp, temp);
        free(temp);
        // printWatchers();
        
    }else {
        cli_watcher_send(wp, "???\n");
    }
    return 0;
}


int cli_watcher_trace(WATCHER *wp, int enable) {
    wp->tracing = enable;
    return 0;
    // abort();
}

