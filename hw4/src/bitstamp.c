#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
    for(int i = 0; i < 3; i++){
        printf("  bitstamp arg %d: %s\n", i, args[i]);
    }
    // WATCHER *wp = newWatcher();
    // wp->type = BITSTAMP_WATCHER_TYPE;
    // cwatcher->pid = -1;
    // cwatcher->fileIn = stdin;
    // cwatcher->fileOut = stdout;
    // TO BE IMPLEMENTED
    abort();
}
// WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    // WATCHER *cwatcher = newWatcher();
    // // printf("cwatcher: %p\n", cwatcher);
    // cwatcher->type = CLI_WATCHER_TYPE;
    // cwatcher->pid = -1;
    // cwatcher->fileIn = stdin;
    // cwatcher->fileOut = stdout;

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
    // return cwatcher;
// }

int bitstamp_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
