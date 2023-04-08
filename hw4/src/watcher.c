#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "ticker.h"
#include "watcher.h"
/*
 * watcher typedef
 */

struct watcher watcherLstHead;

int removeWatcher(struct watcher * watcherPtr){
    kill(watcherPtr->pid, SIGTERM);
    cleanWatcher(watcherPtr);
    return 1;
}

int cleanWatcher(struct watcher * watcherPtr){
    watcherPtr->prev->next = watcherPtr->next;
    if(watcherPtr->next != NULL)
        watcherPtr->prev = NULL;

    free(watcherPtr->lastMsg);
    free(watcherPtr);
    return 1;
}

struct watcher *blankWatcher(){
    printf("CREATED watcher\n");
    struct watcher *res = malloc(sizeof(struct watcher));

    if(res == NULL){
        perror("Cannot create new watcher, out of memory\n");
        return 0;
    }
    res->next = NULL;
    res->prev = NULL;
    res->lastMsg = NULL;
    res->serNum = 0;
    res->wid = 0;
    res->pid = 0;
    res->type = 0;
    res->fileIn = 0;
    res->fileOut = 0;
    return res;
}

struct watcher *newWatcher(){
    struct watcher *watchPtr = &watcherLstHead;
    struct watcher *res = blankWatcher();

    int i = 0;
    // printf("c1\n");
    while(watchPtr->next != NULL){
        if(watchPtr->wid == i){
            watchPtr = watchPtr->next;
            i++;
            continue;
        }

        // printf("c2\n");
        res->prev = watchPtr;
        res->next = watchPtr->next;
        watchPtr->next = res;
        watchPtr->next->prev = res;
        res->wid = i;
        i++;
        return res;
    }
    // printf("c3\n");
    res->prev = watchPtr;
    res->next = NULL;
    watchPtr->next = res;
    return res;

}

int numWatchers(){
    struct watcher *watchPtr = &watcherLstHead;
    int res = 0;
    while(watchPtr->next != NULL){
        res++;
        watchPtr = watchPtr->next;
    }
    return res;
}

struct watcher *getWatcherByPos(int i){
    struct watcher *wp = watcherLstHead.next;
    int j = 0;

    while(wp != NULL){
        if(j == i)
            break;
    }
    return wp;
}

struct watcher *getWatcherByWid(int i){
    struct watcher *wp = watcherLstHead.next;

    while(wp != NULL){
        if(wp->wid == i)
            break;
    }
    return wp;
}

int printWatchers(){
    struct watcher *watchPtr = watcherLstHead.next;
    printf("WATCHERS DELETE ME LATER:\n");
    while(watchPtr != NULL){
        printf("attempting to print: %p\n", watchPtr);
        printWatcher(watchPtr);
        watchPtr = watchPtr->next;
    }
    return 0;
}

int printWatcher(struct watcher *watchPtr){
    if(watchPtr == NULL){
        printf("null watchPtr\n");
        return 0;
    }
    // fprintf(stdout,"here: ptr, %p\n", watchPtr);

    if(watchPtr->type == CLI_WATCHER_TYPE)
        printf("%d	CLI(%d,%d,%d)\n",
                watchPtr->wid,
                -1,
                0,
                1
                );
    else if(watchPtr->type == BITSTAMP_WATCHER_TYPE)
        printf("%d	bitstamp.net(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->pid,
                fileno(watchPtr->fileIn),
                fileno(watchPtr->fileOut)
                );
    else if(watchPtr->type == BLOCKCHAIN_WATCHER_TYPE)
        printf("%d	blockchain.net(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->pid,
                fileno(watchPtr->fileIn),
                fileno(watchPtr->fileOut)
                );
    else
        printf("%d	unclassified_watcher(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->pid,
                fileno(watchPtr->fileIn),
                fileno(watchPtr->fileOut)
                );
    return 0;
}
// int initwatcherLstHead(){
    // was
// }
