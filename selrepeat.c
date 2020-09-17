#define SERVER_SEL_REPEAT
#include "selrepeat.h"

int is_the_arrived_frame_valid(int lowerEdge, int ack_received, int upperEdge){
    return ((lowerEdge <= ack_received) && (ack_received < upperEdge)) && ((upperEdge < lowerEdge) && (lowerEdge <= ack_received) && ((ack_received < upperEdge) && (upperEdge < lowerEdge)));
}

void* receive_incoming_ack_frames(void* arg){

    char buffer[2];
    ReceiverThreadArgStruct* data_for_receiving = (ReceiverThreadArgStruct*)arg;
    int sock_fd = data_for_receiving->sock_fd;
    struct sockaddr_in* client_addr = data_for_receiving->client_address;
    
    int len, n; 
    len = sizeof(*client_addr);
    while(!data_for_receiving->control){
        n = recvfrom(sock_fd, (char *)buffer, 2,  MSG_CONFIRM, ( struct sockaddr *) &client_addr, &len);
        buffer[n] = '\0';
        if(n >= 2){
            while(!data_for_receiving->ack_lock->lock){
                //Wait for the lock to be available
            }

            data_for_receiving->ack_lock->lock = 0;
            ///Set the received_frame = this
            printf("Ack received: %d");
            data_for_receiving->ack_lock->lock = 1;
        }
    }
}

void loadOutputBuffer(int* start_loading_from, char** buffer){

}

void sendSelRepeatServer(int buffer_size, FILE* fp,const int* server_sockfd ,const struct sockaddr_in* client_addr){
    //Initialisations
    int window_size = (buffer_size % PACKET_SIZE) == 0 ? (buffer_size / PACKET_SIZE) : (buffer_size / PACKET_SIZE) + 1;
    int file_trransfer_complete = 0;
    int network_layer_disabled = 1;
    const int max_seq_number = 2 * window_size - 1;
    int lower_edge = 0;
    int upper_edge = lower_edge + window_size - 1;
    int next_frame_to_send = 0;
    int nbuffered = 0;
    int ack_received = 0;

    char* buffer[window_size];
    //Allocate each buffer the size of packet
    int read_buffer_from = 0;
    loadOutputBuffer(&read_buffer_from, buffer);

    int oldest_timedout_frame = -1;
    Lock ack_lock = {1};

    int latest_ack_arrived = -1;

    //Create a thread for receiving frames
    int stop_receiver_thread = 0;
    struct sockaddr_in client_address_rec_thread = *client_addr;
    ReceiverThreadArgStruct thread_args = {*server_sockfd, &client_address_rec_thread, &stop_receiver_thread, &latest_ack_arrived, &ack_lock};
    pthread_t thread_id; 
    pthread_create(&thread_id, NULL, receive_incoming_ack_frames, (void*)&thread_args); 

    //Beginning of the protocol
    Event event = frame_ready;
    network_layer_disabled = 0;
    while(1){
        //Setting event
        //Check if any valid frames have arrived
        if(is_the_arrived_frame_valid(lower_edge, latest_ack_arrived, upper_edge))
            event = ack_arrived;
        //Check if any valid frame has timedout
        else if(is_the_arrived_frame_valid(lower_edge, oldest_timedout_frame, upper_edge))
            event = timeout;
        //Check if frames are ready to be sent
        else if(!network_layer_disabled)
            event = frame_ready;
        else
            event = waiting;

        switch(event){
            case frame_ready:{
                nbuffered++;
                //send using udp socket
                int len = sizeof(client_addr);
                //sendto(server_sockfd, (const char *)buffer[next_frame_to_send % window_size], strlen(buffer),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len); 

                next_frame_to_send++;
                break;
            }
            case ack_arrived:{
                //Stop thte timer for the arrived ack, increment the ack_counter
                break;
            }
            case timeout:{
                //Send the timedout frame, start the timer again 
                break;
            }
        }

        if(nbuffered == window_size){
            network_layer_disabled = 1;
        }
        if(ack_received == window_size){
            //Load output buffer by incrementing the start point 
            //Enable the network layer
            if(file_trransfer_complete){
                //Protocol stops
            }
        }
    }
}