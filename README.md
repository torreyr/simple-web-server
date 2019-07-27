# simple-web-server
created by: Torrey Randolph
            CSC 361 - Lab B07

This program creates a server and allows a client to send GET requests using UDP.

### Syntax:
  ./sws \<port\> \<directory\>

NOTE: the client request path cannot include "../"

### Files:
 - sws.c : simple web server
 - makefile : a makefile to compile sws.c or clean the directory
 - tests.txt : sample requests and their expected outputs
 - www/ : testing directory
 - www/index.html : text file used for testing
 - www/longfile.html : html file used for testing
            
My simple web server code is broken up into sections. Namely, server functions, methods used to parse user input, and console output functions.
