#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

bool startServer(int argc, char* argv[]);
bool isDirectory(char* str);
bool isPort(char* str);
void printBadRequest();
void printNotFound();
void printOK();
void howto();
bool createServer();

// Global Variables
DIR* dirptr;
int portnum;


// PARSING
bool startServer(int argc, char* argv[]) {    
    if (!isPort(argv[1])) {
        printf("Invalid port number. Exiting the program.\n");
        howto();
        return false;
    }
    
    if (!isDirectory(argv[2])) {
        printf("Invalid directory. Exiting the program.\n");
        howto();
        return false;
    }
    
    return true;
}

bool isPort(char* str) {
    portnum = atoi(str);
    if( (portnum > 0) && (portnum < 65535) ) return true;
    return false;
}

bool isDirectory(char* str) {
    dirptr = opendir(str);
    if (dirptr == NULL) return false;
    else return true;
}

bool parseRequest(char* request) {
    // TODO: do this with regular expressions
    char* thing1, path, httpver;
    sscanf("%s %s %s", thing1, path, httpver, request);
    printf("request: %s %s %s", thing1, path, httpver);
    return true;
}


// CONSOLE
void printBadRequest() {
    printf("ERROR 400: Bad Request\n");
}

void printNotFound() {
    printf("ERROR 404: Request Not Found\n");
}

void printOK() {
    printf("Status 200: OK\n");
}

void howto() {
    printf("\nCorrect syntax: ./sws <port> <directory>\n\n");
}


// SERVER
bool createServer() {
    char buffer[1024];
    struct sockaddr_in sock_addr;
    
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        printf("Couldn't create socket. Exiting the program.");
        return false;
    }
    
    ssize_t recsize;
    int len = sizeof(sock_addr);

    memset(&sock_addr, 0, len);
    
    sock_addr.sin_family      = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port        = htons(8080);

    if (bind(sock, (struct sockaddr*) &sock_addr, len) != 0) {
        printf("Couldn't bind socket. Closing the socket.");
        close(sock);
        return false;
    }
    

    while(1) {
        printf("here\n");
        recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &sock_addr, &len);
        if (recsize <= 0) {
            printf("Didn't receive any data...");
            return false;
        } 
        
        printf("data: %s\n", buffer);
        parseRequest(buffer);
    }
    
    return true;
}


// MAIN
int main(int argc, char* argv[]) {
    if ( !startServer(argc, argv) ) return 0;
    if ( !createServer() ) return 0;

}