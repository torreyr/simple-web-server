#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>

/*----------------------------------------------/
 * CODE REFERENCES/HELP:
 * Lab Slides:
 *      https://connex.csc.uvic.ca/access/.../Session_5.pdf
 * udp_client.c from Lab 2:
 *		https://connex.csc.uvic.ca/access/.../udp_server.c
 * Beej's Guide to Network Programming:
 *      http://beej.us/guide/bgnet
  ---------------------------------------------*/

// Functions
char* printBadRequest();
char* printNotFound();
char* printOK();
void  howto();
void  printLogMessage();
char* getResponse();
char* getTime();
bool  isPort();
bool  isDirectory();
bool  parseRequest();
int   sendResponse();
bool  startServer();
bool  createServer();

// Global Variables
DIR* dirptr;
int portnum;
struct sockaddr_in sock_addr;
struct sockaddr_in client_addr;
char* server_dir;
char* res_string;


// ----- CONSOLE ----- //
char* printBadRequest() {
    return "ERROR 400: Bad Request\n";
}

char* printNotFound() {
    return "ERROR 404: Not Found\n";
}

char* printOK() {
    return "Status 200: OK\n";
}

void howto() {
    printf("Correct syntax: ./sws <port> <directory>\n\n");
}

/*
 *	Prints the server's log message.
 */
void printLogMessage(char* req, char* filename, char* res_string) {
	int clport = ntohs(client_addr.sin_port);
	char* clip = inet_ntoa(client_addr.sin_addr);
    int req_len = strlen(req);

	if (strstr(req, "\r\n\r\n") == NULL) {
		printf("%s", printBadRequest());
		return;
	}

    req[req_len - 4] = '\0';
    printf("%s %s:%d %s; HTTP/1.0 200 OK; %s\n\n", getTime(), clip, clport, req, filename);
}

/*
 *	Returns the server's response to be sent to the client.
 */
char* getResponse(char* httpver) {
    res_string = (char*) malloc(sizeof(char) * 1000);
    sprintf(res_string, "%s 200 OK\r\n\r\n", httpver);
    return res_string;
}

/*
 *	Returns the formatted time of the request.
 */
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


// ----- PARSING ----- //
/*
 *	Validates a port number.
 */
bool isPort(char* str) {
    portnum = atoi(str);
    if( (portnum > 0) && (portnum < 65535) ) return true;
    return false;
}

/*
 *	Checks if a path leads to a directory.
 *	Return false if the path cannot be opened as a directory.
 */
bool isDirectory(char* str) {
    dirptr = opendir(str);
    if (dirptr == NULL) return false;
    else return true;
}

/*
 *	Splits the client request into method, path, and HTTP version.
 *	Opens and sends the requested file back to the client.
 *	Then calls printLogMessage();
 */
bool parseRequest(int sock, char* request) {

	char* method  = malloc(50);
	char* path    = malloc(50);
	char* httpver = malloc(50);
    
    char* buffer = (char*) malloc(sizeof(char) * 1000);
    char* buffer_two;

    sscanf(request, "%s %s %s", method, path, httpver);
    sprintf(buffer, "./%s%s", server_dir, path);
    if (isDirectory(buffer)) {
        sprintf(buffer, "%s/index.html", buffer);
    }
	if (strstr(buffer, "../") != NULL) {
		sendResponse(sock, printNotFound());
		return false;
	}
 
    // Account for non-case-sensitive.
    int i, j;
	for(i = 0; method[i]; i++){
        method[i] = toupper(method[i]);
    }
    for(j = 0; httpver[j]; j++){
        httpver[j] = toupper(httpver[j]);
    }
    
    // Validate method and HTTP version.
    if (strcmp(method, "GET") != 0) {
        sendResponse(sock, printBadRequest());
        return false;
    }
    if (strncmp(httpver, "HTTP/1.0\r\n\r\n", 8) != 0) {
        sendResponse(sock, printBadRequest());
        return false;
    }

    // Open and send file.
    FILE* fp = fopen(buffer, "r");
    
	if (fp == NULL) {
        
        sendResponse(sock, printNotFound());
        fclose(fp);
        return false;
    } else {
        
		// Read in the file.
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer_two = (char*) malloc(size * sizeof(char));
		memset(buffer_two, 0, sizeof(buffer_two));
		fread(buffer_two, 1, size, fp);
		buffer_two[size] = 0;
	
		// Create the response.	
        res_string = getResponse(httpver);
		
		// Send the response
        if (sendResponse(sock, res_string) == -1 || sendResponse(sock, buffer_two) == -1) {
            printf("Error sending response.");
        } else {
		    printLogMessage(request, buffer, res_string);
        }
        
		fclose(fp);
	}
    
    return true;
}


// ----- SERVER ----- //
/*
 *	Sends the file to the client.
 *	Sends large files in multiple packets.
 */
int sendResponse(int sock, char* data) {
    int d_len = strlen(data);
	char packet[1024];

    if (d_len > 1024) {
		int offset = 0;
		
		// Create packets.
        while(offset < d_len) {
			memset(packet, 0, 1024);
			strncpy(packet, data + offset, 1024);
			if (sendto(sock, 
                       packet, 
                       1024, 
                       0, 
                       (struct sockaddr*) &client_addr, 
                       sizeof(client_addr)) == -1) return -1;
			else offset = offset + 1024;
        }
		
        return 0;
    } else return sendto(sock, data, d_len, 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
}

/*
 *	Checks command line arguments for correct syntax.
 *	Sets the server directory.
 */
bool startServer(int argc, char* argv[]) {    
	if (argc <= 1) {
		printf("\nIncorrect syntax.\n");
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
	
	printf("sws is running on UDP port %s and serving %s\n", argv[1], argv[2]);
	printf("press 'q' to quit ...\n");
    
	return true;
}

/*
 *	Creates the socket, binds it to the port, sets options.
 *	Waits for client requests.
 */
bool createServer() {
    char buffer[1000];
    fd_set fds;
	memset(buffer, 0, sizeof(buffer));
    
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
    while (1) {
        
        // Reset file descriptors.
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        FD_SET(0, &fds);

		if (select(sock + 1, &fds, NULL, NULL, NULL) < 0) {
            
			printf("Error with select. Closing the socket.\n");
            close(sock);
            return false;
		}

		// Read from standard input.
        if (FD_ISSET(0, &fds)) {
            
            char input[1000];
            fgets(input, 1000, stdin);
            
            if (input[0] == 'q' || input[0] == 'Q') {
                
                close(sock);
                return false;
            }
        } 
        
		// Read from the socket.
        if (FD_ISSET(sock, &fds)) {
                
            recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, &len);
            if (recsize <= 0) {
                
                printf("Didn't receive any data...");
            } else {

                buffer[sizeof(buffer)] = '\0';
                parseRequest(sock, buffer);
            }

			memset(buffer, 0, sizeof(buffer));
        }
		
		memset(buffer, 0, sizeof(buffer));
        
        /* TODO:
		 *	free all your malloc's
         */
    }

	close(sock);
    return true;
}


// ----- MAIN ----- //
int main(int argc, char* argv[]) {
    if ( !startServer(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}
