/* 
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);

struct http {
    //Get, post...
    char requestMethod[5];
    //Protocol HTTP 1.0 or HTTP 1.1
    char protocol[9];
    //uri - may need to increase the size on this..
    char uri[1024];
    //get the content type
    //possible types, .png, .gif, .css, .js, .html, .txt, .jpg
    char contentType[25];



};


int main(int argc, char **argv) 
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfdp = malloc(sizeof(int));
	*connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	//temporarily commenting out
    pthread_create(&tid, NULL, thread, connfdp);

    //wait for a certain amount of time then quit
    //pthread_exit(NULL);
    }
}

/* thread routine */
void * thread(void * vargp) 
{  
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self()); 
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * parse - a function to get all the pieces of the http header
 */


struct http parse(char *exmsg){
    struct http myHttp;
    char* req = strtok(exmsg, " ");
    strncpy(myHttp.requestMethod, req, 4);
    printf("The request method is %s \n", myHttp.requestMethod);

    char* uri = strtok(NULL, " ");
    strncpy(myHttp.uri, uri, 1024);
    printf("The uri is %s \n", myHttp.uri);

    char* protocol = strtok(NULL, "");
    strlcpy(myHttp.protocol, protocol, 9);
    printf("The protocol is %s \n", myHttp.protocol);

    char* fileType = strtok(uri, ".");
    fileType = strtok(NULL, "");
    printf("The fileType is %s \n", fileType);

    // //I am terrible at strings in C, but this does the trick
    //get the correct contentType for http header
    //sad I can't use a switch case but whatev

    if(strcmp(fileType, "html") == 0){
        strcpy(myHttp.contentType, "text/html");
    }
    else if(strcmp(fileType, "txt") == 0){
        strlcpy(myHttp.contentType, "text/plain", 11);
    }
    else if(strcmp(fileType, "png") == 0){
        strlcpy(myHttp.contentType, "image/png", 10);
    }
    else if(strcmp(fileType, "gif") == 0){
        strlcpy(myHttp.contentType, "image/gif", 10);
    }
    else if(strcmp(fileType, "jpg") == 0){
        strlcpy(myHttp.contentType, "image/jpg", 10);
    }
    else if(strcmp(fileType, "css") == 0){
        strlcpy(myHttp.contentType, "text/css", 9);
    }
    else if(strcmp(fileType, "js") == 0){
        strlcpy(myHttp.contentType, "application/javascript", 23);
    }
    else if(strcmp(fileType, "ico") == 0){
        strlcpy(myHttp.contentType, "image/x-icon", 13);
    }
    else {
        strlcpy(myHttp.contentType, "none", 4);
    }

    return myHttp;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello Meep!</h1></html>"; 
    struct stat file_stat;
    n = read(connfd, buf, MAXLINE);

    //parse through buffer to check if the message from the client is valid
    printf("The response from the client is %s \n", buf);
    struct http myHttp;
    myHttp = parse(buf);

    //check if the requestMethod is valid
    //check for correct method and correct protocol
    if(strcmp(myHttp.requestMethod, "GET") || strcmp(myHttp.protocol, "HTTP/1.1")){
        //send a 500 error code
        printf("The request method is %s\n", myHttp.requestMethod);
        char errorMsg[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Type:text/html\r\nContent-Length:54\r\n\r\n<html><h1>Error 500: Internal Server Error</h1></html>";
        printf("server returning a http message with the following content.\n%s\n",errorMsg);
        write(connfd, errorMsg,strlen(errorMsg));
    }
    else {
        //declare some useful variables
        long f_size;
        int offset;
        int remain_data;
        int sent_bytes = 0;

        //create the string where the file lives
        char *myDir = malloc(100);
        char www[] = "www";
        strcat(myDir, www);
        strcat(myDir, myHttp.uri);
        printf("The correct dir is %s \n", myDir);

        //open file/check if it exists
        FILE* thisFile = fopen(myDir, "rb");
        if(thisFile == NULL){
            printf("Failed to read file :( \n");
        } 
        else {
            //get file size
            fseek(thisFile, 0, SEEK_END);
            f_size = ftell(thisFile);
            printf("The file size is %ld\n", f_size);
            //beginning of all HTTP messages
            char str1[] = "HTTP/1.1 200 Document Follows\r\nContent-Type:";
            
            //convert the long to string to put it in the header
            char len[33];
            sprintf(len, "%ld", f_size);

            char end[] = "\r\n\r\n";

            //combine the initial string, the contentType, the contentLength and the end string
            snprintf(buf, 256, "%s%s%s%s%s%s", str1, myHttp.contentType, "\r\n", "ContentLength:", len, end);
            write(connfd, buf, strlen(buf));


            offset = 0;
            remain_data = f_size;
            fseek(thisFile, 0, SEEK_SET);
            //I used this to find a while loop and write command combo https://stackoverflow.com/questions/11952898/c-send-and-receive-file
            //get file data with fread then write to socket
            while ((sent_bytes = fread(buf, 1, MAXLINE, thisFile) > 0) && remain_data > 0)
            {
                write(connfd, buf, strlen(buf));
                offset += sent_bytes;
                remain_data -= sent_bytes;
                printf( "Server sent %d bytes from file's data, offset is now : %d and remaining data = %d\n", sent_bytes, offset, remain_data);
            }
            fclose(thisFile);
        }

    }

    
    
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */

