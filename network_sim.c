#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 
#include <pthread.h>
#include <stdio.h>


#include "network_sim.h"
// Changes in packet_t: now packet_t have a is_dummy flag to determine if a packet is a dummy packet, this helps me implement the
// mechanism of injecting dummy packet and how dummy packet works easily

#define MAX_ROUTERS 5             // Number of routers in this network
#define ROUTER_BUFFER_SIZE 4      // Max buffer size for each router
#define DUMMY_PACKET_PROB 0.2     // Probability of injecting dummy packets
#define FAILURE_PROB 0.05         // Probability of packet loss in unreliable network

typedef struct ack_node {
    int seq;                 // Sequence number of the ACK
    struct ack_node* next;   // Pointer to the next ACK
} ack_node;

typedef struct {
    packet_t buffer[ROUTER_BUFFER_SIZE];
    int head;       // Front of the buffer
    int tail;       // Next insertion point
    int size;       // Number of packets currently in buffer
    pthread_mutex_t lock; // Mutex for thread safety
} Router;

Router routers[MAX_ROUTERS];      // Array of routers

struct {
    int reliable;
    char* msg;
    size_t msg_len;
    int last_ack;
    ack_node* ack_head;        // Head of ACK queue
    ack_node* ack_tail;        // Tail of ACK queue
    pthread_mutex_t ack_mutex; // Mutex for ACK queue
} globals;

void init_routers() {
    for (int i = 0; i < MAX_ROUTERS; i++) {
        routers[i].head = routers[i].tail = routers[i].size = 0;
        pthread_mutex_init(&routers[i].lock, NULL);
    }
}

int is_router_full(Router* router) {
    return router->size == ROUTER_BUFFER_SIZE;
}

void inject_dummy_packet(Router* router) {
    if ((rand() % 10) < (DUMMY_PACKET_PROB * 10)) {
        packet_t dummy_packet;
        dummy_packet.data = strdup("DUMMY");
        dummy_packet.length = 5;
        dummy_packet.seq = -1;  // Dummy packets have no valid sequence
        dummy_packet.is_dummy = 1;

        pthread_mutex_lock(&router->lock);
        if (!is_router_full(router)) {
            router->buffer[router->tail] = dummy_packet;
            router->tail = (router->tail + 1) % ROUTER_BUFFER_SIZE;
            router->size++;
            printf("Injected dummy packet into router buffer\n");
        } else {
            printf("Dummy packet dropped due to buffer overflow\n");
            free(dummy_packet.data);
        }
        pthread_mutex_unlock(&router->lock);
    }
}

//when forwarding, router scans its buffer for the first real packet (packet where (is_dummy == 0)) and forward real packet only
//Dummy packets are skipped and remain in the buffer to occupy space.
void forward_packet(int from, int to) {
    Router* src = &routers[from];
    Router* dest = &routers[to];

    pthread_mutex_lock(&src->lock);

    if (src->size > 0) {
        packet_t packet;
        int packet_found = 0;

        // Search the buffer for the first real packet
        for (int i = 0; i < src->size; i++) {
            int index = (src->head + i) % ROUTER_BUFFER_SIZE;

            if (!src->buffer[index].is_dummy) { // Skip dummy packets
                packet = src->buffer[index];
                packet_found = 1;

                // Remove the real packet from the buffer
                for (int j = i; j < src->size - 1; j++) {
                    src->buffer[(src->head + j) % ROUTER_BUFFER_SIZE] = 
                        src->buffer[(src->head + j + 1) % ROUTER_BUFFER_SIZE];
                }
                src->tail = (src->tail - 1 + ROUTER_BUFFER_SIZE) % ROUTER_BUFFER_SIZE;
                src->size--;
                break;
            }
        }

        pthread_mutex_unlock(&src->lock);

        if (packet_found) {
            usleep(1000);

            pthread_mutex_lock(&dest->lock);
            if (!is_router_full(dest)) {
                dest->buffer[dest->tail] = packet;
                dest->tail = (dest->tail + 1) % ROUTER_BUFFER_SIZE;
                dest->size++;
                printf("Forwarded packet seq %d to router %d\n", packet.seq, to);
            } else {
                printf("Packet seq %d dropped at router %d due to buffer overflow\n", packet.seq, to);
                free(packet.data);
            }
            pthread_mutex_unlock(&dest->lock);
        }
    } else {
        pthread_mutex_unlock(&src->lock);
    }
}


void deliver_to_receiver() {
    Router* final_router = &routers[MAX_ROUTERS - 1];

    pthread_mutex_lock(&final_router->lock);
    while (final_router->size > 0) {
        packet_t packet = final_router->buffer[final_router->head];
        final_router->head = (final_router->head + 1) % ROUTER_BUFFER_SIZE;
        final_router->size--;

        if (!packet.is_dummy) {
            printf("Delivered packet seq %d to receiver\n", packet.seq);

            // create ACK for the delivered packet
            ack_node* new_ack = malloc(sizeof(ack_node));
            new_ack->seq = packet.seq;
            new_ack->next = NULL;

            pthread_mutex_lock(&globals.ack_mutex);
            if (globals.ack_tail) {
                globals.ack_tail->next = new_ack;
            } else {
                globals.ack_head = new_ack;
            }
            globals.ack_tail = new_ack;
            pthread_mutex_unlock(&globals.ack_mutex);

            // Store the packet's data
            globals.msg = realloc(globals.msg, globals.msg_len + packet.length + 1);
            strncpy(globals.msg + globals.msg_len, packet.data, packet.length);
            globals.msg_len += packet.length;
            globals.msg[globals.msg_len] = '\0';
        }

        free(packet.data);
    }
    pthread_mutex_unlock(&final_router->lock);
}

void network_init(int reliable) {
    globals.reliable = reliable;
    globals.msg = malloc(1);
    globals.msg[0] = '\0';
    globals.msg_len = 0;
    globals.last_ack = 0;
    globals.ack_head = NULL;
    globals.ack_tail = NULL;
    pthread_mutex_init(&globals.ack_mutex, NULL);
    srand(time(NULL));
    init_routers();
}


char* network_finalize() {
    pthread_mutex_destroy(&globals.ack_mutex);
    // Free all pending ACK nodes
    while (globals.ack_head != NULL) {
        ack_node* tmp = globals.ack_head;
        globals.ack_head = globals.ack_head->next;
        free(tmp);
    }

    for (int i = 0; i < MAX_ROUTERS; i++) {
        pthread_mutex_destroy(&routers[i].lock);
    }
    return globals.msg;
}


void ip_send(packet_t p) {
    printf("Sending packet seq %d...\n", p.seq);

    if (!globals.reliable && drand48() < FAILURE_PROB) {
        printf("Packet seq %d dropped due to unreliable network\n", p.seq);
        free(p.data);
        return;
    }

    pthread_mutex_lock(&routers[0].lock);
    if (!is_router_full(&routers[0])) {
        routers[0].buffer[routers[0].tail] = (packet_t){p.data, p.length, p.seq, 0};
        routers[0].tail = (routers[0].tail + 1) % ROUTER_BUFFER_SIZE;
        routers[0].size++;
        printf("Packet seq %d added to first router\n", p.seq);
    } else {
        printf("Packet seq %d dropped due to buffer overflow at first router\n", p.seq);
        free(p.data);
    }
    pthread_mutex_unlock(&routers[0].lock);

    for (int i = 0; i < MAX_ROUTERS - 1; i++) {
        inject_dummy_packet(&routers[i]);
        forward_packet(i, i + 1);
    }
    deliver_to_receiver();
}

int ip_recv() {
    int ack = -1;

    pthread_mutex_lock(&globals.ack_mutex);
    if (globals.ack_head != NULL) {
        ack_node* tmp = globals.ack_head;
        ack = tmp->seq; // get the sequence number

        globals.ack_head = globals.ack_head->next;
        if (globals.ack_head == NULL) {
            globals.ack_tail = NULL; // Empty the queue
        }

        free(tmp);
    }
    pthread_mutex_unlock(&globals.ack_mutex);

    return ack; // Return the oldest ACK, or -1 if none
}
