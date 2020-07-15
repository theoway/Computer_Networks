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
    char *message = "Hello There- Client"; 
    struct sockaddr_in server_addr; 

    //Socket creation
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd < 0 ){ 
        printf("Socket creation failed"); 
        exit(1); 
    } 

    memset(&server_addr, 0, sizeof(server_addr));
    //Server socket info
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(PORT); 
    server_addr.sin_addr.s_addr = INADDR_ANY; 

    int n, len; 
      
    sendto(sock_fd, (const char *)message, strlen(message), 0, (const struct sockaddr *) &server_addr,  sizeof(server_addr)); 
    printf("Message has been sent\n"); 

    n = recvfrom(sock_fd, (char *)buffer, MAXSIZE, 0, (struct sockaddr *) &server_addr, &len); 
    buffer[n] = '\0'; 
    printf("Response from server : %s\n", buffer); 
  
    close(sock_fd); 

    return 0;
}