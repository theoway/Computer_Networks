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

#include "selrepeat.h"

int main(){
    int sock_fd; 
    char buffer[MAXSIZE]; 
    char message[MAXSIZE]; 
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
    scanf("%s", message);

    FILE* fp = fopen("./received_file.txt","w");
    if(!fp)
    {
        printf("Couldn't open the file\n");
    }
    sendto(sock_fd, message, strlen(message), 0, (const struct sockaddr *) &server_addr,  sizeof(server_addr)); 
    printf("File name sent\n");
    SelRepeatReceiver(2000, sock_fd, server_addr, fp);

    /*n = recvfrom(sock_fd, (char *)buffer, MAXSIZE, 0, (struct sockaddr *) &server_addr, &len); 
    buffer[n] = '\0'; 
    printf("File content below:\n");

    int i;
    char ch;
    for(i = 0; i < MAXSIZE; i++){
        ch = buffer[i];
        if(ch == EOF)
            return 0;
        printf("%c", ch);
    } */
  
    close(sock_fd); 

    return 0;
}