//Check for each file to be included
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

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

typedef struct Lock Lock;
typedef enum Event Event;

void sendSelRepeatServer(int window_size, FILE* fp, const struct sockaddr_in* client_addr);

#endif