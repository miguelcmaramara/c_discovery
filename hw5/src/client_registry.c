#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "client_registry.h"
#include "client.h"
#include "debug.h"
#include "player.h"
/*
 * The CLIENT_REGISTRY type is a structure that defines the state of a
 * client registry.  You will have to give a complete structure
 * definition in client_registry.c.  The precise contents are up to
 * you.  Be sure that all the operations that might be called
 * concurrently are thread-safe.
 */
// typedef struct client_registry CLIENT_REGISTRY;
// struct clientWrapper {
    // CLIENT * client;
    // struct clientWrapper * next;
    // int numref;
    // pthread_mutex_t cl_lock;
// }

struct client_registry {
    pthread_mutex_t cr_lock;
    CLIENT * clients[MAX_CLIENTS];
    int numClients;
    sem_t sem;
};

/*
 * Initialize a new client registry.
 *
 * @return  the newly initialized client registry, or NULL if initialization
 * fails.
 */
CLIENT_REGISTRY *creg_init(){
    info("Initializing client registry");
    CLIENT_REGISTRY * res = malloc(sizeof(CLIENT_REGISTRY));
    if(pthread_mutex_init(&(res->cr_lock), NULL) != 0)
        error("Registry LOCK FAILED TO INIT");
    if(sem_init(&(res->sem), 0, 0) != 0)
        error("sem FAILED TO INIT");
    for(int i = 0; i < MAX_CLIENTS; i++){
        res->clients[i] = NULL;
    }
    res->numClients = 0;

    return res;
}

/*
 * Finalize a client registry, freeing all associated resources.
 * This method should not be called unless there are no currently
 * registered clients.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr){
    info("creg_fini called for cr %p", cr);
    
    pthread_mutex_lock(&(cr->cr_lock));
    if(cr->clients[0] == NULL){
        pthread_mutex_unlock(&(cr->cr_lock));
        if(pthread_mutex_destroy(&(cr->cr_lock)) != 0){
            error("creg_fini failed to finalize: caught destroying lock");
            return;
        }
        if(sem_destroy(&(cr->sem)) != 0){
            error("creg_fini failed to finalize: caught destroying semaphore");
            return;
        }
        free(cr);
    } else{
        error("creg_fini called before cleint_registry had registered clients");
        abort();
    }


    // iterate through clients
    // struct clientWrapper *cwrap = cr->head
    // while(cwrap != NULL){
        // pthread_mutex_lock(&(cwrap.cl_lock));
        // free(cwrap.client);
        // pthrad_mutex_
    // }
    
    
}

/*
 * Register a client file descriptor.
 * If successful, returns a reference to the the newly registered CLIENT,
 * otherwise NULL.  The returned CLIENT has a reference count of one.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be registered.
 * @return a reference to the newly registered CLIENT, if registration
 * is successful, otherwise NULL.
 */
CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd){
    info("Creating new client");
    if(cr->numClients == MAX_CLIENTS){
        error("MAX_CLIENTS REACHED, did not create a new client");
        return NULL;
    }
    CLIENT *cl = client_create(cr, fd);
    if(cl == NULL){
        error("Client_create returned NULL");
        return NULL;
    }

    pthread_mutex_lock(&cr->cr_lock);
    cr->clients[cr->numClients++] = cl;
    info("client with fd %d created in %p at position %d", fd, cr, cr->numClients - 1);
    // printf("ADD: num: %d, client: %p, cr: %p %p %p %p %p %p\n",
            // cr->numClients,
            // cl,
            // cr->clients[0],
            // cr->clients[1],
            // cr->clients[2],
            // cr->clients[3],
            // cr->clients[4],
            // cr->clients[5]);
    pthread_mutex_unlock(&cr->cr_lock);
    info("New Client %p created", cl);
    return cl;
}


/*
 * Unregister a CLIENT, removing it from the registry.
 * The client reference count is decreased by one to account for the
 * pointer discarded by the client registry.  If the number of registered
 * clients is now zero, then any threads that are blocked in
 * creg_wait_for_empty() waiting for this situation to occur are allowed
 * to proceed.  It is an error if the CLIENT is not currently registered
 * when this function is called.
 *
 * @param cr  The client registry.
 * @param client  The CLIENT to be unregistered.
 * @return 0  if unregistration succeeds, otherwise -1.
 */
int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client){
    info("unregistering client %p from registry %p", client, cr);
    pthread_mutex_lock(&cr->cr_lock);
    int i = 0;
    for(i = 0; i < cr->numClients; i++){
        if(cr->clients[i] == client){
            cr->clients[i] = NULL;
            break;
        }
    }
    if(i == cr->numClients && cr->clients[cr->numClients] != NULL){
        error("CLIENT %p NOT FOUND IN %p", client, cr);
        return -1;
    }

    info("client previously at position %d", i);
    for(int j = i; j < cr->numClients; j++){    // shifting over all clients
        cr->clients[j] = cr->clients[j + 1];
    }

    cr->numClients--;
    if(cr->numClients == 0){
        // printf("CALL SEM\n");
        sem_post(&cr->sem);
    }
    // printf("REM: num: %d, client: %p, cr: %p %p %p %p %p %p\n",
            // cr->numClients,
            // client,
            // cr->clients[0],
            // cr->clients[1],
            // cr->clients[2],
            // cr->clients[3],
            // cr->clients[4],
            // cr->clients[5]);
    pthread_mutex_unlock(&cr->cr_lock);
    // unref players
    // client_unref(client, "Unregistered from client registry");
    client_unref(client, "Unregistered from client registry");
    return 0;
}

/*
 * Given a username, return the CLIENT that is logged in under that
 * username.  The reference count of the returned CLIENT is
 * incremented by one to account for the reference returned.
 *
 * @param cr  The registry in which the lookup is to be performed.
 * @param user  The username that is to be looked up.
 * @return the CLIENT currently registered under )the specified
 * username, if there is one, otherwise NULL.
 */
CLIENT *creg_lookup(CLIENT_REGISTRY *cr, char *user){
    info("Looking up username \"%s\" in cr %p", user, cr);
    pthread_mutex_lock(&cr->cr_lock);
    for(int i = 0; i < cr->numClients; i++){
        if(client_get_player(cr->clients[i]) == NULL){
            continue;   //client not logged in
        }
        if(strcmp(
                player_get_name(client_get_player(cr->clients[i])),
                user
                ) == 0
        ){
            info("Username \"%s\" found in cr %p at pos %d", user, cr, i);
            pthread_mutex_unlock(&cr->cr_lock);
            return client_ref(cr->clients[i], "creg_lookup matches name");
        }
    }
    info("Username \"%s\" not found in cr %p", user, cr);
    pthread_mutex_unlock(&cr->cr_lock);
    return NULL;

}

/*
 * Return a list of all currently logged in players.  The result is
 * returned as a malloc'ed array of PLAYER pointers, with a NULL
 * pointer marking the end of the array.  It is the caller's
 * responsibility to decrement the reference count of each of the
 * entries and to free the array when it is no longer needed.
 *
 * @param cr  The registry for which the set of usernames is to be
 * obtained.
 * @return the list of players as a NULL-terminated array of pointers.
 */
PLAYER **creg_all_players(CLIENT_REGISTRY *cr){
    info("creg_all_players called for cr ptr %p", cr);
    pthread_mutex_lock(&cr->cr_lock);
    PLAYER **res = calloc(1, cr->numClients * sizeof(PLAYER *) + 1);
    // initialize 0
    // for(int i = 0; i <= cr->numClients; i++){
        // res[i] = NULL;
    // }
    int len = 0;
    info("numClients: %d", cr->numClients);
    for(int i = 0; i < cr->numClients; i++){

        if(client_get_player(cr->clients[i]) == NULL)
            continue;
        res[len++] = player_ref(client_get_player(cr->clients[i]), "creg_all_players");
        debug("res at i %d -> %p, len %d", i, &(res[len - 1]), len);
        // r)es[len++] = client_get_player(cr->clients[i]);
    }
    if(len != cr->numClients){
        debug("HERE");
        res = (PLAYER **) realloc(res, len * sizeof(PLAYER *) + 1);
    }

    pthread_mutex_unlock(&cr->cr_lock);
    return res;
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.  Note that this function may be
 * called concurrently by an arbitrary number of threads.
 *
 * @param cr  The client registry.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr){
    if(cr->numClients == 0) return;
    info("Thread is waiting for cr %p to become empty", cr);
    sem_wait(&cr->sem);
    sem_post(&cr->sem);
    info("Thread finished waiting");
    return;
}

/*
 * Shut down (using shutdown(2)) all the sockets for connections
 * to currently registered clients.  The clients are not unregistered
 * by this function.  It is intended that the clients will be
 * unregistered by the threads servicing their connections, once
 * those server threads have recognized the EOF on the connection
 * that has resulted from the socket shutdown.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr){
    info("Shutting down cr %p", cr);
    pthread_mutex_lock(&cr->cr_lock);
    for(int i = 0; i < cr->numClients; i++){
        info("    Shutting down client %p at index %d", cr->clients[i], i);
        shutdown(client_get_fd(cr->clients[i]), 2);
    }
    pthread_mutex_unlock(&cr->cr_lock);
    return;
}

