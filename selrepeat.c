#include "selrepeat.h"

//Returns 0 if the frame is not valid, that is, it doesn't fit inside the transmission/receiving window. Else returns 1
int is_the_arrived_frame_valid(int lowerEdge, int ack_received, int upperEdge){
    return ((lowerEdge <= ack_received) && (ack_received <= upperEdge)) || ((upperEdge <= lowerEdge) && (lowerEdge <= ack_received) || ((ack_received <= upperEdge) && (upperEdge < lowerEdge)));
}


void* receive_incoming_ack_frames(void* arg){
    int ascii_0 = (int)'0';

    char buffer[DIGITS_FOR_SEQ_NO + 1];
    ReceiverThreadArgStruct* data_for_receiving = (ReceiverThreadArgStruct*)arg;
    int sock_fd = data_for_receiving->sock_fd;
    struct sockaddr_in client_addr;
    
    int len, n; 
    len = sizeof(client_addr);
    while(!*data_for_receiving->control){
        n = recvfrom(sock_fd, (char *)buffer, DIGITS_FOR_SEQ_NO + 1,  MSG_CONFIRM, ( struct sockaddr *) &client_addr, &len);
        printf("Packet Received\n");
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
            *(data_for_receiving->frame_arrived) = (int)buffer[0] - ascii_0;
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
        for(i = 0; i < *(data_for_timers->window_size); i++){
            if(data_for_timers->timers[i].id == *data_for_timers->timer_to_set && 
            data_for_timers->timers[i].time_added == -1){
                while(!data_for_timers->timer_array_lock->lock){
                    //Wait for lock to be available
                }
                data_for_timers->timer_array_lock->lock = 0;
                data_for_timers->timers[i].time_added = current_time;
                data_for_timers->timer_array_lock->lock = 1;
                printf("Timer set for: %d at %ld\n", data_for_timers->timers[i].id, data_for_timers->timers[i].time_added);
            }
            
            if(data_for_timers->timers[i].id == *data_for_timers->timer_to_stop &&
            data_for_timers->timers[i].time_added != -1){
                while(!data_for_timers->timer_array_lock->lock){
                    //Wait for lock to be available
                }
                printf("Timer stopped for: %d\n", data_for_timers->timers[i].id);
                data_for_timers->timer_array_lock->lock = 0;
                data_for_timers->timers[i].time_added = -1;
                data_for_timers->timer_array_lock->lock = 1;
            }
        }

        //Check for timed_out timers
        int max_elapsed_time = -100;
        int oldest_frame = -1;
        int index = 0;
        for(i = 0; i < *(data_for_timers->window_size); i++){
            if(data_for_timers->timers[i].time_added != -1){
                int time_elapsed = (current_time - data_for_timers->timers[i].time_added) * 1000 / CLOCKS_PER_SEC;
                if(time_elapsed > TIME_OUT_PERIOD){
                    //Set oldest frame timedout equal to this, with lock settings
                    if(time_elapsed > max_elapsed_time){
                        max_elapsed_time = time_elapsed;
                        oldest_frame = data_for_timers->timers[i].id;
                        index = i;
                    }
                }
            }
        }
        if(oldest_frame != -1){

            while(!data_for_timers->timed_out_frame_lock->lock){
                //Wait for lock to be available
            }
            data_for_timers->timed_out_frame_lock->lock = 0;
            *(data_for_timers->timed_out_frame) = oldest_frame;
            data_for_timers->timed_out_frame_lock->lock = 1;
            while(!data_for_timers->timer_array_lock->lock){
                //Wait for lock to be available
            }
            data_for_timers->timer_array_lock->lock = 0;
            data_for_timers->timers[index].time_added = clock();
            data_for_timers->timer_array_lock->lock = 1;
            printf("timer ran out: %d\n",  *(data_for_timers->timed_out_frame));
        }
    }
}

int loadOutputBuffer(int* file_transfer_status, int window_size, int* start_loading_from, char** buffer, FILE* file_to_read_from){
    int ack_ex = 0;
    fseek(file_to_read_from, *start_loading_from, SEEK_SET);
    char c = fgetc(file_to_read_from);
    int i = 0, j = 0;
    while(c != EOF){
        if(j == BUFFER_SIZE - 1){
            buffer[i][j] = '\0';
            j = 0;
            ack_ex++;
            i++;
        }
        if(i == window_size)
            break;
        else{
        buffer[i][j] = c;
        c = fgetc(file_to_read_from);
        j++;
        *(start_loading_from) += 1;
        }
    }
    if(c == EOF){
        printf("\nEOF encountered!\n");
        buffer[i][j] = '\0';
        ack_ex++;
        i++;
        *file_transfer_status = 1;
        for(;i<window_size;i++){
            int j = 0;
            for(; j < BUFFER_SIZE; j++)
                buffer[i][j] = '\0';
        }
    }
    
    return ack_ex;
}

void SelRepeatServer(int buffer_size, FILE* fp,const int* server_sockfd ,struct sockaddr_in client_addr){
    //Initialisations
    int window_size = (buffer_size % PACKET_SIZE) == 0 ? (buffer_size / PACKET_SIZE) : (buffer_size / PACKET_SIZE) + 1;
    printf("Window size: %d\n", window_size);
    int file_transfer_complete = 0;
    int network_layer_disabled = 1;
    const int max_seq_number = 2 * window_size - 1;
    int lower_edge = 0;
    int upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
    int next_frame_to_send = lower_edge;
    int nbuffered = 0;
    int ack_received = 0;
    int ack_expected = 0;

    char* buffer[window_size];
    //Allocate each buffer the size of packet
    int i = 0;
    for(; i < window_size; i++){
        buffer[i] = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    }
    int read_buffer_from = 0;
    ack_expected = loadOutputBuffer(&file_transfer_complete, window_size, &read_buffer_from, buffer, fp);
    
    Lock timer_array_lock = {1};
    Timer timers_for_sent_frames[window_size];
    /*Intialising timers*/
    for(i = 0; i < window_size; i++){
        timers_for_sent_frames[i].id = (lower_edge + i) % (max_seq_number + 1);
        timers_for_sent_frames[i].time_added = -1;
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
    ReceiverThreadArgStruct thread_args = {*server_sockfd, &stop_receiver_thread, &latest_ack_arrived, &ack_lock};
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

                //Constuct the packet
                char packet[BUFFER_SIZE + DIGITS_FOR_SEQ_NO];
                int ascii_0 = (int)'0';
                packet[0] = (char)(ascii_0 + next_frame_to_send);
                int i;
                for(i = 1; i < PACKET_SIZE; i++)
                    packet[i] = buffer[next_frame_to_send % window_size][i-1];
                printf("Packet to be sent: %d\n", next_frame_to_send);
                
                //Sending the packet using UDP sockets
                int len = sizeof(client_addr);
                sendto(*server_sockfd, (const char *)packet, strlen(packet),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len); 
                timer_to_set = next_frame_to_send;
                int index = -1;
                for(i = 0; i < window_size;i++)
                    index = timers_for_sent_frames[i].id  == next_frame_to_send ? i : index;

                while(timers_for_sent_frames[index].time_added == -1){
                    //Wait for the timer to be set
                }

                printf("In frame ready %d %ld\n", timers_for_sent_frames[index].id, timers_for_sent_frames[index].time_added);
                for(i = 0; i< window_size;i++)
                    printf("Frame ReadyEvent: %d %ld\n", timers_for_sent_frames[i].id, timers_for_sent_frames[i].time_added);
            
                next_frame_to_send++;
                break;
            }
            case ack_arrived:{
                //Stop thte timer for the arrived ack, increment the ack_counter
                printf("Ack arrived: %d\n", latest_ack_arrived);
                timer_to_stop = latest_ack_arrived;
                timer_to_set = timer_to_set != timer_to_stop ? timer_to_set : -1;
                ack_received++;
                while(!ack_lock.lock){
                    //Waiting for the lock to be availbale
                }
                ack_lock.lock = 0;
                latest_ack_arrived = -1;
                ack_lock.lock = 1;
                break;
            }
            case timeout:{
                //Send the timedout frame, start the timer again 
                //Construct the packet
                char packet[BUFFER_SIZE + DIGITS_FOR_SEQ_NO];
                int ascii_0 = (int)'0';
                packet[0] = (char)(ascii_0 + oldest_timedout_frame);
                int i;
                for(i = 1; i< BUFFER_SIZE + DIGITS_FOR_SEQ_NO; i++)
                    packet[i] = buffer[oldest_timedout_frame % window_size][i-1];
                int len = sizeof(client_addr);

                for(i = 0; i< window_size;i++)
                    printf("TimeOut Event: %d %ld\n", timers_for_sent_frames[i].id, timers_for_sent_frames[i].time_added);
                sendto(*server_sockfd, (const char *)packet, strlen(packet),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len);

                printf("Timedout packet to be sent: %d\n", oldest_timedout_frame);

                while(!timed_out_frame_lock.lock){
                    //Waiting for the lock to be availbale
                }
                timed_out_frame_lock.lock = 0;
                oldest_timedout_frame = -1;
                timed_out_frame_lock.lock = 1;
                
                break;
            }
            default:{
                //Do Nothing
                break;
            }
        }

        if(nbuffered == ack_expected){
            network_layer_disabled = 1;
        }

        if(ack_received == ack_expected){
            int len = sizeof(client_addr);

            //Load output buffer by incrementing the start point
            ack_expected = loadOutputBuffer(&file_transfer_complete, window_size, &read_buffer_from, buffer, fp);
            //Enable the network layer
            network_layer_disabled = 0;
            lower_edge = (upper_edge + 1) % (max_seq_number + 1);
            upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
            next_frame_to_send = lower_edge;
        
            while(!timer_array_lock.lock){
                //Waiting for the lock to be available
            }
            timer_array_lock.lock = 0;
            for(i = 0; i < window_size; i++){
                timers_for_sent_frames[i].id = (lower_edge + i) % (max_seq_number + 1);
                timers_for_sent_frames[i].time_added = -1;
            }
            timer_array_lock.lock = 1;

            char* message = ADVANCE_WINDOW;
            printf("Advancing window\n");
            sendto(*server_sockfd, (const char *)message, strlen(message),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len);

            ack_received = 0;
            nbuffered = 0;
            if(file_transfer_complete){
                //Protocol stops
                //Set control for the thread to be 1
                stop_receiver_thread = 1;
                stop_timer_thread = 1;
                printf("File transfer complete\tProtocol Ends!\n");
                char* packet = END_PROTOCOL;
                sendto(*server_sockfd, (const char *)packet, strlen(packet),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len);
                exit(1);
            }
        }
    }
}


void SelRepeatReceiver(int buffer_size, int sock_fd, struct sockaddr_in server_addr, FILE* file_to_write){
    //Initialisations
    int window_size = (buffer_size % PACKET_SIZE) == 0 ? (buffer_size / PACKET_SIZE) : (buffer_size / PACKET_SIZE) + 1;
    printf("Window size: %d\n", window_size);
    const int max_seq_number = 2 * window_size - 1;
    int lower_edge = 0;
    int upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
    
    char* buffer[window_size];
    char* temp_buffer = (char*)malloc(sizeof(char) * PACKET_SIZE);
    //Allocate each buffer the size of packet
    int i = 0;
    for(; i < window_size; i++){
        buffer[i] = (char*)malloc(sizeof(char) * PACKET_SIZE);
        buffer[i][0] = '\0';
    }

    int n, len;
    //Beginning of the protocol
    while(1){
        struct sockaddr_in addr_of_server;
        n = recvfrom(sock_fd, (char *)temp_buffer, PACKET_SIZE, 0, (struct sockaddr *) &addr_of_server, &len);
        printf("Packet received\n");
        int seq_number = (int)temp_buffer[0] - (int)'0';
        if(is_the_arrived_frame_valid(lower_edge, seq_number, upper_edge)){
            //Arrived frame is valid
            int buffer_num = (seq_number) % (window_size);
            int i = 0;
            while(temp_buffer[i] != '\0'){
                buffer[buffer_num][i] = temp_buffer[i];
                printf("%c", buffer[buffer_num][i]);
                i++;
            }
            printf("\n");
            buffer[buffer_num][i] = '\0';

            //Send ack
            char packet[DIGITS_FOR_SEQ_NO + 1];
            packet[0] = temp_buffer[0];
            packet[1] = '\0';
            printf("Ack sent: %d\n", buffer_num);
            sendto(sock_fd, packet, strlen(packet), 0, (const struct sockaddr *) &server_addr,  sizeof(server_addr));
            i = 0;
            while(i < PACKET_SIZE){
                temp_buffer[i] = '\0';
                i++;
            }
        }
        else if(!strncmp(temp_buffer, ADVANCE_WINDOW, strlen(ADVANCE_WINDOW))){
            //Load buffer into the file
            int i = 0;
            /*for(; i < window_size; i++){
                int j = 1;
                printf("%c\n", buffer[i][0]);
                while(buffer[i][j] != '\0' && j < strlen(buffer[i])){
                    printf("%c", buffer[i][j]);
                    j++;
                }
                printf("\n");
            }*/
            //Advance the window
            lower_edge = (upper_edge + 1) % (max_seq_number + 1);
            upper_edge = (lower_edge + window_size - 1) % (max_seq_number + 1);
        }
        else if(!strncmp(temp_buffer, END_PROTOCOL, strlen(END_PROTOCOL))){
            //End protocol
            printf("File received!\n");
            exit(1);
        }
    }
}
