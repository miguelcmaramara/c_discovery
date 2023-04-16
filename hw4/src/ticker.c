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

#define INPUT_BUFFER_SIZE 1024
/*
 * GLobal Variables
 */
// sigActions
sigset_t setMask;
sigset_t ioMask;
struct sigaction sigintAction;
struct sigaction sigchldAction;
struct sigaction sigIOAction;
struct sigaction sigPipeAction;
struct sigaction sigPIAction;
struct sigaction sigkillTerm;

// watcher vars
WATCHER *cli_watcher;
int shouldReprint = 1;

// current parent id
pid_t parentId;

/*
 *
 * SIGNAL HANDLERS
 *
 */

// sigint handler
void sigintHandler(int sig){
    printf("\nKeyboard interupt detected: Quitting program\n");
    removeAllWatchers();
    // todo: create teardown process
    // todo: remove watchers from memory
    // todo: stop processes
    //teardown();
    exit(0);
}

// sigchld handler
void sigchldHandler(int sig, siginfo_t *info, void *context){
    // printf("\nChild process was stopped with signal %d\n child process ended %d", sig, info->si_pid);

    int status;
    WATCHER *wp = watcherLstHead.next->next;
    while(wp != NULL){
        if(waitpid(wp->pid, &status, WNOHANG)){
            // printf("cleaning up wp %p, name %s\n", wp, wp->typ->name);
            cleanWatcher(wp);
        }
        wp = wp->next;
    }
    return;

    // wait
    
    // todo: remove the process from the datastore
    
    // exit(0);
}

// sigKillTerm handler
void sigKillTermHandler(int sig){
    printf("\nProcess killed, Quitting program.\n");
    removeAllWatchers();
    exit(0);
}

// sigint handler
void sigIOHandler(int sig, siginfo_t *info, void*context){

    sigprocmask(SIG_BLOCK, &ioMask, NULL);
    // printf("\nCAUGHT: input put found with signal %d at file descriptor %d\n", sig, info->si_fd);
    
    // fill message with 
    WATCHER *wp = watcherLstHead.next;

    while(wp != NULL){

        char buff[INPUT_BUFFER_SIZE];
        // int numRead = read(info->si_fd, buff, INPUT_BUFFER_SIZE);
        int numRead = read(wp->fdOut, buff, INPUT_BUFFER_SIZE);
        int totalRead = 0;
        // char *message = malloc(1);
        char *message = NULL;

        if(numRead == 0){
            wp = wp->next;
            continue;
        }

        while(numRead >= 0){
            // remove endline
            if(buff[numRead - 1] == '\n'){
                buff[numRead - 1] = 0;
                numRead--;
            }
            // process buffer
            message = realloc(message, totalRead + numRead);
            memcpy(message + totalRead, buff, numRead);
            totalRead += numRead;



            // clean buffer
            for(int i = 0; i < numRead; i++)
                buff[i] = 0;
            // fill buffer
            // numRead = read(info->si_fd, buff, INPUT_BUFFER_SIZE);
            numRead = read(wp->fdOut, buff, INPUT_BUFFER_SIZE);
        } 

        if(totalRead > 0){
            // add endline
            if(message[totalRead - 1] != 0){
                message = realloc(message, totalRead + 1);
                message[totalRead] = 0;
            }

            if(wp->typ == &watcher_types[CLI_WATCHER_TYPE])
                shouldReprint = 1;
            wp->typ->recv(wp, message);
            // watcher_types[CLI_WATCHER_TYPE].recv(cli_watcher, message);
        }
        /*
        if(info->si_fd == STDIN_FILENO){
            watcher_types[CLI_WATCHER_TYPE].recv(cli_watcher, message);
            // cli_watcher_send(WATCHER *wp, void *msg)
        } else {
        }
        */
        if(message != NULL)
            free(message);
        wp = wp->next;
    }

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
    sigdelset(set, SIGPIPE);
    // sigdelset(set, SIGIO); //we don't want program to continue with SIGIO
    
    return 1;
}

int setIOMask(){
    sigemptyset(&ioMask);
    sigaddset(&ioMask, SIGIO);
    
    return 1;
}



/*
 * 
 * Helper functions
 *
 */



int initTicker(){
    // setup masks
    sigMask(&setMask);

    parentId = getpid();


    
    // SigInt Handler
    sigintAction.sa_flags = SIGINT;
    sigintAction.sa_handler = sigintHandler;
    if(sigaction(SIGINT, &sigintAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGINT ACTION");
        return 1;
    }

    // sigchld Handler
    sigchldAction.sa_flags = SIGCHLD;
    sigchldAction.sa_sigaction = sigchldHandler;
    if(sigaction(SIGCHLD, &sigchldAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGCHLD HANDLER");
        return 1;
    }

    sigIOAction.sa_flags = SIGIO;
    sigIOAction.sa_sigaction = sigIOHandler;
    if(sigaction(SIGIO, &sigIOAction, NULL) == -1){
        fprintf(stderr, "FAILED TO ATTACH SIGIO HANDLER");
        return 1;
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
    // }
    
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

    return 1;
}




/*
 *  Watcher list functions
 */





int ticker(void) {
    initTicker();
    initCliWatcher();
    // testExecvp();
     



    while(1){
        if(shouldReprint == 1){
            watcher_types[CLI_WATCHER_TYPE].send(cli_watcher, "ticker> ");
            shouldReprint = 0;
        }
        fflush(stdout);
        sigsuspend(&setMask);
        if(getpid() != parentId){
            break;
        }
    }

    // abort();
    return 1;
}
