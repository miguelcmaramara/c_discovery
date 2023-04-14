#include <bits/types/siginfo_t.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "watcher.h"
#include "ticker.h"
#include "cli.h"

#define INPUT_BUFFER_SIZE 5
/*
 * GLobal Variables
 */
// sigActions
sigset_t setMask;
sigset_t ioMask;
struct sigaction sigintAction;
struct sigaction sigchldAction;
struct sigaction sigIOAction;
struct sigaction sigkillTerm;

// watcher vars
WATCHER *cli_watcher;

/*
 *
 * SIGNAL HANDLERS
 *
 */

// sigint handler
void sigintHandler(int sig){
    printf("Keyboard interupt detected: shutting down.%d\n", sig);
    // todo: create teardown process
    // todo: remove watchers from memory
    // todo: stop processes
    //teardown();
    exit(0);
}

// sigchld handler
void sigchldHandler(int sig){
    printf("\nChild process was stopped with signal %d\n", sig);
    
    // todo: remove the process from the datastore
    
    exit(0);
}

// sigKillTerm handler
void sigKillTermHandler(int sig){
    printf("\nProcess killed(%d) / terminted with signal %d: terminating program.\n", SIGKILL, sig);
    exit(0);
}

// sigint handler
void sigIOHandler(int sig, siginfo_t *info, void*context){

    sigprocmask(SIG_BLOCK, &ioMask, NULL);
    char buff[INPUT_BUFFER_SIZE];
    // printf("\nCAUGHT: input put found with signal %d at file descriptor %d\n", sig, info->si_fd);


    // handle gpio input output
    // if(info->si_fd == STDIN_FILENO)
        
        
    
    // sleep(5);
    
    // fill message with 
    int numRead = read(info->si_fd, buff, INPUT_BUFFER_SIZE);
    int totalRead = 0;
    char *message = malloc(1);

    while(numRead >= 0){
        // remove endline
        if(buff[numRead - 1] == '\n'){
            buff[numRead - 1] = 0;
            numRead--;
        }
        // process buffer
        // printf("    read from file: %s, size: %d\n", buff, numRead);
        message = realloc(message, totalRead + numRead);
        memcpy(message + totalRead, buff, numRead);
        // printf("    total message: %s\n", message);
        totalRead += numRead;



        // clean buffer
        for(int i = 0; i < numRead; i++)
            buff[i] = 0;
        // fill buffer
        numRead = read(info->si_fd, buff, INPUT_BUFFER_SIZE);
    } 

    // add endline
    if(message[totalRead - 1] != 0){
        message = realloc(message, totalRead + 1);
        message[totalRead - 1] = 0;
    }


    if(info->si_fd == STDIN_FILENO){
        watcher_types[CLI_WATCHER_TYPE].recv(cli_watcher, message);
        // cli_watcher_send(WATCHER *wp, void *msg)
    }
    free(message);

    // printf("    read from file: %s\n", buf);
    sigprocmask(SIG_UNBLOCK, &ioMask, NULL);
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
    sigdelset(set, SIGIO);
    // sigdelset(set, SIGIO); //we don't want program to continue with SIGIO
    
    return 0;
}

int setIOMask(){
    sigemptyset(&ioMask);
    sigaddset(&ioMask, SIGIO);
    
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

    sigIOAction.sa_flags = SIGIO;
    sigIOAction.sa_sigaction = sigIOHandler;
    if(sigaction(SIGIO, &sigIOAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGIO HANDLER");
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
    
    // set recieving process for gpio signals
    fcntl(STDIN_FILENO, F_SETOWN, getpid());
    fcntl(STDOUT_FILENO, F_SETOWN, getpid());

    // set the file descriptor to be non-blocking
    int fd_flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, fd_flags | O_ASYNC | O_NONBLOCK);

    // set the file descriptor to be non-blocking
    fd_flags = fcntl(STDOUT_FILENO, F_GETFL);
    fcntl(STDOUT_FILENO, F_SETFL, fd_flags | O_ASYNC | O_NONBLOCK);

    return 1;
}

int initCliWatcher(){
    // Create CLI watcher
    
    watcher_types[CLI_WATCHER_TYPE].start(CLI_WATCHER_TYPE, watcher_types[0].argv);
    // cli_watcher_start(CLI_WATCHER_TYPE, watcher_types[0].argv);
    cli_watcher = watcherLstHead.next;
   
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
     



    while(1){
        watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
        // printf("ticker> ");
        fflush(stdout);
        sigsuspend(&setMask);
    }

    abort();
    return 0;
}
