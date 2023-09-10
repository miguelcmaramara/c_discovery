#include "jeux_globals.h"
#include "invitation.h"
#include "client.h"
#include "game.h"
#include "debug.h"
#include <pthread.h>
#include <stdlib.h>

/*
 * An INVITATION records the status of an offer, made by one CLIENT
 * to another, to participate in a GAME.  The CLIENT that initiates
 * the offer is called the "source" of the invitation, and the CLIENT
 * that is the recipient of the offer is called the "target" of the
 * invitation.  An INVITATION stores references to its source and
 * target CLIENTs, whose reference counts reflect the existence of
 * these references.  At any time, an INVITATION may be in one of
 * three states: OPEN, ACCEPTED, or CLOSED.  A newly created INVITATION
 * starts out in the OPEN state.  An OPEN invitation may be "accepted"
 * or "declined" by its target.  It may also be "revoked" by its
 * source.  An invitation that has been accepted by its target transitions
 * to the ACCEPTED state.  In association with such a transition a new
 * GAME is created and a reference to it is stored in the INVITATION.
 * An invitation that is declined by its target or revoked by its
 * source transitions to the CLOSED state.  An invitation in the
 * ACCEPTED state will also transition to the CLOSED state when the
 * game in progress has ended.
 */

struct invitation{
    pthread_mutex_t lock;
    enum invitation_state state;
    CLIENT * source;
    CLIENT * target;
    GAME_ROLE source_role;
    GAME_ROLE target_role;
    GAME * game;
    int numRef;
};

/*
 * Create an INVITATION in the OPEN state, containing reference to
 * specified source and target CLIENTs, which cannot be the same CLIENT.
 * The reference counts of the source and target are incremented to reflect
 * the stored references.
 *
 * @param source  The CLIENT that is the source of this INVITATION.
 * @param target  The CLIENT that is the target of this INVITATION.
 * @param source_role  The GAME_ROLE to be played by the source of this INVITATION.
 * @param target_role  The GAME_ROLE to be played by the target of this INVITATION.
 * @return a reference to the newly created INVITATION, if initialization
 * was successful, otherwise NULL.
 */
INVITATION *inv_create(CLIENT *source, CLIENT *target,
               GAME_ROLE source_role, GAME_ROLE target_role){

    info("creating invitation using src: %p, targ: %p, srcRole: %d, tarRole: %d",
            source,
            target,
            source_role,
            target_role
            );
    INVITATION *res = calloc(1, sizeof(INVITATION));
    if(res == NULL){
        error("FAILED TO CALLOC");
        return NULL;
    }
    if(source == NULL){
        error("Source can't be null");
        return NULL;
    }
    if(target == NULL){
        error("target can't be null");
        return NULL;
    }

    if(target == source){
        error("Source can't be target");
        return NULL;
    }
    client_ref(source, "source referenced in invitation");
    client_ref(target, "target referenced in invitation");
    res->source = source;
    res->target = target;
    res->state = INV_OPEN_STATE;
    res->source_role = source_role;
    res->target_role = target_role;
    res->game = NULL;
    if(pthread_mutex_init(&(res->lock), NULL) != 0){
        error("invitation lock failed to init");
        return NULL;
    }
    res->numRef = 0;
    inv_ref(res, "Initializing invitation");

    info("Invitation completed");
    return res;

}

/*
 * Increase the reference count on an invitation by one.
 *
 * @param inv  The INVITATION whose reference count is to be increased.
 * @param why  A string describing the reason why the reference count is
 * being increased.  This is used for debugging printout, to help trace
 * the reference counting.
 * @return  The same INVITATION object that was passed as a parameter.
 */
INVITATION *inv_ref(INVITATION *inv, char *why){
    if(inv == NULL) {
        error("attempting to reference null invitation");
        return NULL;
    }
    pthread_mutex_lock(&inv->lock);
    info("Invitation ref count incremented (%d -> %d) due to %s",
            inv->numRef, inv->numRef + 1, why);
    inv->numRef++;
    pthread_mutex_unlock(&inv->lock);
    return inv;
}

/*
 * Decrease the reference count on an invitation by one.
 * If after decrementing, the reference count has reached zero, then the
 * invitation and its contents are freed.
 *
 * @param inv  The INVITATION whose reference count is to be decreased.
 * @param why  A string describing the reason why the reference count is
 * being decreased.  This is used for debugging printout, to help trace
 * the reference counting.
 *
 */
void inv_unref(INVITATION *inv, char *why){
    if(inv == NULL) {
        error("attempting to unreference null invitation");
        return;
    }
    pthread_mutex_lock(&inv->lock);
    if(inv->numRef <= 0){
        error("INVITAION AT COUNT %d, UNREF SHOULD NOT HAVE BEEN CALLED", inv->numRef);
        pthread_mutex_unlock(&inv->lock);
        return;
    }
    info("Invitation ref count decremented (%d -> %d) due to %s",
            inv->numRef, inv->numRef - 1, why);
    inv->numRef--;
    if(inv->numRef == 0){
        info("No references: freeing!");
        if(inv->game != NULL)
            game_unref(inv->game, "Associated invitation being destroyed");
        pthread_mutex_unlock(&inv->lock);
        pthread_mutex_destroy(&inv->lock);
        client_unref(inv->source, "invitation destroyed, so is source ref");
        client_unref(inv->target, "invitation destroyed, so is target ref");
        free(inv);
        return;
    }
    pthread_mutex_unlock(&inv->lock);
    return;
}

/*
 * Get the CLIENT that is the source of an INVITATION.
 * The reference count of the returned CLIENT is NOT incremented,
 * so the CLIENT reference should only be regarded as valid as
 * long as the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the CLIENT that is the source of the INVITATION.
 */
CLIENT *inv_get_source(INVITATION *inv){
    if(inv == NULL) {
        error("Getting source from invitation but inv is NULL");
        return NULL;
    }
    pthread_mutex_lock(&inv->lock);
    info("Getting source from invitation");
    CLIENT * res = inv->source;
    pthread_mutex_unlock(&inv->lock);
    return res;
}

/*
 * Get the CLIENT that is the target of an INVITATION.
 * The reference count of the returned CLIENT is NOT incremented,
 * so the CLIENT reference should only be regarded as valid if
 * the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the CLIENT that is the target of the INVITATION.
 */
CLIENT *inv_get_target(INVITATION *inv){
    if(inv == NULL) {
        error("Getting target from invitation but inv is NULL");
        return NULL;
    }
    pthread_mutex_lock(&inv->lock);
    info("Getting target from invitation");
    CLIENT * res = inv->target;
    pthread_mutex_unlock(&inv->lock);
    return res;
}

/*
 * Get the GAME_ROLE to be played by the source of an INVITATION.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME_ROLE played by the source of the INVITATION.
 */
GAME_ROLE inv_get_source_role(INVITATION *inv){
    if(inv == NULL) {
        error("Getting source_role from invitation but inv is NULL");
        return NULL_ROLE;
    }
    pthread_mutex_lock(&inv->lock);
    info("Getting source_role from invitation");
    GAME_ROLE res = inv->source_role;
    pthread_mutex_unlock(&inv->lock);
    return res;
}

/*
 * Get the GAME_ROLE to be played by the target of an INVITATION.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME_ROLE played by the target of the INVITATION.
 */
GAME_ROLE inv_get_target_role(INVITATION *inv){
    if(inv == NULL) {
        error("Getting target_role from invitation but inv is NULL");
        return NULL_ROLE;
    }
    pthread_mutex_lock(&inv->lock);
    info("Getting target_role from invitation");
    GAME_ROLE res = inv->target_role;
    pthread_mutex_unlock(&inv->lock);
    return res;
}

/*
 * Get the GAME (if any) associated with an INVITATION.
 * The reference count of the returned GAME is NOT incremented,
 * so the GAME reference should only be regarded as valid as long
 * as the INVITATION has not been freed.
 *
 * @param inv  The INVITATION to be queried.
 * @return the GAME associated with the INVITATION, if there is one,
 * otherwise NULL.
 */
GAME *inv_get_game(INVITATION *inv){
    if(inv == NULL) {
        error("Getting game from invitation but inv is NULL");
        return NULL;
    }
    pthread_mutex_lock(&inv->lock);
    info("Getting game from invitation");
    GAME *res = inv->game;
    pthread_mutex_unlock(&inv->lock);
    return res;
}

/*
 * Accept an INVITATION, changing it from the OPEN to the
 * ACCEPTED state, and creating a new GAME.  If the INVITATION was
 * not previously in the the OPEN state then it is an error.
 *
 * @param inv  The INVITATION to be accepted.
 * @return 0 if the INVITATION was successfully accepted, otherwise -1.
 */
int inv_accept(INVITATION *inv){
    if(inv == NULL) {
        error("Attempting to accept NULL invitation");
        return -1;
    }

    pthread_mutex_lock(&inv->lock);
    if(inv->state == INV_OPEN_STATE){
        info("Good conditions to accept invitation");

        inv->game = game_create();
        if(inv -> game == NULL){
            // fail, game does not create
            error("Failed to create game");
            pthread_mutex_unlock(&inv->lock);
            return -1;
        }

        inv->state = INV_ACCEPTED_STATE;
        pthread_mutex_unlock(&inv->lock);
        return 0;
    }

    // fail condtion
    error("Bad conditions to accept. state is %d", inv->state);
    pthread_mutex_unlock(&inv->lock);
    return -1;

}

/*
 * Close an INVITATION, changing it from either the OPEN state or the
 * ACCEPTED state to the CLOSED state.  If the INVITATION was not previously
 * in either the OPEN state or the ACCEPTED state, then it is an error.
 * If INVITATION that has a GAME in progress is closed, then the GAME
 * will be resigned by a specified player.
 *
 * @param inv  The INVITATION to be closed.
 * @param role  This parameter identifies the GAME_ROLE of the player that
 * should resign as a result of closing an INVITATION that has a game in
 * progress.  If NULL_ROLE is passed, then the invitation can only be
 * closed if there is no game in progress.
 * @return 0 if the INVITATION was successfully closed, otherwise -1.
 */
int inv_close(INVITATION *inv, GAME_ROLE role){
    if(inv == NULL) {
        error("Attempting to close NULL invitation");
        return -1;
    }

    pthread_mutex_lock(&inv->lock);
    if(inv->state == INV_OPEN_STATE || inv->state == INV_ACCEPTED_STATE){
        info("Good conditions to close invitation");

        if(inv->game == NULL){
            // fail, game does not create
            info("Game does not exist, closing ivitation");
            inv->state = INV_CLOSED_STATE;
            pthread_mutex_unlock(&inv->lock);
            return 0;
        }

        if(game_is_over(inv->game) == 0){
            info("Game is not over, resign by role %d", role);
            if(role == NULL_ROLE){
                error("NULL_ROLE cannot resign from started game");
                return -1;
            }
            if(game_resign(inv->game, role) == -1){
                error("Game resignation failed");
                return -1;
            }
        }

        info("closing invitation");
        inv->state = INV_CLOSED_STATE;
        pthread_mutex_unlock(&inv->lock);

        return 0;
    }

    // fail condtion
    error("Bad conditions to close. state is %d", inv->state);
    pthread_mutex_unlock(&inv->lock);
    return -1;
}

