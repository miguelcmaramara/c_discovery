#include "server.h"
#include "client.h"
#include "client_registry.h"
#include "game.h"
#include "invitation.h"
#include "player.h"
#include "protocol.h"
#include "debug.h"
#include "jeux_globals.h"

#include <bits/time.h>
// #include <criterion/internal/asprintf-compat.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

struct invWrapper{
    int invID;
    struct invWrapper* next;
};

void cleanup(CLIENT *cl, struct invWrapper *head){
    struct invWrapper* prev;
    struct invWrapper* curr = head;
    while(curr != NULL){
        if(curr->invID != -1) //head has -1
            client_revoke_invitation(cl, curr->invID);
        prev = curr;
        curr = curr->next;
        free(prev);
    }
    close(client_get_fd(cl));
    creg_unregister(client_registry, cl);
    // exit(shutCode);
}

int revokeId(CLIENT *cl, struct invWrapper *head, int id){
    struct invWrapper* prev;
    struct invWrapper* curr = head;
    while(curr != NULL){
        if(curr->invID == id){
            if(client_revoke_invitation(cl, curr->invID) == -1)
                return -1;
            prev->next = curr->next;
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

int removeId(struct invWrapper *head, int id){
    struct invWrapper* prev;
    struct invWrapper* curr = head;
    while(curr != NULL){
        if(curr->invID == id){
            prev->next = curr->next;
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

int addId(struct invWrapper *head, int id){
    struct invWrapper* curr = head;
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = malloc(sizeof(struct invWrapper));
    curr->next->next = NULL;
    curr->next->invID = id;
    return 0;
}


void * blankHdr(JEUX_PACKET_HEADER * hdr){
    memset(hdr, 0, sizeof(JEUX_PACKET_HEADER));
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    hdr->type = JEUX_NO_PKT;
    hdr->id = -1;
    hdr->role = -1;
    hdr->size = 0;
    hdr->timestamp_sec = htonl(ts.tv_sec);
    hdr->timestamp_nsec = htonl(ts.tv_nsec);
    return NULL;
}

/*
 * Thread function for the thread that handles a particular client.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 * @return  NULL
 * 
 * This function executes a "service loop" that receives packets from
 * the client and dispatches to appropriate functions to carry out
 * the client's requests.  It also maintains information about whether
 * the client has logged in or not.  Until the client has logged in,
 * only LOGIN packets will be honored.  Once a client has logged in,
 * LOGIN packets will no longer be honored, but other packets will be.
 * The service loop ends when the network connection shuts down and
 * EOF is seen.  This could occur either as a result of the client
 * explicitly closing the connection, a timeout in the network causing
 * the connection to be closed, or the main thread of the server shutting
 * down the connection as part of graceful termination.
 */
void *jeux_client_service(void *arg){
    // info("client service started ");
    int fd = *(int *)arg;
    info("client service started on fd %d", fd);
    free(arg);
    pthread_detach(pthread_self());

    JEUX_PACKET_HEADER inHdr;
    char* dPtr;
    struct pollfd pfd;
    
    struct invWrapper *head = malloc(sizeof(struct invWrapper));
    head->invID = -1;
    head->next = NULL;
    
    
    CLIENT * cl = creg_register(client_registry, fd);
    info("Created client ptr %p", cl);
    // printf("%p, %p\n", cl, &pfd);
    while(1){
        pfd.fd = fd;
        pfd.events = POLLIN | POLLHUP;
        pfd.revents = 0;
        // int n =
        debug("waiting for data");
        poll(&pfd, 1, -1);
        debug("data recieved");

        int ret = proto_recv_packet(fd, &inHdr, (void **)&dPtr);
        debug("recieved packet: fd/hdr/data, client %p, fd %d", cl, fd);
        if(ret == -1){
            info("Read failure, Cleaning up client %p, fd %d", cl, fd);
            cleanup(cl, head);
            return NULL;
        } 
        
        // soft lock before login
        if(client_get_player(cl) == NULL && inHdr.type != JEUX_LOGIN_PKT){
            debug("Client not logged in yet");
            client_send_nack(cl);
            continue;
        } 

        JEUX_PACKET_HEADER hdr;

        switch(inHdr.type){
            case JEUX_LOGIN_PKT:
                info("LOGIN PACKET RECIEVED FROM CLIENT %d", fd);
                debug("   HERER:%p",client_get_player(cl));
                if(client_get_player(cl) == NULL){
                    // PLAYER * pl = player_create(dPtr);
                    PLAYER * pl = preg_register(player_registry, dPtr);
                    client_login(cl, pl);
                    player_unref(pl, "player logged into client No more need.");

                    info("SENDING LOGIN ACK to fd %d", fd);
                    // blankHdr(&hdr);
                    // hdr.type = JEUX_ACK_PKT;

                    // client_send_packet(cl, &hdr, NULL);
                    client_send_ack(cl, NULL, 0);
                } else {
                    client_send_nack(cl);
                }
                break;

            case JEUX_USERS_PKT:
                info("USERS PACKET RECIEVED FROM CLIENT %d", fd);
                // if( 0){ // placeholder
                // }
                char buf[1000];
                memset(buf, 0, 1000);
                PLAYER ** players = creg_all_players(client_registry);
                PLAYER ** temp = players;

                while(*(players)){
                    // printf("HERE\n");
                    // error("%p\n", *players);
                    strcat(buf, player_get_name(*players));
                    // printf("    %p", *players);
                    strcat(buf, "\t");
                    char minibuf[1000];
                    memset(minibuf, 0, 1000);
                    sprintf(minibuf, "%d", player_get_rating(*players));
                    strcat(buf, minibuf);
                    strcat(buf, "\n");
                    player_unref(*players, "player used for USERS, reference no longer needed");
                    players++;
                }
                free(temp); // malloc'd list
                // break;

                // prep packet
                info("Prepping header to send an ack");
                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                blankHdr(&hdr);
                hdr.type = JEUX_ACK_PKT;
                hdr.size = htons(strlen(buf));
                hdr.timestamp_sec = htonl(ts.tv_sec);
                hdr.timestamp_nsec = htonl(ts.tv_nsec);

                info("Sending packet");
                // send packet
                if(client_send_packet(cl, &hdr, &buf) == -1 ){
                    error("FAILED TO SEND, cl/pkt/buf %p/%p/%s", &cl, &hdr, buf);
                }

                break;

                // invite packet 
                case JEUX_INVITE_PKT:
                info("INVITE PACKET RECIEVED FROM CLIENT %d. Checking Sources", fd);
                CLIENT * source = cl;
                CLIENT * target = creg_lookup(client_registry, dPtr);
                if(target == NULL){
                    error("INVITED CLIENT username %s DOES NOT EXIST", dPtr);
                    client_send_nack(cl); //  bad target string
                    break;
                }
                if(target == source){
                    debug("target client cannot be source client");
                    client_send_nack(cl);
                    client_unref(target, "Invite referenes finished");
                    break;
                }
                if(inHdr.role != 1 && inHdr.role != 2){
                    error("inHdr.role (%d) is not a 1 or a 2", inHdr.role);
                    client_send_nack(cl);   //bad data  
                    client_unref(target, "Invite referenes finished");;
                    break;
                }
                GAME_ROLE source_role =  inHdr.role == 2? FIRST_PLAYER_ROLE: SECOND_PLAYER_ROLE;
                GAME_ROLE target_role =  inHdr.role == 1? FIRST_PLAYER_ROLE: SECOND_PLAYER_ROLE;
                
                info("invitation verified, creating invitation and packets");

                JEUX_PACKET_HEADER ackHdr;
                memset(&ackHdr, 0, sizeof(JEUX_PACKET_HEADER));
                // verify ackHdr
                blankHdr(&ackHdr);
                ackHdr.type = JEUX_ACK_PKT;

                ackHdr.id = client_make_invitation( source, target, source_role, target_role);
                if((int8_t)(ackHdr.id) == -1){
                    error("Source cannot add invitation");
                    client_send_nack(cl);   //bad data  
                    client_unref(target, "Invite referenes finished");;
                    break;
                }
                info("AckHeader: %d %d %d %lu %lu %lu",
                        ackHdr.type,
                        ackHdr.id,
                        ackHdr.role,
                        (unsigned long) htons(ackHdr.size),
                        (unsigned long) htonl(ackHdr.timestamp_sec),
                        (unsigned long) htonl(ackHdr.timestamp_nsec)
                        );
                addId(head, ackHdr.id);

                info("invitation added to client and sent to target");

                info("Both invitations added to client, sending packets");
                client_send_packet(source, &ackHdr, NULL);
                client_unref(target, "Invite referenes finished");
                info("Packets sent");
                break;

            case JEUX_REVOKE_PKT:
                info("REVOKE COMMAND CALLED ON cl %p", cl);
                int revId = inHdr.id;
                if(revokeId(cl, head, revId) == 0){
                    info("REVOKE SUCCEEDED");
                    client_send_ack(cl, NULL, 0);
                } else {
                    info("REVOKE DIDN'T SUCCEED");
                    client_send_nack(cl);
                }
                break;
            case JEUX_DECLINE_PKT:
                info("DECLINE COMMAND CALLED ON cl %p", cl);
                int declineId = inHdr.id;
                if(client_decline_invitation(cl, declineId) == 0){
                    info("DECLINE SUCCEEDED");
                    removeId(head, declineId);
                    client_send_ack(cl, NULL, 0);
                } else {
                    info("DECLINE DIDN'T SUCCEED");
                    client_send_nack(cl);
                }
                break;
            case JEUX_ACCEPT_PKT:
                info("ACCEPT COMMAND CALLED ON cl %p", cl);
                int acceptId = inHdr.id;
                char * gameState;
                if(client_accept_invitation(cl, acceptId, &gameState) == 0){
                    info("ACCEPT SUCCEEDED");
                    if(gameState == NULL){
                        info("ACCEPT GAMESTATE IS NULL");
                        client_send_ack(cl, NULL, 0);
                    } else {
                        info("ACCEPT GAMESTATE IS %s", gameState);
                        client_send_ack(cl, gameState, strlen(gameState));
                    }
                    removeId(head, acceptId);
                } else {
                    info("ACCEPT DIDN'T SUCCEED");
                    client_send_nack(cl);
                }
                break;
            case JEUX_MOVE_PKT:
                info("MOVE COMMAND CALLED ON cl %p", cl);
                int moveId = inHdr.id;
                if(client_make_move(cl, moveId, dPtr) == 0){
                    info("MOVE SUCCEEDED");
                        client_send_ack(cl, NULL, 0);
                } else {
                    info("MOVE DIDN'T SUCCEED");
                        client_send_nack(cl);
                }
                break;
            case JEUX_RESIGN_PKT:
                info("MOVE COMMAND CALLED ON cl %p", cl);
                int resignId = inHdr.id;
                if(client_resign_game(cl, resignId) == 0){
                    info("RESIGN SUCCEEDED");
                    removeId(head, resignId);
                    client_send_ack(cl, NULL, 0);
                } else {
                    info("RESIGN SUCCEED");
                    client_send_nack(cl);
                }
                break;
        }
        if(dPtr != NULL){
            info("freeing unused dPtr");
            free(dPtr);
        }
    }
    return NULL;
}
