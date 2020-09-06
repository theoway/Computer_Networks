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

//For server side
#ifdef SERVER_SEL_REPEAT

struct Lock{
    int lock;
};

enum Event{
    timeout,
    frame_ready,
    ack_arrived,
    waiting
};

typedef enum Event Event;
typedef struct Lock Lock;

void startTimer(int ack_expected);
void stopTimer(int ack_received);

void loadOutputBuffer(int* start_loading_from);
int has_frame_arrived(int* arrived_frames);
int has_frame_timedout(int* timedout_frame_array);

int is_the_arrived_frame_valid(int ack_expected, int ack_received, int frame_to_be_sent);

void sendSelRepeatServer(int window_size, FILE* fp,const int* server_sockfd ,const struct sockaddr_in* client_addr);

#endif