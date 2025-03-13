#pragma once

#include <stdlib.h>
#include <stddef.h> 

typedef struct {
    char* data;    // The data in this segment
    size_t length; // The length of the data array
    int seq;       // The sequence number of the first byte
} packet;

typedef struct {
    char* data;
    size_t length;
    int seq;
    int is_dummy; // 1 if dummy packet, 0 otherwise
} packet_t;
// Initialize the network simulator. The network can be set up to force in-order
// and/or reliable delivery (or neither). If the network is not reliable then
// packets that are sent may never be received. If it is not in-order then
// packets may be received in a different order than they were sent.
void network_init(int reliable);
void deliver_to_receiver();

// Clean up the network, freeing all associated resources. This returns a string
// which is the concatenation of the information in all packets received by the
// network.
char* network_finalize();

// Send a packet over the network.
void ip_send(packet_t);

// Check for a response from the network. This function will check whether any
// ACKs have been sent and, if one has, will return the oldest ACK. Note that
// this function does _not_ block waiting for a message. If the network has not
// sent any response, this function returns immediately with -1.
int ip_recv();
