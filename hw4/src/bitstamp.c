#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "store.h"
#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"
#include "watcher.h"
#include "argo.h"

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {

    char ** wArgs = malloc(sizeof(char*));
    int sz = 1;
    // don't forget to free these
    for(int i = 2; args[i] != NULL; i++){
        char *temp;
        asprintf(&temp, "%s", args[i]);
        *(wArgs + sz - 1) = temp;
        wArgs = realloc(wArgs, (++sz) *sizeof(char*));

    }
    *(wArgs + sz - 1) = NULL;
    

    int fdp2c[2];
    int fdc2p[2];

    if(pipe(fdp2c) == -1 || pipe(fdc2p) == -1){
        fprintf(stderr, "ERROR: Pipe failed\n");
        printf("pipe failed\n");
        abort();
    }

    WATCHER *wp = newWatcher();
    // wp->pid;
    wp->typ = type;
    wp->args = wArgs;
    wp->tracing = 0;
        
    // fcntl(fd[0], F_SETOWN, getpid());
    // fcntl(fd[0], F_SETOWN, getpid());

    // set the file descriptor to be non-blocking
    // fcntl(fd[0], F_SETSIG, );

    // set the file descriptor to be non-blocking
    // fd_flags = fcntl(STDOUT_FILENO, F_GETFL);
    // fcntl(STDOUT_FILENO, F_SETFL, fd_flags | O_ASYNC | O_NONBLOCK);
    
    // printf("   outside of here  pid: %d\n",  getpid());

    pid_t msPid = fork(); // main fork
    if(msPid == 0){
        close(fdc2p[0]); //close read end of out child
        close(fdp2c[1]); //close write end end in child
        // do rest of the shenanigans here
        

        // feed the execvp output to fdOut
        dup2(fdc2p[1], STDOUT_FILENO);
        dup2(fdp2c[0], STDIN_FILENO);
        // feed execvp additional commands
        // int feedExecVp = dup(STDIN_FILENO);

        // printf("stdout alias : %d, stdin alias: %d\n", fdc2p[1], fdp2c[0]);

        // printf("pid: %d\n", wp->pid);
        // printf("parent pid: %d\n", msPid);
        // printf("   inside msPID loop, counter = %d, pid: %d, parent: %d\n", NORMAL_COUNTER++, getpid(), getppid());
        execvp(wp->typ->argv[0], wp->typ->argv);
    } else {
        // possibly wait for something from the pipe?
        close(fdc2p[1]); //close read end of out child
        close(fdp2c[0]); //close write end end in child
        // printf("   inside of here  pid: %d, parent: %d\n",  getpid(), getppid());
        int fd_flags = fcntl(fdc2p[0], F_GETFL);
        fcntl(fdc2p[0], F_SETFL, fd_flags | O_ASYNC | O_NONBLOCK);
        fcntl(fdc2p[0], F_SETOWN, getpid());
        fcntl(fdc2p[0], F_SETSIG, SIGIO);
        wp->pid = msPid;
        wp->fdIn = fdp2c[1];
        wp->fdOut = fdc2p[0];

        // printf(" used by main: c2p: %d, p2c: %d\n", fdc2p[0], fdp2c[1]);
        // sleep(1);
        // printf("HERE\n");
        // /*
        if(wp->args != NULL){
            char *temp;

            asprintf(&temp, 
                    "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n" ,
                    wp->args[0]
                    );

            wp->typ->send(wp,temp);
            // write(fdp2c[1], temp, len);
            free(temp);
        }
    }
    return wp;
}

int bitstamp_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    removeWatcher(wp);
    printf("bitstamp watcher stop\n");
    return 1;
    // abort();
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    char *temp;
    int len = asprintf(&temp, 
            "%s",
            (char*) arg
            );
    write(wp->fdIn, temp, len);
    free(temp);
    return 1;
    // printf("bitstamp watcher send\n");
    // abort();
}

int compstr(char *str1, char *str2){
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

long lenStr(char * str){
    int i = 0;
    while(str[i++]!=0);
    return i;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    wp->serNum++;
    if(wp->tracing){
        printTrace(wp, txt);
    }

    // printf("beginning characters: %d %d %d", (int)txt[0], txt[1], txt[2]);
    if(compstr(txt, "\b\bSend")){
        return 1;
    } else if(compstr(txt, "\b\bServer message: '{\"data\"")){
        char * buffer = NULL;
        size_t size = 0;
        FILE * fp = open_memstream(&buffer, &size);


        // char * modifiedStart = txt + 19;
        size_t len = lenStr(txt);
        // printf("\n-----------------\n\n\nlen: %ld, msg:   %s\n\n Loaded: ", len, txt);
        for(int i = 19; i < len - 5; i++){
            fprintf(fp, "%c", txt[i]);
            // printf("%c", txt[i]);
        }
        // printf("\n");

        ARGO_VALUE * av = argo_read_value(fp);

        char * tempBuffer = NULL;
        size_t tempSize = 0;
        FILE *tempFp = open_memstream(&tempBuffer, &tempSize);

        ARGO_VALUE *name = argo_value_get_member(av, "channel");
        if(name == NULL) return 0;
        argo_write_value(name, tempFp, 1);

        // int num;
        char buf1[500];
        for(int i = 0; i < 500; i++)
            buf1[i] = 0;
        fread(buf1, sizeof(char), 500, tempFp);
        // printf("\nsize %d, channel: %s\n", num,  buf1);

        int chanLen = lenStr(buf1);
        char nm[chanLen - 2];
        for(int i = 1; i < chanLen - 3; i++)
            nm[i-1] = buf1[i];

        // name = buf1[11];
        nm[chanLen - 4] = 0;
        // printf("name: %s\n", nm);

        // argo_write_value(av, tempFp, 1);
        ARGO_VALUE *inner = argo_value_get_member(av, "data");

        if(inner == NULL) return 0;
        ARGO_VALUE *price = argo_value_get_member(inner, "price");

        double pr;
        argo_value_get_double(price, &pr);
        // argo_write_value(price, tempFp, 1);

        // char buf2[500];
        // num = fread(buf2, sizeof(char), 500, fp);
        // printf("size %d, price: %s", num,  buf2);

        ARGO_VALUE *amount = argo_value_get_member(inner, "amount");
        double amt;
        argo_value_get_double(amount, &amt);
        // argo_write_value(amount, tempFp, 1);

        // printf("price: %lf, amount %lf\n", pr, amt);

        char *priceKey;
        char *amountKey;
        asprintf(&priceKey, "%s:%s:%s", wp->typ->name, nm, "price");
        asprintf(&amountKey, "%s:%s:%s", wp->typ->name, nm, "volume");

        struct store_value psv;
        psv.type = STORE_DOUBLE_TYPE;
        psv.content.double_value = pr;

        struct store_value asv;
        asv.type = STORE_DOUBLE_TYPE;
        asv.content.double_value = amt;


        store_put(priceKey, &psv);
        struct store_value *asvOld = store_get(amountKey);
        if(asvOld == NULL)
            store_put(amountKey, &asv);
        else{
            asv.content.double_value += asvOld->content.double_value;
            store_put(amountKey, &asv);
        }

        // printf("  11111111 args: |%s|\n", priceKey);
        free(priceKey);
        free(amountKey);
        if(asvOld != NULL)
            store_free_value(asvOld);
        argo_free_value(av);

        // /*
        // struct store_value *psvOut = store_get(priceKey);
        // struct store_value *asvOut = store_get(amountKey);

        // printf("key: %s, value: %lf\n", priceKey, psvOut->content.double_value);
        // printf("key: %s, value: %lf\n", amountKey, asvOut->content.double_value);
        // free(psvOut);
        // free(asvOut);
        // */

        /*
        char buf[500];
        int num = fread(buf, sizeof(char), 500, fp);
        printf("read %d, serv msg: %s\n\n", num,  buf);
        */
        // fflush(fp);
        fclose(tempFp);
        free(tempBuffer);
        fclose(fp);
        free(buffer);

    }



    

    // TO BE IMPLEMENTED
    // printf("%s\n", txt);
    return 1;
    // abort();
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    wp->tracing = enable;
    // TO BE IMPLEMENTED
    abort();
}
