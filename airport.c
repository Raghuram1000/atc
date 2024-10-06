#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/msg.h>
#include <stdbool.h>

#define MAX_RUNWAYS 10
#define BACKUP_RUNWAY_LOAD_CAPACITY 15000
#define ATC_RCV_MSG_TYPE 4
#define ATC_SND_MSG_TYPE 5

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

// Structure to represent a runway
typedef struct {
    int runway_id;
    double load_capacity;
    bool is_available;
    pthread_mutex_t lock;
} Runway;

// Structure to hold thread function arguments
typedef struct {
    Message *msg;
    Runway *runways;
    int num_runways;
    int airport_num;
    int msgqid;
} ThreadArgs;

// Function to initialize the airport
void initialize_airport(int airport_num, int num_runways, Runway *runways) {
    // Prompt the user to enter the load capacity for each runway
    printf("Enter loadCapacity of Runways for Airport %d (give as a space separated list in a single line): ", airport_num);
    for (int i = 0; i < num_runways; i++) {
        scanf("%lf", &runways[i].load_capacity);
        
        runways[i].is_available = true;
        pthread_mutex_init(&runways[i].lock, NULL);
    }
}

// Function to select a runway based on best-fit logic
int select_runway(Runway *runways, int num_runways, double total_weight) {
    int best_fit_runway = -1;
    double min_difference = __INT_MAX__;

    // Find the runway with load capacity closest to the total weight
    for (int i = 0; i < num_runways; ++i) {
        pthread_mutex_lock(&runways[i].lock);
        if (runways[i].is_available) {
            double difference = runways[i].load_capacity - total_weight;
            if (difference >= 0 && difference < min_difference) {
                min_difference = difference;
                best_fit_runway = i;
            }
        }
        pthread_mutex_unlock(&runways[i].lock);
    }

    // If no runway found, use backup runway
    if (best_fit_runway == -1) {
        return num_runways;
    }

    return best_fit_runway;
}

// Function to simulate boarding/loading process
void simulate_boarding_loading(double duration) {
    printf("Boarding/loading for %.0f seconds...\n", duration);
    sleep(duration);
}

// Function to simulate deboarding/unloading process
void simulate_deboarding_unloading(double duration) {
    printf("Deboarding/unloading for %.0f seconds...\n", duration);
    sleep(duration);
}

// Function to create a single message queue for communication
int create_message_queue() {
    // Generate a unique key for the message queue
    key_t key = ftok(".", 'g');

    // Create a message queue with read-write permissions
    int msgqid = msgget(key, 0666 | IPC_CREAT);

    return msgqid;
}

// Function to handle plane departure
void* handle_departure(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs*) args;
    Message *msg = threadArgs->msg;
    PlaneDetails plane = msg->details;
    Runway *runways = threadArgs->runways;
    int num_runways = threadArgs->num_runways;
    int msgqid = threadArgs->msgqid;
    int airport_num = threadArgs->airport_num;

    // Find the best-fit runway for departure
    int selected_runway = select_runway(runways, num_runways, plane.total_weight);
    if (selected_runway == -1) {
        printf("No runway available for plane %d departure from Airport %d\n", plane.plane_id, plane.departure_airport);
        return NULL;
    }

    // Lock the selected runway
    pthread_mutex_lock(&runways[selected_runway].lock);
    runways[selected_runway].is_available = false;

    // Simulate boarding/loading process
    simulate_boarding_loading(3);

    // Simulate takeoff process
    sleep(2);

    // Send message to air traffic controller
    Message departure_msg;
    //departure_msg.mtype = ATC_SND_MSG_TYPE;
    departure_msg.mtype = airport_num+30;
    departure_msg.details = plane;
    msgsnd(msgqid, &departure_msg, sizeof(Message) - sizeof(long), 0);

    // Print departure message
    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d\n", plane.plane_id, selected_runway + 1, plane.departure_airport);

    // Unlock the selected runway
    runways[selected_runway].is_available = true;
    pthread_mutex_unlock(&runways[selected_runway].lock);

    return NULL;
}

// Function to handle plane arrival
void* handle_arrival(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs*) args;
    Message *msg = threadArgs->msg;
    PlaneDetails plane = msg->details;
    Runway *runways = threadArgs->runways;
    int num_runways = threadArgs->num_runways;
    int msgqid = threadArgs->msgqid;
    int airport_num = threadArgs->airport_num;

    // Find the best-fit runway for arrival
    int selected_runway = select_runway(runways, num_runways, plane.total_weight);
    if (selected_runway == -1) {
        printf("No runway available for plane %d arrival at Airport %d\n", plane.plane_id, plane.arrival_airport);
        return NULL;
    }

    // Lock the selected runway
    pthread_mutex_lock(&runways[selected_runway].lock);
    runways[selected_runway].is_available = false;

    // Simulate landing process
    sleep(2);

    // Simulate deboarding/unloading process
    simulate_deboarding_unloading(3);

    // Send message to air traffic controller
    Message arrival_msg;
    //arrival_msg.mtype = ATC_SND_MSG_TYPE;
    arrival_msg.mtype = airport_num+30;
    arrival_msg.details = plane;
    msgsnd(msgqid, &arrival_msg, sizeof(Message) - sizeof(long), 0);

    // Print arrival message
    printf("Plane %d has landed on Runway No. %d of Airport No. %d and has completed deboarding/unloading\n", plane.plane_id, selected_runway + 1, plane.arrival_airport);

    // Unlock the selected runway
    runways[selected_runway].is_available = true;
    pthread_mutex_unlock(&runways[selected_runway].lock);

    return NULL;
}

int main() {
    int airport_num;
    int num_runways;

    // Prompt the user to enter the airport number
    printf("Enter Airport Number: ");
    scanf("%d", &airport_num);

    // Prompt the user to enter the number of runways
    printf("Enter number of Runways: ");
    scanf("%d", &num_runways);

    // Initialize the airport
    Runway runways[num_runways + 1]; 
     // +1 for backup runway
    initialize_airport(airport_num, num_runways, runways);

    // Create a single message queue for communication
    int msgqid = create_message_queue();

    // Main loop to handle incoming messages
    while (true) {
        // Declare a buffer for receiving messages
        Message msg;

        // Receive a message from the air traffic controller
        //msgrcv(msgqid, &msg, sizeof(Message) - sizeof(long), ATC_RCV_MSG_TYPE, 0);
        msgrcv(msgqid, &msg, sizeof(Message) - sizeof(long), airport_num + 20, 0);

        // Create a new thread to handle the arrival or departure of the plane
        

        if (msg.details.arrival_airport == airport_num) {
            pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr); // Initialize thread attributes
         
        ThreadArgs threadArgs;
        threadArgs.msg = &msg;
        threadArgs.runways = runways;
        threadArgs.num_runways = num_runways;
        threadArgs.msgqid = msgqid;
        threadArgs.airport_num = airport_num;
            pthread_create(&tid, &attr, handle_arrival, (void*) &threadArgs);
            // Wait for the thread to finish before proceeding to the next iteration
            pthread_join(tid, NULL);
        } else if (msg.details.departure_airport == airport_num) {
           pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr); // Initialize thread attributes
         
        ThreadArgs threadArgs;
        threadArgs.msg = &msg;
        threadArgs.runways = runways;
        threadArgs.num_runways = num_runways;
        threadArgs.msgqid = msgqid;
        threadArgs.airport_num = airport_num;
            pthread_create(&tid, &attr, handle_departure, (void*) &threadArgs);
            // Wait for the thread to finish before proceeding to the next iteration
            pthread_join(tid, NULL);
        }

        
    }

    return 0;
}

