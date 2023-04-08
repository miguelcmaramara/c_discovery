#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "watcher.h"
#include "ticker.h"
#include "cli.h"

/*
 * GLobal Variables
 */
// sigActions
sigset_t setMask;
struct sigaction sigintAction;
struct sigaction sigchldAction;
struct sigaction sigkillTerm;



/*
 *
 * SIGNAL HANDLERS
 *
 */

/*
 * sigint handler
 */
void sigintHandler(int sig){
    printf("\nCAUGHT: Process interupted with signal %d: terminating program.\n", sig);
    exit(0);
}

/*
 * sigchld handler
 */
void sigchldHandler(int sig){
    printf("\nChild process was stopped with signal %d\n", sig);
    
    // todo: remove the process from the datastore
    
    // exit(0);
}

/*
 * sigKillTerm handler
 */
void sigKillTermHandler(int sig){
    printf("\nProcess killed(%d) / terminted with signal %d: terminating program.\n", SIGKILL, sig);
    // exit(0);
}


/*
 *
 * MASKS
 *
 */

/*
 * Ignores all signals but sigint
 */
int sigMask(sigset_t *set){
    sigfillset(set);
    sigdelset(set, SIGINT);
    sigdelset(set, SIGCHLD);
    sigdelset(set, SIGKILL);
    sigdelset(set, SIGTERM);
    
    return 0;
}



/*
 * 
 * Helper functions
 *
 */



int initTicker(){
    // setup masks
    sigMask(&setMask);
    
    // SigInt Handler
    sigintAction.sa_flags = SIGINT;
    sigintAction.sa_handler = sigintHandler;
    if(sigaction(SIGINT, &sigintAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGINT ACTION");
        return 0;
    }

    // sigchld Handler
    sigchldAction.sa_flags = SIGCHLD;
    sigchldAction.sa_handler = sigchldHandler;
    if(sigaction(SIGCHLD, &sigchldAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGCHLD HANDLER");
        return 0;
    }

    // sigkillTerm Handler
    sigkillTerm.sa_flags = SIGKILL | SIGTERM;
    sigkillTerm.sa_handler = sigKillTermHandler;
    // if(sigaction(SIGKILL, &sigkillTerm, NULL) == -1){
        // fprintf(stderr, "FAILED TO ATTACH SIGKILL HANDLER");
        // return 0;
    // }
    // if(sigaction(SIGTERM, &sigkillTerm,NULL) == -1){
        // fprintf(stderr, "FAILED TO ATTACH SIGTERM HANDLER");
        // return 0;
    // }
    return 1;
}

int initCliWatcher(){
    // Create CLI watcher
    
    cli_watcher_start(CLI_WATCHER_TYPE, watcher_types[0].argv);
    // cleanupCrew()

    return 0;
}




/*
 *  Watcher list functions
 */



/*
void testExecvp(){
    // abort();
    printf("before\n");
    // char *args[] = {"uwsc", "wss://ws.bitstamp.net", NULL};
    // char *args[] = {"ls", NULL};
    // pid_t p = fork();
    //child
    // if(p == 0){
        // printf("p before p2: %d\n", p);
        
        // pid_t p2 = fork();
        // if(p2 == 0){
            // printf("CHILD2: p2 %d\n", p2);
            // printf("CHILD2: p after p2 %d\n", p);
        // } else {
        // }
        // pid_t p3 = fork();
        // if(p3 == 0)p           printf("p3 %d\n", p3);
            // printf("CHILD3: p2 after p2: %d\n", p2);
            // printf("CHILD3: p after p3: %d\n", p3);
        // }
    // }
        
   
}*/



int ticker(void) {
    initTicker();
    initCliWatcher();
    // testExecvp();
     



    printf("before sleep \n");
    sigsuspend(&setMask);
    for(long i = 0; i < 1000000000000; i++);
    printf("finished sleeping, pid: %d \n", getpid());

    abort();
    return 0;
}
