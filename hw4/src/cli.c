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
    if(!str1 || !str2) // null string
        return 0;
    // compares that string 2 is the same as the first characters of string1
    char *s1, *s2;
    for(s1 = str1, s2 = str2; *s1 && *s2; s1++, s2++)
        if(*s1 != *s2)  // mid-string difference
            return 0;
    if(!*s1 && *s2) // string 1 can't continue, string 2 can
        return 0; 
    return 1; //subset
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
    cwatcher->fdIn = STDIN_FILENO;//stdin;
    cwatcher->fdOut = STDOUT_FILENO;//stdout;
    cwatcher->serNum = 0;
    cwatcher->tracing = 0;
    cwatcher->args = NULL;

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
    // removeAllWatchers();
    cli_watcher_send(wp, "??? - to quit please type 'quit'\n");
    // if(wp->pid != -1)
        // removeWatcher(wp);
    // else
        // cleanWatcher(wp);
    // exit(1);
    return 1;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    char *temp;
    int num = asprintf(&temp, "%s", (char *) arg);
    write(wp->fdOut, temp, num);
    // fprintf(wp->fileOut, "%s", (char *)arg);
    free(temp);
    // fflush(wp ->fileOut);
    return 1;

    // abort();
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    // TODO: implement a "wrong message + error message system"
    wp->serNum++;
    if(wp->tracing){
        printTrace(wp, txt);
    }

    // handle each case
    if(compareStrings(txt, USR_CMD_START)){
        printf("  arg matches USR_CMD_START\n");
        for(WATCHER_TYPE *wtp = watcher_types; *(long *)wtp != 0; wtp++){
            // printf("  text: %s\n", txt);
            if(compareStrings(txt + 6, wtp->name)){

                int sz = 0;
                char **args = malloc(sizeof(char*));
                char *tok = strtok(txt, " ");
                while( tok!= 0){
                    args = realloc(args, (++sz) * sizeof(char*));
                    args[sz - 1] = tok;
                    tok = strtok(NULL, " ");
                }
                args = realloc(args, (++sz) * sizeof(char*));
                args[sz - 1] = NULL;

                
                wtp->start(wtp, args);
                free(args);
                break;
            }

        }
        
    } else if(compareStrings(txt, USR_CMD_STOP)){
        printf("  arg matches USR_CMD_STOP\n");

        WATCHER *wp_stop = getWatcherByWid(atoi(txt + 4));

        if(wp_stop == NULL)
            cli_watcher_send(wp, "??? - Cannot stop Nonexistent watcher\n"); //error
        else{
            wp_stop->typ->stop(wp_stop);
        }

    } else if(compareStrings(txt, USR_CMD_SHOW)){
        printf("  arg matches USR_CMD_SHOW\n");

        printf("  relevant args: |%s|\n", txt + 5);

        struct store_value *svOut = store_get(txt + 5);

        if(svOut == NULL){
            cli_watcher_send(wp, "??? - Key not found\n");
        } else {
            char* buf;
            asprintf(&buf, "%s\t%lf\n", txt + 5, svOut->content.double_value);
            cli_watcher_send(wp, buf);
            // wp->typ->send(buf)
            free(buf);
            free(svOut);
        }



    } else if(compareStrings(txt, USR_CMD_TRACE)){
        // Handle a trace
        printf("  arg matches USR_CMD_TRACE\n");
        WATCHER *wp_trace = getWatcherByWid(atoi(txt + 5));

        if(wp_trace == NULL)
            cli_watcher_send(wp, "??? - Cannot trace Nonexistent watcher\n"); //error
        else{
            wp_trace->tracing = 1;
        }

    } else if(compareStrings(txt, USR_CMD_UNTRACE)){
        // Handle an untrace
        printf("  arg matches USR_CMD_UNTRACE\n");
        WATCHER *wp_trace = getWatcherByWid(atoi(txt + 7));

        if(wp_trace == NULL)
            cli_watcher_send(wp, "???\n"); //error
        else{
            wp_trace->tracing = 0;
        }
    } else if(compareStrings(txt, USR_CMD_QUIT)){
        printf("  arg matches USR_CMD_QUIT\n");

        free(txt);
        wp->typ->stop(wp);
    } else if(compareStrings(txt, USR_CMD_WATCHERS)){
        // printf("  arg matches USR_CMD_WATCHERS\n");
        char* watchers_string_temp = NULL;
        // printWatchers();
        free(watchers_string_temp);
        watchersString(&watchers_string_temp);
        wp->typ->send(wp, watchers_string_temp);
        free(watchers_string_temp);
        // printWatchers();
        
    }else {
        cli_watcher_send(wp, "???\n");
    }
    return 1;
}


int cli_watcher_trace(WATCHER *wp, int enable) {
    wp->tracing = enable;
    return 1;
    // abort();
}

