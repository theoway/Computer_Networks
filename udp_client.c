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

    fclose(fp);

    fp = fopen("./received_file.txt", "r");
    //Displaying file content
    printf("File content: \n");
    char c;
    while((c = fgetc(fp)) != EOF && c != '\0'){
        printf("%c",c);
    }
    fclose(fp);
    close(sock_fd); 

    return 0;
}