#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/times.h>



// Local Header Files
#include "Headers.h"
#include "Client_Handle.h"
#include "Trie.h"
#include "LRU.h"

// Global Header Files
#include "../Externals.h"
#include "../colour.h"

// Global Variables
CLIENT_HANDLE_LIST_STRUCT* clientHandleList;
FILE *logs;
CLOCK* Clock;
TrieNode* MountTrie;
LRUCache* MountCache;

/**
 * Initializes the clock object.
**/
CLOCK* InitClock()
{
    CLOCK* C = (CLOCK*) malloc(sizeof(CLOCK));
    if(CheckNull(C, "[-]InitClock: Error in allocating memory"))
    {
        fprintf(logs, "[-]InitClock: Error in allocating memory\n");
        exit(EXIT_FAILURE);
    }
    
    C->bootTime = 0;
    C->bootTime = GetCurrTime(C);
    if(CheckError(C->bootTime, "[-]InitClock: Error in getting current time"))
    {
        fprintf(logs, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }

    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &C->Btime);
    if(CheckError(err, "[-]InitClock: Error in getting current time"))
    {
        fprintf(logs, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }
        
    return C;
}

/**
 * Returns the current time in seconds.
 * @param Clock: The clock object.
 * @return: The current time in seconds on success, -1 on failure.
**/
double GetCurrTime(CLOCK* Clock)
{
    if(CheckNull(Clock, "[-]GetCurrTime: Invalid clock object"))
    {
        fprintf(logs, "[-]GetCurrTime: Invalid clock object\n");
        return -1;
    }
    struct timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    if(CheckError(err, "[-]GetCurrTime: Error in getting current time"))
    {
        fprintf(logs, "[-]GetCurrTime: Error in getting current time\n");
        return -1;
    }
    return (time.tv_sec + time.tv_nsec * 1e-9) - (Clock->bootTime);
}

// Thread to Asynchronously accept client connections
void* Client_Acceptor_Thread()
{
    printf(UGRN"[+]Client Acceptor Thread Initialized\n"reset);
    fprintf(logs, "[+]Client Acceptor Thread Initialized [Time Stamp: %f]\n", GetCurrTime(Clock));

    // Create a socket
    int iServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(CheckError(iServerSocket, "[-]Client Acceptor Thread: Error in creating socket")) return NULL;

    // Specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_CLIENT_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

    // Bind the socket to our specified IP and port
    int iBindStatus = bind(iServerSocket, (struct sockaddr *) &server_address, sizeof(server_address));
    if(CheckError(iBindStatus, "[-]Client Acceptor Thread: Error in binding socket to specified IP and port"))return NULL;

    // Listen for connections
    int iListenStatus = listen(iServerSocket, MAX_QUEUE_SIZE);
    if(CheckError(iListenStatus, "[-]Client Acceptor Thread: Error in listening for connections"))return NULL;

    printf(GRN"[+]Client Acceptor Thread: Listening for connections\n"reset);
    fprintf(logs, "[+]Client Acceptor Thread: Listening for connections [Time Stamp: %f]\n", GetCurrTime(Clock));

    struct sockaddr_in client_address;
    socklen_t iClientSize = sizeof(client_address);
    int iClientSocket; 
    // Accept a connection
    while(iClientSocket = accept(iServerSocket, (struct sockaddr *) &client_address, &iClientSize))
    {
        if(CheckError(iClientSocket, "[-]Client Acceptor Thread: Error in accepting connection")) continue;

        // Store the client IP and Port in Client Handle Struct
        CLIENT_HANDLE_STRUCT clientHandle;
        strncpy(clientHandle.sClientIP, inet_ntoa(client_address.sin_addr), IP_LENGTH);
        clientHandle.sClientPort = ntohs(client_address.sin_port);
        clientHandle.iClientSocket = iClientSocket;

        // Add the client to the client list
        if(CheckError(AddClient(&clientHandle, clientHandleList),"[-]Client Acceptor Thread: Error in adding client to client list"))
        {
            close(iClientSocket);
            continue;
        }

        // Create a thread to handle the client
        pthread_t tClientHandlerThread;
        int iThreadStatus = pthread_create(&tClientHandlerThread, NULL, Client_Handler_Thread, (void *) &clientHandle);
        if(CheckError(iThreadStatus, "[-]Client Acceptor Thread: Error in creating thread")) continue;  
    } 

    return NULL;
}

// Thread to handle a client
void* Client_Handler_Thread(void* clientHandle)
{
    CLIENT_HANDLE_STRUCT *client = (CLIENT_HANDLE_STRUCT *) clientHandle;
    printf(UGRN"[+]Client Handler Thread Initialized for Client %d (%s:%d)\n"reset, client->ClientID, client->sClientIP, client->sClientPort);
    fprintf(logs, "[+]Client Handler Thread Initialized for Client %d (%s:%d)\n", client->ClientID, client->sClientIP, client->sClientPort);

    // Send data to the client
    char sServerResponse[MAX_BUFFER_SIZE];
    sprintf(sServerResponse, "Hello Client %d", client->ClientID);
    int iSendStatus = send(client->iClientSocket, sServerResponse, sizeof(sServerResponse), 0);
    if(CheckError(iSendStatus, "[-]Client Handler Thread: Error in sending data to client")) return NULL;
    
    // Receive data from the client
    char sClientRequest[MAX_BUFFER_SIZE];
    int iRecvStatus = recv(client->iClientSocket, &sClientRequest, sizeof(sClientRequest), 0);
    if(CheckError(iRecvStatus, "[-]Client Handler Thread: Error in receiving data from client")) return NULL;

    printf(GRN"[+]Client Handler Thread: Client %d sent: %s\n"reset, client->ClientID, sClientRequest);
    fprintf(logs, "[+]Client Handler Thread: Client %d sent: %s\n", client->ClientID, sClientRequest);

    // Close the socket
    RemoveClient(client->ClientID, clientHandleList);
    close(client->iClientSocket);

    return NULL;
}

// Thread to flush the logs periodically
void* Log_Flusher_Thread()
{
    while(1)
    {
        sleep(LOG_FLUSH_INTERVAL);
        printf(BBLK"[+]Log Flusher Thread: Flushing logs\n"reset);

        fprintf(logs, "[+]Log Flusher Thread: Flushing logs [Time Stamp: %f]\n", GetCurrTime(Clock));
        fprintf(logs, "------------------------------------------------------------\n");
        fprintf(logs, "Current Mount Trie:\n");
        fprintf(logs, "%s\n",Get_Directory_Tree(MountTrie, "mount"));
        fprintf(logs, "Number of Current Clients: %d\n", clientHandleList->iClientCount);
        fprintf(logs, "------------------------------------------------------------\n");
        
        fflush(logs);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Open the logs file
    logs = fopen("NSlog.log", "w");

    // Initialize the Naming Server Global Variables
    clientHandleList = InitializeClientHandleList();

    // Initialize the Mount Paths Trie
    MountTrie = Init_Trie();
    Insert_Path(MountTrie, "mount", NULL);

    // Initialize the LRU Cache
    MountCache = createCache();

    // Initialize the clock object
    Clock = InitClock();

    // Create a thread to flush the logs periodically
    pthread_t tLogFlusherThread;
    int iThreadStatus = pthread_create(&tLogFlusherThread, NULL, Log_Flusher_Thread, NULL);
    if(CheckError(iThreadStatus, "[-]Error in creating thread")) return 1;

    printf(BGRN"[+]Naming Server Initialized\n"reset);
    fprintf(logs, "[+]Naming Server Initialized [Time Stamp: %f]\n", GetCurrTime(Clock));

    // Create a thread to accept client connections
    pthread_t tClientAcceptorThread;
    iThreadStatus = pthread_create(&tClientAcceptorThread, NULL, Client_Acceptor_Thread, NULL);
    if(CheckError(iThreadStatus, "[-]Error in creating thread")) return 1;

    // Wait for the thread to terminate
    pthread_join(tClientAcceptorThread, NULL);  

    return 0;
}