#define SERVER_SEL_REPEAT
#include "selrepeat.h"

int is_the_arrived_frame_valid(int lowerEdge, int ack_received, int upperEdge){
    return ((lowerEdge <= ack_received) && (ack_received < upperEdge)) && ((upperEdge < lowerEdge) && (lowerEdge <= ack_received) && ((ack_received < upperEdge) && (upperEdge < lowerEdge)));
}

void* receive_incoming_ack_frames(void* arg){

    char buffer[DIGITS_FOR_SEQ_NO + 1];
    ReceiverThreadArgStruct* data_for_receiving = (ReceiverThreadArgStruct*)arg;
    int sock_fd = data_for_receiving->sock_fd;
    struct sockaddr_in* client_addr = data_for_receiving->client_address;
    
    int len, n; 
    len = sizeof(*client_addr);
    while(!data_for_receiving->control){
        n = recvfrom(sock_fd, (char *)buffer, DIGITS_FOR_SEQ_NO,  MSG_CONFIRM, ( struct sockaddr *) &client_addr, &len);
        //Set remaining places to \0
        int i = 0;
        for(i = n; i <= DIGITS_FOR_SEQ_NO; i++){
            buffer[i] = '\0';
        }

        if(buffer[0] != '\0'){
            while(!data_for_receiving->ack_lock->lock){
                //Wait for the lock to be available
            }

            data_for_receiving->ack_lock->lock = 0;
            *(data_for_receiving->frame_arrived) = atoi(buffer);
            printf("Ack received: %d", *(data_for_receiving->frame_arrived));
            data_for_receiving->ack_lock->lock = 1;
        }
    }
}

void* TimerClock(void* args){
    TimerThreadArgStruct* data_for_timers = (TimerThreadArgStruct*)args;
    while(!*(data_for_timers->control)){
        clock_t current_time = clock();
        //Set & stop given timers
        int i;
        for(i = 0; i < data_for_timers->window_size; i++){
            if(data_for_timers->timers[i].id == data_for_timers->timer_to_set && 
            data_for_timers->timers[i].time_added == 0)
                //See for lock here
                data_for_timers->timers[i].time_added = current_time;
            
            if(data_for_timers->timers[i].id == data_for_timers->timer_to_stop &&
            data_for_timers->timers[i].time_added != -1)
                //See for lock here
                data_for_timers->timers[i].time_added = 0;
        }

        //Check for timed_out timers
        for(i = 0; i < data_for_timers->window_size; i++){
            if(!data_for_timers->timers[i].time_added){
                int time_elapsed = (current_time - data_for_timers->timers[i].time_added) * 1000 / CLOCKS_PER_SEC;
                if(time_elapsed > TIME_OUT_PERIOD){
                    //Set oldest frame timedout equal to this, with lock settings
                    //Set data_for_timers->timers[i].time_added = 0, with lock settings
                }
            }
        }
    }
}

void loadOutputBuffer(int window_size, int* start_loading_from, char** buffer, FILE* file_to_read_from){
    fseek(file_to_read_from, *start_loading_from, SEEK_SET);
    char c = fgetc(file_to_read_from);
    int i = 0, j = 0;
    while(c != EOF){
        if(i == window_size)
            break;
        buffer[i][j] = c;
        c = fgetc(file_to_read_from);
        j++;
        *start_loading_from++;
        if(j == BUFFER_SIZE)
            i++;
    }
}

void sendSelRepeatServer(int buffer_size, FILE* fp,const int* server_sockfd ,const struct sockaddr_in* client_addr){
    //Initialisations
    int window_size = (buffer_size % PACKET_SIZE) == 0 ? (buffer_size / PACKET_SIZE) : (buffer_size / PACKET_SIZE) + 1;
    int file_trransfer_complete = 0;
    int network_layer_disabled = 1;
    const int max_seq_number = 2 * window_size - 1;
    int lower_edge = 0;
    int upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
    int next_frame_to_send = lower_edge;
    int nbuffered = 0;
    int ack_received = 0;

    char* buffer[window_size];
    //Allocate each buffer the size of packet
    int i = 0;
    for(; i < window_size; i++){
        buffer[i] = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    }
    int read_buffer_from = 0;
    loadOutputBuffer(window_size, &read_buffer_from, buffer, fp);

    Lock timer_array_lock = {1};
    Timer timers_for_sent_frames[window_size];
    /*Intialising timers*/
    for(i = 0; i < window_size; i++){
        timers_for_sent_frames[i].id = (lower_edge + i) % (max_seq_number + 1);
        timers_for_sent_frames[i].time_added = 0;
    }
    Lock timed_out_frame_lock = {1};
    int oldest_timedout_frame = -1;
    int timer_to_set = -1;
    int timer_to_stop = -1;

    int stop_timer_thread = 0;
    TimerThreadArgStruct timer_thread_args = {&window_size, &stop_timer_thread, &timer_to_stop, &timer_to_set, &timed_out_frame_lock, &oldest_timedout_frame, &timer_array_lock, timers_for_sent_frames};
    pthread_t timer_thread_id; 
    pthread_create(&timer_thread_id, NULL, TimerClock, (void*)&timer_thread_args); 

    Lock ack_lock = {1};
    int latest_ack_arrived = -1;
    //Create a thread for receiving frames
    int stop_receiver_thread = 0;
    struct sockaddr_in client_address_rec_thread = *client_addr;
    ReceiverThreadArgStruct thread_args = {*server_sockfd, &client_address_rec_thread, &stop_receiver_thread, &latest_ack_arrived, &ack_lock};
    pthread_t receiver_thread_id; 
    pthread_create(&receiver_thread_id, NULL, receive_incoming_ack_frames, (void*)&thread_args); 

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
            network_layer_disabled = 0;
            lower_edge = (upper_edge + 1) % (max_seq_number + 1);
            upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
            next_frame_to_send = lower_edge;

            for(i = 0; i < window_size; i++){
                timers_for_sent_frames[i].id = (lower_edge + i) % (max_seq_number + 1);
                timers_for_sent_frames[i].time_added = 0;
            }

            if(file_trransfer_complete){
                //Protocol stops
            }
        }
    }
}
