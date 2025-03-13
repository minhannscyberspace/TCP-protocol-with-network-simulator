#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tcp.h"
#include "network_sim.h"

int main() {
    printf("Initializing the network...\n");
    network_init(1); // Reliable network

    const char* message = "Hello from Minh Ann Nguyen!";
    printf("Sending the message: \"%s\"\n", message);

    tcp_send((char*)message, strlen(message));

    char* result = network_finalize();

    printf("Received message: \"%s\"\n", result);
    printf("Message verification: %s\n", strcmp(message, result) == 0 ? "SUCCESS" : "FAILURE");

    free(result);

    return 0;
}
