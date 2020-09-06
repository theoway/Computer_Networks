#define SERVER_SEL_REPEAT
#include "selrepeat.h"

void sendSelRepeatServer(int window_size, FILE* fp,const int* server_sockfd ,const struct sockaddr_in* client_addr){
    //Initialisations
    int file_trransfer_complete = 0;
    int network_layer_disabled = 1;
    const int max_seq_number = 2 * window_size - 1;
    int lower_edge = 0;
    int upper_edge = lower_edge + window_size - 1;
    int next_frame_to_send = 0;
    int nbuffered = 0;
    char* buffer[window_size];

    int* timed_out = (int*)malloc(sizeof(int*) * max_seq_number);

    Lock arrived_frames_array_lock = {1};
    int* arrived_frames = (int*)malloc(sizeof(int*) * max_seq_number);

    //Create a thread for receiving frames

    //Beginning of the protocol
    Event event = frame_ready;
    network_layer_disabled = 0;
    while(1){
        //Setting event
        if(event == waiting){
            //Checking if any frames have arrived
            if(has_frame_arrived(arrived_frames))
                event = ack_arrived;
            else if(has_frame_timedout(timed_out))
                event = timeout;
            else if(!network_layer_disabled)
                event = frame_ready;
            else
                event = waiting;
        }

        switch(event){
            case frame_ready:{
                nbuffered++;
                loadOutputBuffer(buffer);

                //send using udp socket
                int len = sizeof(client_addr);
                sendto(server_sockfd, (const char *)buffer[next_frame_to_send % window_size], strlen(buffer),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len); 

                next_frame_to_send++;
                break;
            }
            case ack_arrived:{
                //Iterate over arrived_frame_array. stop the timers for arrived frames, first check if they are valid
                break;
            }
            case timeout:{
                //Iterate over timedout_frames and the send the timedout ones. 
                break;
            }
        }
    }
}