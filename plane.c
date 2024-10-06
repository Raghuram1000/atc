#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdbool.h>

#define MAX_PASSENGERS 10
#define MAX_WEIGHT 100
#define MIN_WEIGHT 10
#define NUM_CREW_MEMBERS 7
#define NUM_CREW_MEMBERS_CARGO 2
#define AVG_CREW_WEIGHT 75
#define MAX_AIRPORT_NUM 10
#define MIN_AIRPORT_NUM 1


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

// Function to initialize the plane
PlaneDetails initialize_plane() {
    PlaneDetails details;
    
    // Prompt the user to enter the type of plane
    printf("Enter Plane ID (1 to 10): ");
    scanf("%d", &details.plane_id);

    // Prompt the user to enter the type of plane
    printf("Enter Type of Plane (1 for Passenger, 0 for Cargo): ");
    scanf("%d", &details.plane_type);

    // Validate the plane type input
    while (details.plane_type != 0 && details.plane_type != 1) {
        printf("Invalid input. Please enter 1 for Passenger or 0 for Cargo: ");
        scanf("%d", &details.plane_type);
    }

    // Display the assigned plane type
    if (details.plane_type == 1) {
        printf("Assigned Plane Type: Passenger\n");
        // If the plane is of passenger type, prompt for the number of occupied seats
        printf("Enter Number of Passengers (1 to 10): ");
        scanf("%d", &details.num_passengers);

        // Validate the number of passengers
        while (details.num_passengers < 1 || details.num_passengers > 10) {
            printf("Invalid input. Please enter a number between 1 and 10: ");
            scanf("%d", &details.num_passengers);
        }

        // Display the number of passengers
        printf("Number of Passengers: %d\n", details.num_passengers);
    } else {
        printf("Assigned Plane Type: Cargo\n");
        // For cargo planes, set the number of passengers to 0
        details.num_passengers = 0;
    }

    // Prompt for departure airport number
    printf("Enter Airport Number for Departure (1 to 10): ");
    scanf("%d", &details.departure_airport);

    // Validate the departure airport number
    while (details.departure_airport < MIN_AIRPORT_NUM || details.departure_airport > MAX_AIRPORT_NUM) {
        printf("Invalid input. Please enter a number between 1 and 10: ");
        scanf("%d", &details.departure_airport);
    }

    // Prompt for arrival airport number
    printf("Enter Airport Number for Arrival (1 to 10): ");
    scanf("%d", &details.arrival_airport);

    // Validate the arrival airport number
    while (details.arrival_airport < MIN_AIRPORT_NUM || details.arrival_airport > MAX_AIRPORT_NUM || details.arrival_airport == details.departure_airport) {
        printf("Invalid input. Please enter a number between 1 and 10 (different from departure): ");
        scanf("%d", &details.arrival_airport);
    }

    // Add code to initialize the plane here
    printf("Initializing plane...\n");

    // Return the details of the plane
    return details;
}

// Function to create passenger processes and get total passenger weight
double create_passenger_processes(int num_passengers, int pipefd[num_passengers][2]) {
    int total_passenger_weight = 0;

    // Loop to create each passenger process
    for (int i = 0; i < num_passengers; ++i) {
        if (pipe(pipefd[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork(); // Fork a new process for each passenger
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process (passenger process)
            close(pipefd[i][0]); // Close read end of pipe

            // Call the passenger logic function
            // Prompt the user to enter the passenger weight
    	    int passenger_weight;
	    printf("Enter Your Weight: ");
    	    scanf("%d", &passenger_weight);

    	    // Validate the passenger weight input
    	    while (passenger_weight < MIN_WEIGHT || passenger_weight > MAX_WEIGHT) {
        	printf("Invalid input. Please enter a number between %d and %d: ", 		    MIN_WEIGHT, MAX_WEIGHT);
            scanf("%d", &passenger_weight);
            }

            // Communicate the passenger weight to the plane process (parent) using a pipe
    	    if (write(pipefd[i][1], &passenger_weight, sizeof(passenger_weight)) == -1) {
        	perror("write");
        	exit(EXIT_FAILURE);
    	    }

    	    // Close write end of pipe
            close(pipefd[i][1]);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process (plane process)
            close(pipefd[i][1]); // Close write end of pipe

            // Read passenger weight from pipe
            int passenger_weight;
            if (read(pipefd[i][0], &passenger_weight, sizeof(passenger_weight)) == -1) {
                perror("read");
                exit(EXIT_FAILURE);
            }
            total_passenger_weight += passenger_weight;

            // Close read end of pipe
            close(pipefd[i][0]);
        }
    }

    return total_passenger_weight;
}

// Function to handle passenger logic
void passenger_logic(int writefd) {
    // Prompt the user to enter the passenger weight
    int passenger_weight;
    printf("Enter Your Weight: ");
    scanf("%d", &passenger_weight);

    // Validate the passenger weight input
    while (passenger_weight < MIN_WEIGHT || passenger_weight > MAX_WEIGHT) {
        printf("Invalid input. Please enter a number between %d and %d: ", MIN_WEIGHT, MAX_WEIGHT);
        scanf("%d", &passenger_weight);
    }

    // Communicate the passenger weight to the plane process (parent) using a pipe
    if (write(writefd, &passenger_weight, sizeof(passenger_weight)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Close write end of pipe
    close(writefd);
}

// Function to calculate the total weight of a passenger plane
double calculate_total_weight_passenger(int num_passengers, double total_passenger_weight) {
    // Calculate total crew weight
    int total_crew_weight = NUM_CREW_MEMBERS * AVG_CREW_WEIGHT;

    // Calculate total weight of the plane
    double total_weight = total_crew_weight + total_passenger_weight;

    return total_weight;
}

// Function to calculate the total weight of a cargo plane
double calculate_total_weight_cargo(int num_cargo_items, int avg_cargo_weight) {
    // Calculate total crew weight
    int total_crew_weight = NUM_CREW_MEMBERS_CARGO * AVG_CREW_WEIGHT;

    // Calculate total weight of the plane
    double total_weight = num_cargo_items * avg_cargo_weight + total_crew_weight;

    return total_weight;
}

// Function to send plane details to air traffic controller
void send_plane_details(int msgqid, PlaneDetails details) {
    // Create a message
    Message msg;
    msg.mtype = details.plane_id; // Message type 1 for plane details
    msg.details = details;

    // Send the message
    msgsnd(msgqid, &msg, sizeof(Message) - sizeof(long), 0);
}

// Function to receive confirmation from air traffic controller
void receive_confirmation(int msgqid, PlaneDetails details) {
    // Declare a buffer for receiving messages
    Message msg;

    // Receive a message with type 2 (confirmation)
    msgrcv(msgqid, &msg, sizeof(Message) - sizeof(long), details.plane_id+10, 0);

    // Print the final message
    printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n", msg.details.plane_id, msg.details.departure_airport, msg.details.arrival_airport);
}

int main() {
    // Initialize the plane
    PlaneDetails details = initialize_plane();

    // Create pipes for communication with passenger processes
    int pipefd[details.num_passengers][2];

    // Get total passenger weight and establish communication pipes
    double total_passenger_weight = create_passenger_processes(details.num_passengers, pipefd);
    
        // Perform operations based on plane type
    if (details.plane_type == 0) {
        // If the plane is of cargo type, prompt for cargo details
        // Prompt for the number of cargo items
        int num_cargo_items;
        printf("Enter Number of Cargo Items (1 to 100): ");
        scanf("%d", &num_cargo_items);

        // Validate the number of cargo items
        while (num_cargo_items < 1 || num_cargo_items > 100) {
            printf("Invalid input. Please enter a number between 1 and 100: ");
            scanf("%d", &num_cargo_items);
        }

        // Prompt for the average weight of cargo items
        int avg_cargo_weight;
        printf("Enter Average Weight of Cargo Items (1 to 100): ");
        scanf("%d", &avg_cargo_weight);

        // Validate the average weight of cargo items
        while (avg_cargo_weight < 1 || avg_cargo_weight > 100) {
            printf("Invalid input. Please enter a number between 1 and 100: ");
            scanf("%d", &avg_cargo_weight);
        }

        // Calculate total weight of the cargo plane
        details.total_weight = calculate_total_weight_cargo(num_cargo_items, avg_cargo_weight);

        // Display the total weight of the cargo plane
        printf("Total Weight of Cargo Plane: %.2f kgs\n", details.total_weight);
    } else {
        // If the plane is of passenger type, calculate total weight of passenger plane
        details.total_weight = calculate_total_weight_passenger(details.num_passengers, total_passenger_weight);

        // Display the total weight of the passenger plane
        printf("Total Weight of Passenger Plane: %.2f kgs\n", details.total_weight);
    }

    // Create message queue
    key_t key = ftok(".", 'g'); // Generate a unique key for the message queue
    int msgqid = msgget(key, 0666 | IPC_CREAT); // Create a message queue with read-write permissions
    
    
    // Send plane details to air traffic controller
    send_plane_details(msgqid, details);
    
    // Send completion message to air traffic controller
    receive_confirmation(msgqid, details);

    return 0;
}

