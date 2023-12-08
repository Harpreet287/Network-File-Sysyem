#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "Headers.h"
#include "Client_Handle.h"
#include "../colour.h"


CLIENT_HANDLE_LIST_STRUCT* InitializeClientHandleList()
{
    CLIENT_HANDLE_LIST_STRUCT *clientHandleList = (CLIENT_HANDLE_LIST_STRUCT *) malloc(sizeof(CLIENT_HANDLE_LIST_STRUCT));
    memset(clientHandleList, 0, sizeof(clientHandleList));
    pthread_mutex_init(&clientHandleList->clientListMutex, NULL);
    return clientHandleList;
}

/**
 * Adds a client to the client list.
 *
 * @param clientHandle The client handle of the client to be added.
 * @return -1 if the maximum number of clients have been reached, otherwise 0.
 */
int AddClient(CLIENT_HANDLE_STRUCT *clientHandle, CLIENT_HANDLE_LIST_STRUCT *clientHandleList)
{
    pthread_mutex_lock(&clientHandleList->clientListMutex);
    if(clientHandleList->iClientCount == MAX_CLIENTS)
    {
        pthread_mutex_unlock(&clientHandleList->clientListMutex);
        printf(RED"[-]AddClient: Maximum number of clients reached\n"reset);
        fprintf(logs, "[-]AddClient: Maximum number of clients reache [Time Stamp: %f]\n", GetCurrTime(Clock));
        return -1;
    }
    
    // Find First Empty Slot
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clientHandleList->InUseList[i] == 0)
        {
            clientHandleList->InUseList[i] = 1;
            clientHandleList->clientList[i] = *clientHandle;
            clientHandleList->iClientCount++;
            pthread_mutex_unlock(&clientHandleList->clientListMutex);
            break;
        }
    }

    
    printf(GRN"[+]AddClient: Client-%d (%s:%d) added to client list\n"reset, clientHandle->ClientID, clientHandle->sClientIP, clientHandle->sClientPort);
    fprintf(logs, "[+]AddClient: Client-%d (%s:%d) added to client list [Time Stamp: %f]\n", clientHandle->ClientID, clientHandle->sClientIP, clientHandle->sClientPort, GetCurrTime(Clock));
    return 0;
}

/**
 * Removes a client from the client list.
 *
 * @param ClientID The ID of the client to be removed.
 * @return -1 if there are no clients in the list or the client is not found, otherwise 0.
 */
int RemoveClient(int ClientID, CLIENT_HANDLE_LIST_STRUCT *clientHandleList)
{
    pthread_mutex_lock(&clientHandleList->clientListMutex);
    if(clientHandleList->iClientCount == 0)
    {
        pthread_mutex_unlock(&clientHandleList->clientListMutex);
        printf(RED"[-]RemoveClient: No clients to remove\n"reset);
        fprintf(logs, "[-]RemoveClient: No clients to remove [Time Stamp: %f]\n", GetCurrTime(Clock));
        return -1;
    }    

    // Find the client
    CLIENT_HANDLE_STRUCT *clientHandle = NULL;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clientHandleList->clientList[i].ClientID == ClientID)
        {
            clientHandle = &clientHandleList->clientList[i];
            break;
        }
    }

    if(CheckNull(clientHandle, "[-]RemoveClient: Client not found"))
    {
        pthread_mutex_unlock(&clientHandleList->clientListMutex);
        fprintf(logs, "[-]RemoveClient: Client not found [Time Stamp: %f]\n", GetCurrTime(Clock));
        return -1;
    }

    // Remove the client
    clientHandleList->InUseList[clientHandle->ClientID] = 0;
    clientHandleList->iClientCount--;
    pthread_mutex_unlock(&clientHandleList->clientListMutex);

    printf(GRN"[+]RemoveClient: Client-%d (%s:%d) removed from client list\n"reset, clientHandle->ClientID, clientHandle->sClientIP, clientHandle->sClientPort);
    fprintf(logs, "[+]RemoveClient: Client-%d (%s:%d) removed from client list [Time Stamp: %f]\n", clientHandle->ClientID, clientHandle->sClientIP, clientHandle->sClientPort, GetCurrTime(Clock));
}

