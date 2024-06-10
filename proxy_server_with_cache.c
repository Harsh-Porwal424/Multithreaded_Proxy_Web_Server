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
    /*
    comments: The htons function converts the unsigned short integer hostshort from host byte order to network byte order.
    */
    server_addr.sin_port = htons(port_number);
    server_addr.sin_add.s_addr = INADDR_ANY;

    /*
    The bind function is used to bind a socket to a specific address and port. In this case, it's binding the server socket to the address 0.0.0.0 and port 8080.
    */


}