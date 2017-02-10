#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
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
char* server_dir;


// CONSOLE
void printBadRequest() {
    printf("ERROR 400: Bad Request\n");
}

void printNotFound() {
    printf("ERROR 404: Not Found\n");
}

void printOK() {
    printf("Status 200: OK\n");
}

void howto() {
    printf("Correct syntax: ./sws <port> <directory>\n\n");
}

char* getTime() {
    char* buffer;
	buffer = malloc(20);
    time_t curtime;
	struct tm* times;
	
	time(&curtime);
    times = localtime(&curtime);
    strftime(buffer, 30, "%b %d %T", times);
	return buffer;
}


// PARSING
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
	
	printf("%s\n", request);
	printf("in parseRequest\n");

	char* method  = malloc(50);
	char* path    = malloc(50);
	char* httpver = malloc(50);
    
    char* buffer = (char*) malloc(sizeof(char) * 1000);
    char* buffer_two = (char*) malloc(sizeof(char) * 1000);
    
    
    /* TODO:
     *  Scan through for first newline character. Delete everything following.
     *  Split at the spaces.
     *  Send chunks to their respective functions.
     */
    
    /*printf("HERE\n");
    char* ptr = strchr(request, '\n');
    if (ptr == NULL) {
        printf("No newline character found.\n");
    } else {
        printf("found a newline character\n");
        printf("%s", strtok(request, ptr));
    }*/
    
    //strtok(request, " \t");
    
    sscanf(request, "%s %s %s", method, path, httpver);
    sprintf(buffer, "./%s%s", server_dir, path);
    
    // Account for non-case-sensitive.
    int i, j;
	for(i = 0; method[i]; i++){
        method[i] = toupper(method[i]);
    }
    for(j = 0; httpver[i]; i++){
        httpver[i] = toupper(httpver[i]);
    }
    
    // Validate request.
    if (strcmp(method, "GET") != 0) {
        printBadRequest();
        return false;
    }
    if (strncmp(httpver, "HTTP/1.0", 8) != 0) {
        printBadRequest();
        return false;
    }
    
    FILE* fp = fopen(buffer, "r");
    if (fp == NULL) {
        printNotFound();
        return false;
    } else {
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        fread(buffer_two, 1, size, fp);
        buffer_two[size] = 0;
        printf("FILE CONTENTS:\n%s\n", buffer_two);
        fclose(fp);
    }
    
	printf("scanned request and printing\n");
	printf("request: %s %s %s\n", method, path, httpver);
    return true;
}


// SERVER
bool startServer(int argc, char* argv[]) {    
	if (argc <= 1) {
		printf("\nERROR\n");
        howto();
		return false;
	}

    if (!isPort(argv[1])) {
        printf("\nInvalid port number. Exiting the program.\n");
        howto();
        return false;
    }
    
    if (!isDirectory(argv[2])) {
        printf("\nInvalid directory. Exiting the program.\n");
        howto();
        return false;
    } else server_dir = argv[2];
    
	return true;
}

bool createServer() {
    char buffer[1024];
    struct sockaddr_in sock_addr;
    fd_set read_fds;
    
    
    // Create socket.
    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        printf("Couldn't create socket. Exiting the program.");
        return false;
    }
    
    // Set socket options.
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &opt, sizeof(opt)) == -1) {
        printf("Problem setting socket options. Closing the socket.");
        return false;
    }
    
    ssize_t recsize;
    int len = sizeof(sock_addr);

    memset(&sock_addr, 0, len);
    
    sock_addr.sin_family      = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port        = htons(portnum);

    // Bind socket.
    if (bind(sock, (struct sockaddr*) &sock_addr, len) != 0) {
        printf("Couldn't bind socket. Closing the socket.\n");
        close(sock);
        return false;
    }
    
    // Loop forever to receive requests.
    while(1) {
        printf("in while loop\n");
        
        // Reset file descriptors.
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(0, &read_fds);

        if (select(sock + 1, &read_fds, 0, 0, 0) < 0) {
            if (FD_ISSET(0, &read_fds)) {
                
                close(sock);
                printf("Quitting...\n");
                return false;
                
            } else if (FD_ISSET(sock + 1, &read_fds)) {
                
                recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &sock_addr, &len);
                if (recsize <= 0) {
                    printf("Didn't receive any data...");
                    return false;
                } else {
                    printf("%s\n", getTime());
                }
                
                //printf("printing data...\n");        
                buffer[sizeof(buffer)] = '\0';
                printf("data: %s\n", buffer);
                if (!parseRequest(buffer)) printf("Error parsing request...");
                
            }
        }
        
		/*recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &sock_addr, &len);
        if (recsize <= 0) {
            printf("Didn't receive any data...");
            return false;
        } else {
			printf("%s\n", getTime());
        }
        
		//printf("printing data...\n");        
		buffer[sizeof(buffer)] = '\0';
        printf("data: %s\n", buffer);
		if (!parseRequest(buffer)) printf("Error parsing request...");
        */
        
        //parseRequest("GET /index.html HTTP/1.0\r\n\r\n");
        //break;
        
        /* TODO:
         *  listen for the q to quit
         *  send the response header and file
         *  reference the lab code that you used
         */
    }
    
    return true;
}


// MAIN
int main(int argc, char* argv[]) {
    if ( !startServer(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}
