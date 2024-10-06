#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdbool.h>

#define MAX_AIRPORTS 10
#define PLANE_RCV_MSG_TYPE 1
#define PLANE_SND_MSG_TYPE 2
#define CLEANUP_MSG_TYPE 3
#define AIRPORT_SND_MSG_TYPE 4
#define AIRPORT_RCV_MSG_TYPE 5

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

// Function to initialize the air traffic controller
int initialize_air_traffic_controller() {
    int num_airports;

    // Prompt the user to enter the number of airports
    printf("Enter the number of airports to be handled/managed (2 to 10): ");
    scanf("%d", &num_airports);

    // Validate the number of airports
    while (num_airports < 2 || num_airports > 10) {
        printf("Invalid input. Please enter a number between 2 and 10: ");
        scanf("%d", &num_airports);
    }

    return num_airports;
}

// Function to create a single message queue for communication
int create_message_queue() {
    // Generate a unique key for the message queue
    key_t key = ftok(".", 'g');

    // Create a message queue with read-write permissions
    int msgqid = msgget(key, 0666 | IPC_CREAT);

    return msgqid;
}

// Function to handle messages received from planes
void handle_messages(int msgqid, int plane_id) {
    // Declare a buffer for receiving messages
    Message plane_msg;

    // Receive a message with type 1 (plane details)
    msgrcv(msgqid, &plane_msg, sizeof(Message) - sizeof(long), plane_id, 0);

    // Forward the message to the appropriate departure airport
    printf("sent");
    //plane_msg.mtype = AIRPORT_SND_MSG_TYPE;
    plane_msg.mtype = plane_msg.details.departure_airport + 20;
    msgsnd(msgqid, &plane_msg, sizeof(Message) - sizeof(long), 0);
    
    // Declare a buffer for receiving messages
    Message departure_msg;

    // Receive a message with type 5 (departure details)
    //msgrcv(msgqid, &departure_msg, sizeof(Message) - sizeof(long), AIRPORT_RCV_MSG_TYPE, 0);
    msgrcv(msgqid, &departure_msg, sizeof(Message) - sizeof(long), plane_msg.details.departure_airport+30, 0);
    
    printf("Takeoff Message received from departure airport");
    // File pointer
    FILE *file;

    // Open file in write mode ("w")
    file = fopen("output.txt", "w");

    // Write output to the file
    fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d",
            plane_msg.details.plane_id, plane_msg.details.departure_airport, plane_msg.details.arrival_airport);

    // Close the file
    fclose(file);
    
    // Forward the message to the appropriate arrival airport
    //plane_msg.mtype = AIRPORT_SND_MSG_TYPE;
    plane_msg.mtype = plane_msg.details.arrival_airport + 20;
    msgsnd(msgqid, &plane_msg, sizeof(Message) - sizeof(long), 0);
    
    // Declare a buffer for receiving messages
    Message arrival_msg;

    // Receive a message with type 5 (arrival details)
    msgrcv(msgqid, &arrival_msg, sizeof(Message) - sizeof(long), plane_msg.details.arrival_airport+30, 0);
    
    printf("Arrival Message received from arrival airport");

    // Send confirmation message back to the plane
    plane_msg.mtype = plane_id+10;
    msgsnd(msgqid, &plane_msg, sizeof(Message) - sizeof(long), 0);
}

int main() {
    // Initialize the air traffic controller
    int num_airports = initialize_air_traffic_controller();

    // Create a single message queue for communication
    int msgqid = create_message_queue();

    // Main loop to handle incoming messages
    int plane_id = 1;
    while (true) {
   	

        // Receive message from either plane or airport
        handle_messages(msgqid, plane_id);
   
        // Declare a buffer for receiving messages
        Message msg;

        // Receive a message with type 3 (cleanup)
        msgrcv(msgqid, &msg, sizeof(Message) - sizeof(long), CLEANUP_MSG_TYPE, IPC_NOWAIT);
    
        if(msg.details.plane_id == -1){
            msgctl(msgqid,IPC_RMID,NULL);
    	    return 1;
        }
        plane_id++;
        
    }

    return 0;
}

