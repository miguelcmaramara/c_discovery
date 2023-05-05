#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <getopt.h>

#include "csapp.h"

#include "debug.h"
#include "protocol.h"
#include "server.h"
#include "client_registry.h"
#include "player_registry.h"
#include "jeux_globals.h"

#ifdef DEBUG
int _debug_packets_ = 1;
#endif

static void terminate(int status);

struct sigaction sighupAction;
struct sigaction sigintAction;
void sigHandler(int sig, siginfo_t *info, void *context){
    debug("QUITTING HANDLER FOUND\n");
    terminate(EXIT_SUCCESS);
}

/*
 * "Jeux" game server.
 *
 * Usage: jeux <port>
 */
// CLIENT_REGISTRY *testCr;
// CLIENT *crArr[5];
// pthread_t threads[10];
// int *ints[10];

// void *wrapperfxn(void *num){
    // int i = *(int *)num;
    // // if((i/2) * 2 == i)
    // if(i - 5 < 0){
        // printf("cREATION fxn %d is starting\n", i);
        // crArr[i] = creg_register(testCr, dup(1));
    // }else{
        // printf("DESTRUCTION fxn %d is starting\n", i);
        // creg_unregister(testCr, crArr[i - 5]);
    // }
    // printf("fxn %d is finished running\n", i);
    // creg_wait_for_empty(testCr);
    // printf("fxn %d is finished waiting\n", i);
    // return NULL;
// }

// void testfxn(){
    // testCr = creg_init();

    // for( int i = 0; i < 10; i++ ){
        // ints[i] = malloc(sizeof(int));
        // *ints[i] = i;
        // printf("calling fxn %d\n", *ints[i]);
        // pthread_create(&threads[i], NULL, wrapperfxn, ints[i]);
        // // wrapperfxn(ints[i]);
    // }

    // printf("main thread waiting\n");
    // creg_wait_for_empty(testCr);
    // printf("main thread finished waiting\n");

    // for( int i = 0; i < 10; i++ ){
        // free(ints[i]);
    // }
// }

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    opterr = 0;
    int res;
    // int port = -1;
    char* port;
    while(-1 != (res = getopt(argc, argv, "p:"))){
        switch (res){
            case 'p':
                port = optarg;
                // if(atoi(optarg))
                break;
            case '?':
                if(optopt == 'p')
                    error("Port is required for p option.\n");
                else
                    error("Unrecognized argument %c", optopt);
                break;
            default:
                abort();    // getopt went wrong
        }
        
    }
    if(atoi(port) == 0 && port[0] != 0){
        printf("Usage: bin/jeux -p <port>\n");
        exit(EXIT_FAILURE);
        terminate(EXIT_FAILURE);
    }
    info("attempting to connexct to port %s\n", port);
    // printf("port %d\n", port);


    // Perform required initializations of the client_registry and
    // player_registry.
    client_registry = creg_init();
    player_registry = preg_init();

    // set SIGHUP handler;
    sighupAction.sa_flags = SIGHUP;
    sighupAction.sa_sigaction = sigHandler;
    if(sigaction(SIGHUP, &sighupAction, NULL) == -1){
        error("FAILED TO ATTATCH SIGHUPACTION\n");
        terminate(EXIT_FAILURE);
    }

    // setup sigint handler
    sigintAction.sa_flags = SIGINT;
    sigintAction.sa_sigaction = sigHandler;
    if(sigaction(SIGINT, &sigintAction, NULL) == -1){
        error("FAILED TO ATTATCH SIGINTACTION\n");
        exit(EXIT_SUCCESS);
    }

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function jeux_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    
    // create socket and listen for new clients
    int listenfd = open_listenfd(port);
    if(listenfd < 0){
        error("Failed to bind to socket with error code %d, errno %d\n", listenfd, errno);
        terminate(EXIT_FAILURE);
    }

    // handle clients being 
    while(1){
        int *clientfd = malloc(sizeof(int));
        *clientfd = accept(listenfd, NULL, NULL);
        info("Client established at fd %d, ptr %p\n", *clientfd, clientfd);
        // create new thread for accepting client data

        pthread_t tid;
        if(pthread_create(&tid, NULL, jeux_client_service, (void*)clientfd) != 0){
            error("Thread failed to start with error code\n");
            terminate(EXIT_FAILURE);
        }
        info("Thread established at tid %ld\n", tid);
    }


    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);
    
    debug("%ld: Waiting for service threads to terminate...", pthread_self());
    creg_wait_for_empty(client_registry);
    debug("%ld: All service threads terminated.", pthread_self());

    // Finalize modules.
    creg_fini(client_registry);
    preg_fini(player_registry);

    debug("%ld: Jeux server terminating", pthread_self());
    exit(status);
}
