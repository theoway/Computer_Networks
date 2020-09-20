#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 9877
#define MAXSIZE 1024

#define SERVER_SEL_REPEAT
#include "selrepeat.h" //For selective repeat protcol on server side

void clearBuffer(char* buf){
    int i = 0;
    for(; i < MAXSIZE; i++)
        buf[i] = '\0';
}

int main(){
    
    int sock_fd;
    char buffer[MAXSIZE];
    FILE* fp;

    struct sockaddr_in server_addr, client_addr;

    //Socket Creation
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0){
        printf("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr)); 

    //Server Info
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(sock_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("Binding failed");
        exit(1);
    }

    int len, n; 
    len = sizeof(client_addr);
    
    n = recvfrom(sock_fd, (char *)buffer, MAXSIZE,  MSG_CONFIRM, ( struct sockaddr *) &client_addr, &len);
    buffer[n] = '\0';
    fp = fopen(buffer, "r");
    if(fp == NULL){
        printf("File not available");
        exit(1);
    }
    clearBuffer(buffer);
    int i = 0;
    char c = '\0';
    for(; i < MAXSIZE; i++){
        c = fgetc(fp);
        buffer[i] = c;
        if(c == EOF)
            break;
    }
    //Start selective repeat sending from here

    sendSelRepeatServer(1000,fp, &sock_fd, &client_addr);
    fclose(fp);

    sendto(sock_fd, (const char *)buffer, strlen(buffer),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len); 
    printf("Response sent!");
    return 0;
}