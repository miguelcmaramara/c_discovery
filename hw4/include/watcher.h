#ifndef WATCHDEF
#define WATCHDEF

#include "ticker.h"
#include <stdio.h>
#include <unistd.h>

/*
 * watcher typedef
 */
struct watcher {
    int wid;
    pid_t pid;
    long type;
    WATCHER_TYPE * typ;
    FILE *fileIn;
    FILE *fileOut;
    int serNum;
    char *lastMsg;
    int tracing;
    struct watcher *next;
    struct watcher *prev;
};

extern struct watcher watcherLstHead;
//struct watcher watcherLstHead;
int removeWatcher(struct watcher *watcherPtr);
struct watcher *getWatcherByPos(int i);
struct watcher *getWatcherByWid(int i);
int cleanWatcher(struct watcher *watcherPtr);
int numWatchers();
int printWatchers();
int printWatcher(struct watcher *watcherPtr);
int watchersString(char** str);
int watcherString(char** str, struct watcher *watcherPtr);
struct watcher *newWatcher();
int initwatcherLstHead();
int printTrace(struct watcher *wp, char *txt);

#endif // !WATCHDEF
