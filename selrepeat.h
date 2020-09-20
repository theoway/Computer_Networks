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
#define TIME_OUT_PERIOD 5 //in msec

//For server side
#ifdef SERVER_SEL_REPEAT

/*A mutex lock: 1 for available, 0 for unavailable*/
struct Lock{
    int lock;
};

/*Keeps track of the various events that are happening*/
enum Event{
    timeout,
    frame_ready,
    ack_arrived,
    waiting
};

struct Timer{
    int id;
    clock_t time_added;
};

typedef struct Timer Timer;
typedef enum Event Event;
typedef struct Lock Lock;

struct ReceiverThreadArgStruct{
    int sock_fd;
    struct sockaddr_in* client_address;
    int* control;
    int* frame_arrived;
    Lock* ack_lock;
};


struct TimerThreadArgStruct{
    const int* window_size;
    int* control;
    int* timer_to_stop;
    int* timer_to_set;
    Lock* timed_out_frame_lock;
    int* timed_out_frame;
    Lock* timer_array_lock;
    Timer* timers;
};

typedef struct ReceiverThreadArgStruct ReceiverThreadArgStruct; 
typedef struct TimerThreadArgStruct TimerThreadArgStruct;

/*A clock for timers*/
void* TimerClock(void* args);
/*Starts a timer for a given frame. Creates a thread*/
void startTimer(int ack_expected);
/*Stops the timer for a given frame. Stops the thread with a given id*/
void stopTimer(int ack_received);

/*Loads output buffer by reading from the file to be sent. Keeps track of the position,
where the reading was paused, of the file being read*/
int loadOutputBuffer(int window_size, int* start_loading_from, char** buffer, FILE* file_to_read_from);
/*A thread that only keeps track of the incoming acknowledgements*/
void* receive_incoming_ack_frames(void* arg);
/*checks if the arrived frame fits in the transmission window*/
int is_the_arrived_frame_valid(int ack_expected, int ack_received, int frame_to_be_sent);

/*The protocol starts here!*/
void sendSelRepeatServer(int window_size, FILE* fp,const int* server_sockfd ,const struct sockaddr_in* client_addr);

#endif