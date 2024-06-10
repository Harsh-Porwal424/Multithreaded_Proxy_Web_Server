#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_CLIENTS 10
#define MAX_BYTES 4096

typedef struct cache_element cache_element;

struct cache_element{
    char* data;
    int len;
    char* url;
    cache_element* next;
};

cache_element* find(char* url);
int add_cache_element(char* data, int len, char* url);
void remove_cache_element();

int port_number = 8080;
int proxy_socketId;

pthread_t tid[MAX_CLIENTS];
sem_t semaphore;
pthread_mutex_t lock;

cache_element* head;
int cache_size;

int handle_request(int clientSocketId, ParsendRequest *request, char* tempReq){
    char *buf = (char *)malloc(MAX_BYTES*sizeof(char));
    strcpy(buf, "GET ");
    strcat(buf, request->path);
    strcat(buf, " ");
    strcat(buf, request->version);
    strcat(buf, "\r\n");

    size_t len = strlen(buf);
    if(ParsedHeader_get(request, "Host") == NULL){
        if(ParsedHeader_set(request, "Host", request->host) < 0){
            
        }
    }

}
void *thread_fn(void *socketNew){
    sem_wait(&semaphore);
    //Check is semaphore is -1 or not, if it is -1 then it will wait until it becomes 0
    int p;
    sem_getvalue(&semaphore, p);
    printf("Semaphore Value: %d\n", p);

    int *t = (int *) socketNew;
    int socketId = *t;

    int bytes_send_client, len;

    char *buffer = (char *)calloc(MAX_BYTES, sizeof(char));

    bzero(buffer, MAX_BYTES);

    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0);

    while (bytes_send_client > 0)
    {
        len = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL){
            bytes_send_client = recv(socket, buffer+len, MAX_BYTES-len, 0);
        }
        else{
            break;
        }
    }

    //coping the request from buffer to tempReq
    char *tempReq = (char *)malloc(strlen(buffer)*sizeof(char)+1);
    for(int i = 0; i < strlen(buffer); i++){
        tempReq[i] = buffer[i];
    }

    //parsing the request
    struct cache_element* temp = find(tempReq);

    //Code related to cache 
    if(temp != NULL){
        int size = temp->len/sizeof(char);
        int pos = 0;
        char response[MAX_BYTES];
        while(pos < size){
            bzero(response, MAX_BYTES);
            for(int i = 0; i < MAX_BYTES; i++){
                response[i] = temp->data[i];
                pos++;
            }
            send(socket, response, MAX_BYTES, 0);
        }
        printf("Data Retrieved From Cache\n");
        printf("Response is: %s\n", response);
    }
    else if(bytes_send_client > 0){
        len = strlen(buffer);
        ParsedRequest *request = ParsedRequest_create();

        if(ParsedRequest_parse(request, buffer, len) < 0){
            printf("Error Parsing Request\n");
        }
        else{
            bzero(buffer, MAX_BYTES);
            if(!strcmp(request->method, "GET")){
                if(request->host != NULL && request->path != NULL && checkHTTPversion(request->version) == 1){
                    bytes_send_client = handle_request(socket, request, tempReq);
                    if(bytes_send_client == -1){
                        sendErrorMessage(socket, 500);
                    }
                }
                else{
                    sendErrorMessage(socket, 500);
                }
            }
            else{
                printf("This Code Doesnot any  Method apart from GET\n");
            }
        }
        ParsedRequest_destroy(request);
    }else if(bytes_send_client == 0){
        printf("Client Disconnected\n");
    }
    shutdown(socket, SHUT_RDWR);
    close(socket);
    free(buffer);
    sem_post(&semaphore);
    sem_getvalue(&semaphore, p);
    printf("Semaphore Post Value: %d\n", p);
    free(tempReq);
    return NULL;
}


int main(int argc, char* argv[]){
    int client_socketId, client_len;
    struct sockaddr_in server_addr, client_addr;
    sem_init(&semaphore, 0, MAX_CLIENTS);
    pthread_mutex_init(&lock, NULL);
    if(argv == 2){
        port_number = atoi(argv[1]);
    }
    else{
        printf("Too Few Arguments\n");
        exit(1);
    }

    printf("Starting Proxy Server on port: %d\n", port_number);

    /*
    1. socket is a system call in C that creates a new socket and returns a file descriptor (proxy_socketId in this case) that can be used for communication over a network.
    2. AF_INET is the address family, which specifies the protocol family to be used for the socket. AF_INET is used for IPv4 internet protocols.
    3. SOCK_STREAM is the socket type, which specifies the communication semantics. SOCK_STREAM indicates a reliable, two-way, connection-based byte stream, typically used for protocols like TCP.
    4. The third argument 0 is the protocol number. When this value is set to 0, the operating system will choose the appropriate protocol based on the address family and socket type. For AF_INET and SOCK_STREAM, the protocol will be TCP.

    So, this line of code creates a new TCP socket for IPv4 communication. The returned proxy_socketId is a file descriptor that can be used for subsequent 
    socket operations, such as binding the socket to a specific address and port, listening for incoming connections, accepting connections, sending and receiving data, and closing the socket
    */
    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);


    if(proxy_socketId < 0){
        printf("Error Creating Socket\n");
        exit(1);
    } 

    int reuse = 1; //variable is used to allow or disallow the reuse of the same address and port combination for the server socket.

    /*
    The setsockopt function is used to set options on a socket. In this case, it's setting the SO_REUSEADDR
    option, which allows reusing the same address and port for the server socket, even if it's in the
    TIME_WAIT state from a previous connection.
    */
    if(setsockopt(port_number, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0){
        perror("setsockopt failed\n");
    }

    bzero((char*)&server_addr, sizeof(server_addr));  //function erases the data in the n bytes of the memory starting at the location pointed to by s
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number); //converts the unsigned short integer hostshort from host byte order to network byte order.

    /*
    By setting server_addr.sin_addr.s_addr to INADDR_ANY, you're telling the server socket to listen for incoming connections on all available network interfaces on the machine. This is useful when 
    you want the server to be accessible from other machines on the network, or from the internet.
    */
    server_addr.sin_addr.s_addr = INADDR_ANY;

    
    /*
    The bind function is used to bind a socket to a specific address and port. In this case, it's binding the server socket to the address 0.0.0.0 and port 8080.
    */

    //bind() is used to bind a socket to a specific address and port
    if(bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind failed\nPort Already in Use\n");
        exit(1);
    }
    printf("Binding on port: %d\n", port_number);

    int listen_status = listen(proxy_socketId, MAX_CLIENTS);

    if(listen_status < 0){
        perror("listen failed\n");
        exit(1);
    }

    int i = 0;
    int Connected_socketId[MAX_CLIENTS];

    while(1){

        bzero((char*)&client_addr, sizeof(client_addr));

        client_len = sizeof(client_addr);

        client_socketId = accept(proxy_socketId, (struct sockaddr *)&client_addr, (socklen_t*)&client_len);

        if(client_socketId < 0){
            printf("Not able to accept connection\n");
            exit(1);
        }
        else{
            Connected_socketId[i] = client_socketId;
        }


        //Code for printing the IP address and port number of the connected client
        struct sockaddr_in * client_pt = (struct sockaddr_in *)&client_addr;
        struct in_addr * ip_addr = &client_pt->sin_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip_addr, str, INET_ADDRSTRLEN);
        printf("Client is connected with Port number %d IP Address: %s\n", ntohs(client_pt->sin_port), str);



        //Main Server Code

        pthread_create(&tid[i], NULL, thread_fn,(void *)&Connected_socketId[i]);
        i++;
    }
    close(proxy_socketId);
    return 0;


}