#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdbool.h>

#define CLEANUP_MSG_TYPE 3

// Structure to store plane details
typedef struct {
    int plane_id;
    int departure_airport;
    int arrival_airport;
    double total_weight;
    int plane_type; // 0 for cargo, 1 for passenger
    int num_passengers; // Relevant only for passenger planes
} PlaneDetails;

// Structure for message sent between processes
typedef struct {
    long mtype; // message type
    PlaneDetails details; // plane details
} Message;

// Function to prompt for termination input
char prompt_termination() {
    char choice;

    // Prompt the user for termination input
    printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No): ");
    scanf(" %c", &choice);

    return choice;
}

// Function to send termination message to air traffic controller
void send_termination_message(int msgqid) {
    // Create a message
    Message msg;
    msg.mtype = CLEANUP_MSG_TYPE; // Message type for termination
    msg.details.plane_id = -1; // Special value to indicate termination

    // Send the message
    msgsnd(msgqid, &msg, sizeof(Message) - sizeof(long), 0);
}

int main() {
    // Create a single message queue for communication
    key_t key = ftok(".", 'g'); // Generate a unique key for the message queue
    int msgqid = msgget(key, 0666 | IPC_CREAT); // Create a message queue with read-write permissions

    // Main loop to handle termination input
    while (true) {
        // Prompt for termination input
        char choice = prompt_termination();

        if (choice == 'Y' || choice == 'y') {
            // Send termination message to air traffic controller
            send_termination_message(msgqid);
            break;
        } else if (choice == 'N' || choice == 'n') {
            // Continue running
            printf("Continuing...\n");
        } else {
            // Invalid input, prompt again
            printf("Invalid input. Please enter Y for Yes or N for No.\n");
        }
    }

    return 0;
}
