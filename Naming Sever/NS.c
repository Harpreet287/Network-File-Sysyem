#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/times.h>
#include <semaphore.h>



// Local Header Files
#include "Headers.h"
#include "Client_Handle.h"
#include "Server_Handle.h"
#include "Trie.h"
#include "LRU.h"

// Global Header Files
#include "../Externals.h"
#include "../colour.h"

// Global Variables
CLIENT_HANDLE_LIST_STRUCT* clientHandleList;
SERVER_HANDLE_LIST_STRUCT* serverHandleList;
FILE *logs;
CLOCK* Clock;
TrieNode* MountTrie;
LRUCache* MountCache;
sem_t serverStartSem;

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

void* Client_Handler_Thread(void* clientHandle)
{
    CLIENT_HANDLE_STRUCT *client = (CLIENT_HANDLE_STRUCT *) clientHandle;
    printf(UGRN"[+]Client Handler Thread Initialized for Client %lu (%s:%d)\n"reset, client->ClientID, client->sClientIP, client->sClientPort);
    fprintf(logs, "[+]Client Handler Thread Initialized for Client %lu (%s:%d)\n", client->ClientID, client->sClientIP, client->sClientPort);

    // Send data to the client
    char sServerResponse[MAX_BUFFER_SIZE];
    sprintf(sServerResponse, "Hello Client %lu", client->ClientID);
    int iSendStatus = send(client->iClientSocket, sServerResponse, sizeof(sServerResponse), 0);
    if(CheckError(iSendStatus, "[-]Client Handler Thread: Error in sending data to client")) return NULL;
    
    // Receive data from the client
    char sClientRequest[MAX_BUFFER_SIZE];
    int iRecvStatus = recv(client->iClientSocket, &sClientRequest, sizeof(sClientRequest), 0);
    if(CheckError(iRecvStatus, "[-]Client Handler Thread: Error in receiving data from client")) return NULL;

    printf(GRN"[+]Client Handler Thread: Client %lu sent: %s\n"reset, client->ClientID, sClientRequest);
    fprintf(logs, "[+]Client Handler Thread: Client %lu sent: %s\n", client->ClientID, sClientRequest);

    // Close the socket
    RemoveClient(client->ClientID, clientHandleList);
    close(client->iClientSocket);

    return NULL;
}


void* Storage_Server_Acceptor_Thread()
{
    printf(UGRN"[+]Storage Server Acceptor Thread Initialized\n"reset);
    fprintf(logs, "[+]Storage Server Acceptor Thread Initialized [Time Stamp: %f]\n", GetCurrTime(Clock));

    // Create a socket
    int iServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(CheckError(iServerSocket, "[-]Storage Server Acceptor Thread: Error in creating socket")) return NULL;

    // Specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

    // Bind the socket to our specified IP and port
    int iBindStatus = bind(iServerSocket, (struct sockaddr *) &server_address, sizeof(server_address));
    if(CheckError(iBindStatus, "[-]Storage Server Acceptor Thread: Error in binding socket to specified IP and port"))return NULL;

    // Listen for connections
    int iListenStatus = listen(iServerSocket, MAX_QUEUE_SIZE);
    if(CheckError(iListenStatus, "[-]Storage Server Acceptor Thread: Error in listening for connections"))return NULL;

    printf(GRN"[+]Storage Server Acceptor Thread: Listening for connections\n"reset);
    fprintf(logs, "[+]Storage Server Acceptor Thread: Listening for connections [Time Stamp: %f]\n", GetCurrTime(Clock));

    struct sockaddr_in client_address;
    socklen_t iClientSize = sizeof(client_address);
    int iClientSocket; 
    // Accept a connection
    while(iClientSocket = accept(iServerSocket, (struct sockaddr *) &client_address, &iClientSize))
    {
        if(CheckError(iClientSocket, "[-]Storage Server Acceptor Thread: Error in accepting connection")) continue;

        // Store the server IP and Port in Server Handle Struct
        SERVER_HANDLE_STRUCT serverHandle;
        strncpy(serverHandle.sServerIP, inet_ntoa(client_address.sin_addr), IP_LENGTH);
        serverHandle.sServerPort = ntohs(client_address.sin_port);
        serverHandle.sSocket_Write = iClientSocket;

        // Add the server to the server list
        if(CheckError(AddServer(&serverHandle, serverHandleList),"[-]Storage Server Acceptor Thread: Error in adding server to server list"))
        {
            close(iClientSocket);
            continue;
        }

        // Create a thread to handle the server
        pthread_t tServerHandlerThread;
        int iThreadStatus = pthread_create(&tServerHandlerThread, NULL, Storage_Server_Handler_Thread, (void *) &serverHandle);
        if(CheckError(iThreadStatus, "[-]Storage Server Acceptor Thread: Error in creating thread")) continue;        
    }

}

void* Storage_Server_Handler_Thread(void* storageServerHandle)
{
    SERVER_HANDLE_STRUCT *server = (SERVER_HANDLE_STRUCT *) storageServerHandle;
    printf(UGRN"[+]Storage Server Handler Thread Initialized for Server (%s:%d)\n"reset, server->sServerIP, server->sServerPort);
    fprintf(logs, "[+]Storage Server Handler Thread Initialized for Server (%s:%d) [Time Stamp: %f]\n", server->sServerIP, server->sServerPort, GetCurrTime(Clock));

    // Post the semaphore to indicate that a server is online
    sem_post(&serverStartSem);

    // Check if enough servers are running for backups
    if(serverHandleList->iServerCount < (BACKUP_SERVERS + 1))
    {
        printf(YELHB"[+]Storage Server Handler Thread: Waiting for enough servers to be online\n"reset);
        fprintf(logs, "[+]Storage Server Handler Thread: Waiting for other servers to start [Time Stamp: %f]\n", GetCurrTime(Clock));
        
        sem_wait(&serverStartSem);

        printf(GRN"[+]Storage Server Handler Thread: servers online\n"reset);
        fprintf(logs, "[+]Storage Server Handler Thread: servers online [Time Stamp: %f]\n", GetCurrTime(Clock));
    }

    // Recieve the Server Init Packet
    STORAGE_SERVER_INIT_STRUCT serverInitPacket;
    int iRecvStatus = recv(server->sSocket_Write, &serverInitPacket, sizeof(serverInitPacket), 0);
    if(CheckError(iRecvStatus, "[-]Storage Server Handler Thread: Error in receiving data from server"))
    {
        RemoveServer(GetServerID(server), serverHandleList);
        close(server->sSocket_Write);
        return NULL;
    }

    // Unpack the Server Init Packet
    strncpy(server->sServerIP, serverInitPacket.sServerIP, IP_LENGTH);
    server->sServerPort_Client = serverInitPacket.sServerPort_Client;
    server->sServerPort_NServer = serverInitPacket.sServerPort_NServer;

    // Extract indivisual path from the mount paths string (tokenize on \n) and Insert into the mount trie
    char *token = strtok(serverInitPacket.MountPaths, "\n");
    while(token != NULL)
    { 
        token = strtok(NULL, "/");             // Remove the the first token in the path [e.g. (server name/~) , (./~) , (mount/~) , etc.]
        int err_code = Insert_Path(MountTrie, token, server);
        if(CheckError(err_code, "[-]Storage Server Handler Thread: Error in inserting path into mount trie"))
        {
            RemoveServer(GetServerID(server), serverHandleList);
            close(server->sSocket_Write);
            return NULL;
        }
        token = strtok(NULL, "\n");
    }

    // Set Up the Backup Servers for the server
    int err_code = AssignBackupServer(serverHandleList, server->ServerID);  
    if(CheckError(err_code, "[-]Storage Server Handler Thread: Error in assigning backup servers"))
    {
        RemoveServer(server->ServerID, serverHandleList);
        close(server->sSocket_Write);
        return NULL;
    }

    // Set Up Backups in the backup servers (handle later)

    // SetUp listner for the server

    int iServerSocket = socket(AF_INET, SOCK_STREAM, 0);    
    if(CheckError(iServerSocket, "[-]Storage Server Handler Thread: Error in creating socket"))
    {
        RemoveServer(GetServerID(server), serverHandleList);
        close(server->sSocket_Write);
        return NULL;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server->sServerPort_NServer);
    server_address.sin_addr.s_addr = inet_addr(server->sServerIP);
    memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

    int iconnectStatus = connect(iServerSocket, (struct sockaddr *) &server_address, sizeof(server_address));
    if(CheckError(iconnectStatus, "[-]Storage Server Handler Thread: Error in connecting to server"))
    {
        RemoveServer(GetServerID(server), serverHandleList);
        close(server->sSocket_Write);
        return NULL;
    }

    server->sSocket_Read = iServerSocket;

    while(1)
    {
        // Receive the request from the server
        REQUEST_STRUCT request;
        int iRecvStatus = recv(server->sSocket_Write, &request, sizeof(request), 0);
        if(CheckError(iRecvStatus, "[-]Storage Server Handler Thread: Error in receiving data from server"))
        {
            RemoveServer(GetServerID(server), serverHandleList);
            close(server->sSocket_Write);
            return NULL;
        }
        else if(iRecvStatus == 0)
        {
            printf(RED"[-]Storage Server Handler Thread: Server %lu (%s:%d) disconnected(UNGRACEFULLY)\n"reset, server->ServerID, server->sServerIP, server->sServerPort);
            fprintf(logs, "[-]Storage Server Handler Thread: Server %lu (%s:%d) disconnected(UNGRACEFULLY) [Time Stamp: %f]\n", server->ServerID, server->sServerIP, server->sServerPort, GetCurrTime(Clock));

            close(server->sSocket_Write);
            close(server->sSocket_Read);
            int err_code = SetInactive(server->ServerID, serverHandleList);
            if(CheckError(err_code, "[-]Storage Server Handler Thread: Error in setting server inactive"))
            {
                RemoveServer(GetServerID(server), serverHandleList);
                close(server->sSocket_Write);
            }            
            
            return NULL;
        }

        // Handle the request (Generate a response)
    }

    // Disconnect gracefully
    printf(URED"[-]Storage Server Handler Thread: Server %lu (%s:%d) disconnected(GRACEFULLY)\n"reset, server->ServerID, server->sServerIP, server->sServerPort);
    fprintf(logs, "[-]Storage Server Handler Thread: Server %lu (%s:%d) disconnected(GRACEFULLY) [Time Stamp: %f]\n", server->ServerID, server->sServerIP, server->sServerPort, GetCurrTime(Clock));
    close(server->sSocket_Write);
    close(server->sSocket_Read);
    RemoveServer(GetServerID(server), serverHandleList);
    return NULL;
}


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
        fprintf(logs, "Number of Current Servers: %d\n", serverHandleList->iServerCount);
        fprintf(logs, "------------------------------------------------------------\n");
        
        fflush(logs);
    }
    return NULL;
}

void exit_handler()
{
    printf(BRED"[-]Server Exiting\n"reset);
    fprintf(logs, "[-]Server Exiting [Time Stamp: %f]\n", GetCurrTime(Clock));
    Delete_Trie(MountTrie);
    freeCache(MountCache);
    fclose(logs);
}


int main(int argc, char *argv[])
{
    // Open the logs file
    logs = fopen("NSlog.log", "w");

    // Initialize the Naming Server Global Variables
    clientHandleList = InitializeClientHandleList();
    serverHandleList = InitializeServerHandleList();
    sem_init(&serverStartSem, 0, -BACKUP_SERVERS );

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

    // Register the exit handler
    atexit(exit_handler);

    // Create a thread to accept client connections
    pthread_t tClientAcceptorThread;
    iThreadStatus = pthread_create(&tClientAcceptorThread, NULL, Client_Acceptor_Thread, NULL);
    if(CheckError(iThreadStatus, "[-]Error in creating thread")) return 1;

    // Create a thread to accept storage server connections
    pthread_t tStorageServerAcceptorThread;
    iThreadStatus = pthread_create(&tStorageServerAcceptorThread, NULL, Storage_Server_Acceptor_Thread, NULL);
    if(CheckError(iThreadStatus, "[-]Error in creating thread")) return 1;

    // Wait for the thread to terminate
    pthread_join(tClientAcceptorThread, NULL);  
    pthread_join(tStorageServerAcceptorThread, NULL);

    return 0;
}