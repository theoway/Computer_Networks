#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 9877
#define MAXSIZE 100

int main(){
    int sock_fd;
    char buffer[MAXSIZE];
    char *response = "Message received";

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

    n = recvfrom(sock_fd, (char *)buffer, MAXSIZE,  0, ( struct sockaddr *) &client_addr, &len); 

    buffer[n] = '\0'; 
    printf("Message from client : %s\n", buffer); 

    sendto(sock_fd, (const char *)response, strlen(response),  MSG_CONFIRM, (const struct sockaddr *) &client_addr, len); 
    printf("Response sent!");
    return 0;
}