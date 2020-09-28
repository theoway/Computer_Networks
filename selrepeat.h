//Check for each file to be included
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <time.h>

#define PACKET_SIZE 500
#define DIGITS_FOR_SEQ_NO 1
#define BUFFER_SIZE (PACKET_SIZE - DIGITS_FOR_SEQ_NO)
#define TIME_OUT_PERIOD 100 //in msec
#define ADVANCE_WINDOW "ADV_WIN"
#define END_PROTOCOL "END_PRCL"

/*checks if the arrived frame fits in the transmission/receiving window*/
int is_the_arrived_frame_valid(int ack_expected, int ack_received, int frame_to_be_sent);

//For server side
/*A mutex lock: 1 for available, 0 for unavailable*/
struct Lock{
    int lock;
};

/*Keeps track of the various events that are happening*/
enum Event{
    timeout, //In case a timeout happens
    frame_ready, //When frames are ready to be sent
    ack_arrived, //When an acknowledgement is received
    waiting //When nothing new has happend
};

/*Struct for timers, keeps the id(frame number) and the time at which its timer was started*/
struct Timer{
    int id; //Stores the sequence number of the packet whose timer is to be set
    clock_t time_added; //Stores the clock ticks that have been elapsed between the time it was added and the beginning of the thread
};

typedef struct Timer Timer;
typedef enum Event Event;
typedef struct Lock Lock;

/*A structure for arguements passing to the receiver thread*/
struct ReceiverThreadArgStruct{
    int sock_fd; //Points to socket file descriptor of server
    int* control; //To stop the thread. Stop when- *control = 1
    int* frame_arrived; //Pointer to the variable that stores the latest acknowledgement arrived
    Lock* ack_lock; //Lock for concurrency for- int* frame_arrived
};

/*A structure for arguements passing to the timer thread*/
struct TimerThreadArgStruct{
    const int* window_size;
    int* control; //To stop the thread. Stop when- *control = 1
    int* timer_to_stop; //Pointer to the variable that stores the frame number whose timers is to be stopped
    int* timer_to_set; //Pointer to the variable that stores the frame number whose timers is to be started
    Lock* timed_out_frame_lock; //Lock for concurrency for- int* timed_out_frame  
    int* timed_out_frame; //Pointer to the variable that stores the frame number of oldest timed out frame
    Lock* timer_array_lock; //Lock for concurrency for- Timer* timers  
    Timer* timers; //An array of <struct Timer> that keeps the record of timers added, their sequence numbers and the time at which they were added
};

typedef struct ReceiverThreadArgStruct ReceiverThreadArgStruct; 
typedef struct TimerThreadArgStruct TimerThreadArgStruct;

/*A clock for timers. The clock keeps ticking till the protocol ends.*/
void* TimerClock(void* args);
/*Starts a timer for a given frame*/
void startTimer(int ack_expected);
/*Stops the timer for a given frame*/
void stopTimer(int ack_received);

/*Loads output buffer by reading from the file to be sent. Keeps track of the position,
where the reading was paused, of the file being read*/
int loadOutputBuffer(int* file_transfer_status, int window_size, int* start_loading_from, char** buffer, FILE* file_to_read_from);

/*A thread that only keeps track of the incoming acknowledgements. Stores the latest arrived acknowledgement*/
void* receive_incoming_ack_frames(void* arg);

/*The protocol for server starts here!*/
//Note: buffer_size cannot be less than PACKET_SIZE
void SelRepeatServer(int buffer_size, FILE* fp, const int* server_sockfd ,struct sockaddr_in client_addr);


/*The protocol for client starts here!*/
void SelRepeatReceiver(int buffer_size, int sock_fd, struct sockaddr_in server_addr, FILE* file_to_write);

