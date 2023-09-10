#include "player.h"
#include "debug.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
typedef struct player PLAYER;

/* The initial rating assigned to a player. */
#define PLAYER_INITIAL_RATING 1500
struct player{
    pthread_mutex_t lock;
    char *name;
    int rating;
    int numRef;
};

/*
 * Create a new PLAYER with a specified username.  A private copy is
 * made of the username that is passed.  The newly created PLAYER has
 * a reference count of one, corresponding to the reference that is
 * returned from this function.
 *
 * @param name  The username of the PLAYER.
 * @return  A reference to the newly created PLAYER, if initialization
 * was successful, otherwise NULL.
 */
PLAYER *player_create(char *name){
    info("Creating Player");
    if(name == NULL){
        error("Cannot Create player with NULL name");
        return NULL;
    }


    PLAYER * res = calloc(1, sizeof(PLAYER));
    if(res == NULL){
        error("FAILED TO CALLOC");
        return NULL;
    }

    res->name = calloc(1, sizeof(char) * strlen(name) + 1);
    res->name[strlen(name)] = 0 ;
    if(res->name == NULL){
        error("FAILED TO CALLOC2");
        return NULL;
    }

    if(strcpy(res->name, name) == NULL){
        error("FAILED TO COPY NAME");
        return NULL;
    }

    res->rating = PLAYER_INITIAL_RATING;
    if(pthread_mutex_init(&(res->lock), NULL) != 0){
        error("Player lock failed to init");
        return NULL;
    }

    res->numRef = 0;
    player_ref(res, "Player created with player_create");
    info("Player completed");
    return res;
}

/*
 * Increase the reference count on a player by one.
 *
 * @param player  The PLAYER whose reference count is to be increased.
 * @param why  A string describing the reason why the reference count is
 * being increased.  This is used for debugging printout, to help trace
 * the reference counting.
 * @return  The same PLAYER object that was passed as a parameter.
 */
PLAYER *player_ref(PLAYER *player, char *why){
    if(player == NULL) {
        error("attempting to reference null player");
        return NULL;
    }
    pthread_mutex_lock(&player->lock);
    info("Player [%s] ref count incremented (%d -> %d) due to %s",
            player->name, player->numRef, player->numRef + 1, why);
    player->numRef++;
    pthread_mutex_unlock(&player->lock);
    return player;
}

/*
 * Decrease the reference count on a PLAYER by one.
 * If after decrementing, the reference count has reached zero, then the
 * PLAYER and its contents are freed.
 *
 * @param player  The PLAYER whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 *
 */
void player_unref(PLAYER *player, char *why){
    if(player == NULL) {
        error("attempting to unreference null player");
        return;
    }
    pthread_mutex_lock(&player->lock);
    if(player->numRef <= 0){
        error("PLAYER AT COUNT %d, UNREF SHOULD NOT HAVE BEEN CALLED", player->numRef);
        pthread_mutex_unlock(&player->lock);
        return;
    }
    info("Player [%s] ref count decremented (%d -> %d) due to %s",
            player->name, player->numRef, player->numRef - 1, why);
    player->numRef--;
    if(player->numRef == 0){
        info("No references: freeing!");
        free(player->name);
        pthread_mutex_unlock(&player->lock);
        pthread_mutex_destroy(&player->lock);
        free(player);
        return;
    }
    pthread_mutex_unlock(&player->lock);
    return;
}

/*
 * Get the username of a player.
 *
 * @param player  The PLAYER that is to be queried.
 * @return the username of the player.
 */
char *player_get_name(PLAYER *player){
    if(player == NULL) {
        error("Getting name from null player");
        return NULL;
    }
    pthread_mutex_lock(&player->lock);
    info("Getting name from player");
    char * res = player->name;
    pthread_mutex_unlock(&player->lock);
    return res;
}

/*
 * Get the rating of a player.
 *
 * @param player  The PLAYER that is to be queried.
 * @return the rating of the player.
 */
int player_get_rating(PLAYER *player){
    if(player == NULL) {
        error("Getting rating from null player");
        return -1;
    }
    pthread_mutex_lock(&player->lock);
    info("Getting rating from player");
    int res = player->rating;
    pthread_mutex_unlock(&player->lock);
    return res;
}

/*
 * Post the result of a game between two players.
 * To update ratings, we use a system of a type devised by Arpad Elo,
 * similar to that used by the US Chess Federation.
 * The player's ratings are updated as follows:
 * Assign each player a score of 0, 0.5, or 1, according to whether that
 * player lost, drew, or won the game.
 * Let S1 and S2 be the scores achieved by player1 and player2, respectively.
 * Let R1 and R2 be the current ratings of player1 and player2, respectively.
 * Let E1 = 1/(1 + 10**((R2-R1)/400)), and
 *     E2 = 1/(1 + 10**((R1-R2)/400))
 * Update the players ratings to R1' and R2' using the formula:
 *     R1' = R1 + 32*(S1-E1)
 *     R2' = R2 + 32*(S2-E2)
 *
 * @param player1  One of the PLAYERs that is to be updated.
 * @param player2  The other PLAYER that is to be updated.
 * @param result   0 if draw, 1 if player1 won, 2 if player2 won.
 */
void player_post_result(PLAYER *player1, PLAYER *player2, int result){
    char *ps1 = NULL;
    char *ps2 = NULL;
    ps1 = player_get_name(player2);
    ps2 = player_get_name(player1);

    PLAYER *p1;
    PLAYER *p2;
    double s1;
    double s2;
    double e1;
    double e2;
    if(strcmp(ps1, ps2) > 0){
        p1 = player1;
        p2 = player2;
        s1 = (result == 0) ? .5 : (result == 1) ? 1 : 0;
        s2 = (result == 0) ? .5 : (result == 2) ? 1 : 0;
    } else {
        p1 = player2;
        p2 = player1;
        s1 = (result == 0) ? .5 : (result == 2) ? 1 : 0;
        s2 = (result == 0) ? .5 : (result == 1) ? 1 : 0;
    }

    pthread_mutex_lock(&p1->lock);
    pthread_mutex_lock(&p2->lock);
    e1 = 1/(1 + pow(10 , (float)(p2->rating - p1->rating)/400));
    e2 = 1/(1 + pow(10 , (float)(p1->rating - p2->rating)/400));
    p1->rating = p1->rating + 32 *(s1 - e1);
    p2->rating = p2->rating + 32 *(s2 - e2);
    pthread_mutex_unlock(&p2->lock);
    pthread_mutex_unlock(&p1->lock);
    return;
}
