#include <string.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include "tcp.h"
#include "network_sim.h"

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define min(a, b) ((a) < (b) ? (a) : (b))

#define K 4           // multiplier for RTTVAR
#define ALPHA 0.125   // alpha = 1/8 for SRTT update specified in RFC
#define BETA 0.25     // beta = 1/4 for RTTVAR update specified in RFC
#define MIN_RTO 1     // minimum RTO in seconds
#define MAX_RTO 60    // maximum RTO in seconds

// Maximum amount of data that can be sent in one TCP segment.
const size_t MAX_LENGTH = 128;
// Maximum number of unacknowledged segments to send.
const size_t WINDOW_SIZE = 6;
typedef struct {
    struct timespec send_time;
    size_t seq_num;
} rtt_sample_t; //struct type for RTT measurements

//helper function to help calculate precise time difference
double calculate_elapsed_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}
void tcp_send(char* data, size_t length) {
    size_t seq_num = 0; // Sequence number of current packet
    size_t base = 0;    // Base of the sliding window
    size_t next_seq = 0; // Next sequence number to send

    size_t window = WINDOW_SIZE * MAX_LENGTH; // max bytes in the sliding window

    double SRTT = -1, RTTVAR = -1, RTO = 1.0; // Initial RTO 
    rtt_sample_t rtt_sample; //store RTT sample

    struct timespec last_ack_time;   // track last time receiving an ACK 
    clock_gettime(CLOCK_REALTIME, &last_ack_time);

    // split data into packets and send them
    while (base <length) {
        // Send bytes of each packet within the window
        while (next_seq < base + window && next_seq < length) {
            // Prepare packet data
            size_t packet_length = (next_seq + MAX_LENGTH > length) ? length - next_seq : MAX_LENGTH;
            packet_t p;
            p.data = malloc(packet_length);
            if (!p.data) {
                fprintf(stderr, "Memory allocation failed!\n");
                exit(1);
            }
            // Copy packet data into the sliding window
            strncpy(p.data, data + next_seq, packet_length);
            p.length = packet_length;
            p.seq = next_seq;
            // for debugging
            printf("Allocating memory for p.data: %p\n", p.data);
            // Send packet
            ip_send(p); 
            printf("After ip_send, p.data: %p\n", p.data);
            // save send time for RTT calculation
            if (next_seq == base) { // Track the first unacknowledged packet
                clock_gettime(CLOCK_REALTIME, &rtt_sample.send_time);
                rtt_sample.seq_num = next_seq;
            }
            next_seq += packet_length; // increment next sequence number 
        }

        // Wait for ACKs
        int ack;
        while ((ack = ip_recv()) != -1) {
            // printf("%d\n", ack);
            if (ack >= base) { 
                base += MAX_LENGTH; // Update base to the last acknowledged byte
                printf("ACK received for seq %d. Base updated to %zu\n", ack, base);
                clock_gettime(CLOCK_REALTIME, &last_ack_time);

                // if get an ACK for the first unacked packet, update RTT
                if (ack > rtt_sample.seq_num) {
                    struct timespec ack_time;
                    clock_gettime(CLOCK_REALTIME, &ack_time);
                    double RTT = calculate_elapsed_time(rtt_sample.send_time, ack_time);
                
                    if (SRTT < 0) { // First RTT measurement
                        SRTT = RTT;
                        RTTVAR = RTT / 2;
                    } else { // Update SRTT and RTTVAR
                        RTTVAR = (1 - BETA) * RTTVAR + BETA * fabs(SRTT - RTT);
                        SRTT = (1 - ALPHA) * SRTT + ALPHA * RTT;
                    }
                    // update RTO
                    RTO = SRTT + max(1.0, K * RTTVAR);
                    RTO = max(MIN_RTO, RTO); // ensure RTO >= MIN_RTO
                    RTO = min(MAX_RTO, RTO); // ensure RTO <= MAX_RTO
                    //printf("Current timeout (RTO): %.6f seconds\n", RTO); //debug
                    //base = ack; // update base to the last acknowledged byte         
                    //clock_gettime(CLOCK_REALTIME, &last_ack_time); // reset timer 
                }
            }
        }
        // timeouts case --> resend
        struct timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        double elapsed_time = calculate_elapsed_time(last_ack_time, current_time);
        //printf("Current timeout (RTO): %.6f seconds, Elapsed time: %.6f seconds\n", RTO, elapsed_time); //debug
        if (elapsed_time > RTO) {
            printf("Timeout Timeout Timeout !!! Resending packets...\n");
            size_t resend_seq = base;

            // Resend all unacked packets
            while (resend_seq < next_seq) {
                size_t packet_length = (resend_seq + MAX_LENGTH > length) ? length - resend_seq : MAX_LENGTH;
                packet_t p;
                p.data = malloc(packet_length);
                if (!p.data) {
                    fprintf(stderr, "Memory allocation failed!\n");
                    exit(1);
                }
                strncpy(p.data, data + resend_seq, packet_length);
                p.length = packet_length;
                p.seq = resend_seq;

                ip_send(p);

                // Move the sequence forward
                resend_seq += packet_length;
            }
            // Reset  timer
            clock_gettime(CLOCK_REALTIME, &last_ack_time);
        }
    }
}
