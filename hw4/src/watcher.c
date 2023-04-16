#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

#include "ticker.h"
#include "watcher.h"
/*
 * watcher typedef
 */

struct watcher watcherLstHead;

char* tempCpy(char * str){
    char* temp = malloc(sizeof(char*) *strlen(str));
    strcpy(temp, str);
    return temp;
}

int join(char ** str, char delim,  char **args){
    if(args == NULL)
        return -1;
    if(*args == 0)
        return -1;
    char * old = *str;
    // char buf[500];
    // for(int i = 0; i < 500; i++)
        // buf[i] = 0;
    // strcpy(buf, *str);
    int res = asprintf(str, "%s", *args);
    while(*(++args) != NULL){
        res += asprintf(str, "%s%c%s", *str, delim, *args);
        free(old);
        old = *str;
    }
    // free(old);
    return res;
}

int removeWatcher(struct watcher * watcherPtr){
    if(watcherPtr->typ != &watcher_types[CLI_WATCHER_TYPE]){
        kill(watcherPtr->pid, SIGTERM);
        // watcherPtr->pid *=  -1;
    }
        
    // cleanWatcher(watcherPtr);
    return 1;
}

int cleanWatcher(struct watcher * wp){
    if(wp == NULL)
        return 0;
    wp->prev->next = wp->next;
    if(wp->next != NULL)
        // wp->prev = NULL;
        wp->next->prev = wp->prev;

    // char **temp = wp->args;
    // printf(" args: %s\n", wp->args);
    if(wp->args != NULL)
        for(int i = 0; wp->args[i]; i++){
            free(wp->args[i]);
        }
    // while(*temp != NULL){
        // char ** next = temp++;
        // printf(" freeing args %s\n", *temp);
        // free(*temp);
        // temp = next;
    // }

    close(wp->fdIn);
    close(wp->fdOut);
    free(wp->args);
    free(wp->lastMsg);
    free(wp);
    return 1;
}

int purgeWatcher(struct watcher *wp){
    if(wp->typ != &watcher_types[CLI_WATCHER_TYPE]){
        kill(wp->pid, SIGTERM);
        // watcherPtr->pid *=  -1;
    } else {
        cleanWatcher(wp);
    }
        

    return 1;
}

int removeAllWatchers(){
    struct watcher *wp = watcherLstHead.next;

    while(wp != NULL){
        struct watcher *temp = wp->next;
        purgeWatcher(wp);
        wp = temp;
    }
    return 1;
}

struct watcher *blankWatcher(){
    // printf("CREATED watcher\n");
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
    res->fdIn = -1;
    res->fdOut = -1;
    return res;
}

struct watcher *newWatcher(){
    struct watcher *watchPtr = watcherLstHead.next;
    struct watcher *res = blankWatcher();
    struct watcher *prevWatch = &watcherLstHead;

    int i = 0;
    // 
    while(watchPtr != NULL){
        // wid matches position, don't add watcher here
        if(watchPtr->wid != i){
            break;
        }
        prevWatch = watchPtr;
        watchPtr = watchPtr->next;

        i++;

    }
    res->prev = prevWatch;
    res->next = prevWatch->next;
    if(prevWatch->next != NULL)
        prevWatch->next->prev = res;
    prevWatch->next = res;
    res->wid = i;
    i++;
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
        wp = wp->next;
    }
    return wp;
}

struct watcher *getWatcherByPid(int i){
    struct watcher *wp = watcherLstHead.next;

    while(wp != NULL){
        if(wp->pid == i)
            break;
        wp = wp->next;
    }
    return wp;
}

int printWatchers(){
    struct watcher *watchPtr = watcherLstHead.next;
    while(watchPtr != NULL){
        printWatcher(watchPtr);
        watchPtr = watchPtr->next;
    }
    return 1;
}

int printWatcher(struct watcher *watchPtr){
    if(watchPtr == NULL){
        // printf("null watchPtr\n");
        return 0;
    }
    char* res;
    watcherString(&res, watchPtr);
    // fprintf(stdout,"here: ptr, %p\n", watchPtr);
    free(res);
    return 1;
}

int watcherString(char ** strPtrPtr, struct watcher *watchPtr){
    char *typeArgs = NULL;
    char *userArgs = NULL;

    join(&typeArgs, ' ', watchPtr->typ->argv);
    join(&userArgs, ' ', watchPtr->args);
    
    // printf(" BEFORE: %s\n", *strPtrPtr);

    // if(*strPtrPtr != NULL)
        // free(*strPtrPtr);
    // char* temp;

    char * old;
    int num = asprintf(strPtrPtr, 
                "%d	%s(%d,%d,%d)",
                watchPtr->wid,
                watchPtr->typ->name,
                watchPtr->pid,
                watchPtr->fdIn,
                watchPtr->fdOut
                );
    if(typeArgs != NULL){
        // char* temp = malloc(sizeof(char*) *strlen(*strPtrPtr));
        // strcpy(temp, *strPtrPtr);
        old = *strPtrPtr;
        num += asprintf(strPtrPtr,
                "%s %s",
                *strPtrPtr,
                typeArgs
                );
        free(old);
        // free(temp);
        free(typeArgs);
    } if(userArgs != NULL){
        old = *strPtrPtr;
        num += asprintf(strPtrPtr,
                "%s [%s]",
                *strPtrPtr,
                userArgs
                );
        free(old);
        free(userArgs);
    }

    // char * temp = tempCpy(*strPtrPtr);
    char buf[500];
    for(int i = 0; i < 500; i++)
        buf[i] = 0;
    strcpy(buf, *strPtrPtr);
    // old = *strPtrPtr;
    free(*strPtrPtr);
    num += asprintf(strPtrPtr, "%s\n", buf);
    // printf("   OLD: |%s|\n", buf);
    // printf("   NEW: |%s|\n", *strPtrPtr);
    // free(temp);
    return num;
    /*
    if(watchPtr->type == CLI_WATCHER_TYPE){
        num = asprintf(strPtrPtr, 
                "%d	%s(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->typ->name,
                -1,
                0,
                1
                );
    }else if(watchPtr->type == BITSTAMP_WATCHER_TYPE)
        num = asprintf(strPtrPtr, 
                "%d	%s(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->typ->name,
                watchPtr->pid,
                watchPtr->fdIn,// fileno(watchPtr->fileIn),
                watchPtr->fdOut// fileno(watchPtr->fileOut)
                );
    else if(watchPtr->type == BLOCKCHAIN_WATCHER_TYPE)
        num = asprintf(strPtrPtr, 
                "%d	%s(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->typ->name,
                watchPtr->pid,
                watchPtr->fdIn,// fileno(watchPtr->fileIn),
                watchPtr->fdOut// fileno(watchPtr->fileOut)
                );
    else
        num = asprintf(strPtrPtr, 
                "%d	unclassified_watcher(%d,%d,%d)\n",
                watchPtr->wid,
                watchPtr->pid,
                watchPtr->fdIn,// fileno(watchPtr->fileIn),
                watchPtr->fdOut// fileno(watchPtr->fileOut)
                );
    return num;
    */
}

int watchersString(char ** str){
    struct watcher *watchPtr = watcherLstHead.next;
    // char * temp;
    // if(*str != NULL)
        // free(*str);
    int len = watcherString(str, watchPtr);
    watchPtr = watchPtr->next;
        

    while(watchPtr != NULL){
        char * temp;
        len += watcherString(&temp, watchPtr);
        char *temp2 = tempCpy(*str);
        asprintf(str, "%s%s", temp2, temp);
        // printWatcher(watchPtr);
        watchPtr = watchPtr->next;
        free(temp2);
        free(temp);
    }
    return len;
}

int printTrace(struct watcher *wp, char *txt){
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    fprintf(stderr, 
            "[%ld.%ld][%s][ %d][   %d]: '%s'\n",
            tp.tv_sec,
            tp.tv_nsec / 1000,
            wp->typ->name,
            wp->fdIn, // fileno(wp->fileIn),
            wp->serNum,
            txt
            );
    return 1;
}

// int initwatcherLstHead(){
    // was
// }
