// #include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "protocol.h"
#include "debug.h"

const int INPUT_BUFFER_SIZE = 1024;

void ntohsArr(short * in, long sz){
    for(int i = 0; i < sz; i++)
        in[i] = ntohs(in[i]);
    return;
}
void htonsArr(short * in, long sz){
    for(int i = 0; i < sz; i++)
        in[i] = htons(in[i]);
    return;
}


char * getMsg(int fd, int * sz){
    char buff[INPUT_BUFFER_SIZE];
    memset(buff, 0, INPUT_BUFFER_SIZE);
    char *message = NULL;
    info("Reading message: ");
    int numRead = read(fd, buff, sizeof(char)*INPUT_BUFFER_SIZE);
    int totalRead = 0; // in bytes
    if(numRead == 0){
        info("No characters read from fd %d", fd);
        return NULL;
    }

    while(numRead > 0){
        // process buffer
        message = realloc(message, (totalRead + numRead));
        memcpy(message + totalRead, buff, numRead);
        totalRead += numRead;

        // clean buffer
        for(int i = 0; i < numRead; i++)
            buff[i] = 0;

        // fill buffer
        numRead = read(fd, buff, INPUT_BUFFER_SIZE);

    } 

    return message;
}

/*
 * Send a packet, which consists of a fixed-size header followed by an
 * optional associated data payload.
 *
 * @param fd  The file descriptor on which packet is to be sent.
 * @param hdr  The fixed-size packet header, with multi-byte fields
 *   in network byte order
 * @param data  The data payload, or NULL, if there is none.
 * @return  0 in case of successful transmission, -1 otherwise.
 *   In the latter case, errno is set to indicate the error.
 *
 * All multi-byte fields in the packet are assumed to be in network byte order.
 */
int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data){
    info("attempting to send packet header: %p data: %p", hdr, data);

    int datalen = ntohs(hdr->size);

    info("Writing header %p",  hdr);
    int hsz = write(fd, hdr, sizeof(JEUX_PACKET_HEADER));
    if(hsz != sizeof(JEUX_PACKET_HEADER))
        debug("sizes don't match: sz/expeted = %d/%ld", hsz, sizeof(JEUX_PACKET_HEADER));
    info("Writing header %p",  hdr);
    if(datalen > 0 && data!= NULL){
        // htonsArr(data, datalen);

        info("Writing data. Ptr: %p, len %d",  data, datalen);
        int sz = write(fd, data, datalen);
        if( sz != datalen)
            debug("data does not match specified size: hdr->size, numRead: %d, %d" ,
                ntohs(hdr->size),
                sz);
    }

    // JEUX_PACKET_HEADER net_hdr;
    // htons(hdr, sizeof(JEUX_PACKET_HEADER));
    // debug("size of 

    return 0;
}

/*
 * Receive a packet, blocking until one is available.
 *
 * @param fd  The file descriptor from which the packet is to be received.
 * @param hdr  Pointer to caller-supplied storage for the fixed-size
 *   packet header.
 * @param datap  Pointer to a variable into which to store a pointer to any
 *   payload received.
 * @return  0 in case of successful reception, -1 otherwise.  In the
 *   latter case, errno is set to indicate the error.
 *
 * The returned packet has all multi-byte fields in network byte order.
 * If the returned payload pointer is non-NULL, then the caller has the
 * responsibility of freeing that storage.
 */
int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp){
    hdr->size = 0;
    *payloadp = NULL;

    int hsz = read(fd, hdr, sizeof(JEUX_PACKET_HEADER));
    if( hsz != sizeof(JEUX_PACKET_HEADER)){
        debug("header does not match specified size: hdr->size, numRead: %d, %d",
            ntohs(hdr->size),
            hsz);
        // case that EOF was reached
        if(hsz == 0){
            debug("EOF DETECTED");
            // errno=EOF;
            return -1;
        }
    }
    info("recieved %d byte header from %d: typ %d, role: %d, id: %d, sz: %d",
            hsz, fd, hdr->type, hdr->role, hdr->id, ntohs(hdr->size) );

    if(ntohs(hdr->size) > 0){
        // error("I AM IN HERE");
        *payloadp = malloc(ntohs(hdr->size));
        for(int i = 0; i < ntohs(hdr->size); i++){
            *((char*)*payloadp + i) = 0;
        }
        // deal with string payloads. append 0
        switch(hdr->type){
            case JEUX_LOGIN_PKT:
            case JEUX_INVITE_PKT:
            case JEUX_MOVE_PKT:
                debug("STRING BASED: ALLOCATING MORE SPACE (%d -> %d)",
                        ntohs(hdr->size), ntohs(hdr->size) + 1);
                *payloadp = realloc(*payloadp, ntohs(hdr->size) + 1);
                ((char *)*payloadp)[ntohs(hdr->size)] = 0;
        }
        int sz = read(fd, *payloadp, ntohs(hdr->size));
        info("Input Packet read: %s", (char*)*payloadp);
        if(sz != ntohs(hdr->size)){
            debug("payload does not match specified size: hdr->size, numRead: %d, %d" ,
                ntohs(hdr->size),
                sz);
            free(*payloadp);
        }
        info("resceived %d byte data from %d", sz, fd);
    }
    return 0;
}
