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
 * Beej's Guide to Network Programming:
 *      http://beej.us/guide/bgnet
  ---------------------------------------------*/

bool  isDirectory();
bool  isPort();
char* printBadRequest();
char* printNotFound();
char* printOK();
void  howto();
void  printLogMessage();
char* getResponse();
char* getTime();
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


// CONSOLE
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

void printLogMessage(char* req, char* filename, char* res_string) {
	int clport = ntohs(client_addr.sin_port);
	char* clip = inet_ntoa(client_addr.sin_addr);
    int req_len = strlen(req);

    req[req_len - 4] = '\0';
    printf("%s %s:%d %s; HTTP/1.0 200 OK; %s\n", getTime(), clip, clport, req, filename);
}

char* getResponse(char* httpver) {
	printf("got reponse\n");
    res_string = (char*) malloc(sizeof(char) * 1000);
    sprintf(res_string, "%s 200 OK\r\n\r\n", httpver);
    return res_string;
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

bool parseRequest(int sock, char* request) {

	char* method  = malloc(50);
	char* path    = malloc(50);
	char* httpver = malloc(50);
    
    char* buffer = (char*) malloc(sizeof(char) * 1000);
    char* buffer_two;

    
    // TODO: test if it works with tab delimiters
    // TODO: check for blank line at the end of the request
    
    /*printf("HERE\n");
    char* ptr = strchr(request, '\n');
    if (ptr == NULL) {
        printf("No newline character found.\n");
    } else {
        printf("found a newline character\n");
        printf("%s", strtok(request, ptr));
    }*/
    
    sscanf(request, "%s %s %s", method, path, httpver);
    sprintf(buffer, "./%s%s", server_dir, path);
    if (isDirectory(buffer)) {
        sprintf(buffer, "%s/index.html", buffer);
    }
 
    // Account for non-case-sensitive.
    int i, j;
	for(i = 0; method[i]; i++){
        method[i] = toupper(method[i]);
    }
    for(j = 0; httpver[i]; i++){
        httpver[i] = toupper(httpver[i]);
    }
    
    // Validate method and HTTP version.
    if (strcmp(method, "GET") != 0) {
        sendResponse(sock, printBadRequest());
        return false;
    }
    if (strncmp(httpver, "HTTP/1.0", 8) != 0) {
        sendResponse(sock, printBadRequest());
        return false;
    }

    // Open file and get contents.
    FILE* fp = fopen(buffer, "r");
    
	if (fp == NULL) {
        
        sendResponse(sock, printNotFound());
        fclose(fp);
        return false;
    } else {
        
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer_two = (char*) malloc(size * sizeof(char));
		memset(buffer_two, 0, sizeof(buffer_two));
		fread(buffer_two, 1, size, fp);
		buffer_two[size] = 0;
	
		// Create the response.	
        res_string = getResponse(httpver);
        printf("%s\n", res_string);
		// Send the response
        if (sendResponse(sock, res_string) == -1 || sendResponse(sock, buffer_two) == -1) {
            printf("Error sending response.");
        } else {
        	//printf("here\n");
		    printLogMessage(request, buffer, res_string);
        }
        
		fclose(fp);
	}
    
    return true;
}


// SERVER
int sendResponse(int sock, char* data) {
    int d_len = strlen(data);

    if (d_len > 1024) {
        int offset = 0;
        while(offset < d_len) {
            if (sendto(sock, 
                       data + offset, 
                       1024, 
                       0, 
                       (struct sockaddr*) &client_addr, 
                       sizeof(client_addr)) == -1) return -1;
			offset = offset + 1024;
        }
        return 0;
    } else {
	    return sendto(sock, data, d_len, 0, (struct sockaddr*) &client_addr, sizeof(client_addr));
    }
}

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
    char buffer[1000];
    fd_set fds;
    
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

        if (FD_ISSET(0, &fds)) {
            
            char input[1000];
            fgets(input, 1000, stdin);
            
            if (input[0] == 'q' || input[0] == 'Q') {
                
                close(sock);
                return false;
            }
        } 
        
        if (FD_ISSET(sock, &fds)) {
                
            recsize = recvfrom(sock, (void*) buffer, sizeof(buffer), 0, (struct sockaddr*) &client_addr, &len);
            if (recsize <= 0) {
                
                printf("Didn't receive any data...");
                return false;
            } else {
                buffer[sizeof(buffer)] = '\0';
                if (!parseRequest(sock, buffer)) printf("Error parsing request...");
            }
        }
		
		memset(buffer, 0, 1000 * sizeof(char));
        
        /* TODO:
         *  send the response header and file
         *  send long files in multiple packets
		 *	free all your malloc's
         *  reference the code that you used
         */
    }
    
    return true;
}


// MAIN
int main(int argc, char* argv[]) {
    if ( !startServer(argc, argv) ) return 0;
    if ( !createServer(argv) ) return 0;
}
