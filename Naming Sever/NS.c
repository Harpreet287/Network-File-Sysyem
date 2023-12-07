#include "Headers.h"
#include "../Externals.h"
#include "../colour.h"

int main(int argc, char *argv[])
{
    // Initialize the Naming Server
    printf(GRN"[+]Naming Server Initialized\n"reset);

    // Create a socket
    int iServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    CheckError(iServerSocket, "[-]Error in creating socket");

    // Specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_PORT);
    server_address.sin_addr.s_addr = inet_addr(NS_IP);

    // Bind the socket to our specified IP and port
    int iBindStatus = bind(iServerSocket, (struct sockaddr *) &server_address, sizeof(server_address));
    CheckError(iBindStatus, "[-]Error in binding socket to specified IP and port");

    // Listen for connections
    int iListenStatus = listen(iServerSocket, MAX_QUEUE_SIZE);
    CheckError(iListenStatus, "[-]Error in listening for connections");

    // Accept a connection
    int iClientSocket = accept(iServerSocket, NULL, NULL);
    CheckError(iClientSocket, "[-]Error in accepting connection");

    // Send data to the client
    char sServerMessage[MAX_BUFFER_SIZE] = "Hello from the server";
    send(iClientSocket, sServerMessage, sizeof(sServerMessage), 0);

    // Receive data from the client
    char sClientResponse[MAX_BUFFER_SIZE];
    recv(iClientSocket, &sClientResponse, sizeof(sClientResponse), 0);
    printf(GRN"[+]Client: %s\n"reset, sClientResponse);

    // Close the socket
    close(iServerSocket);

    return 0;
}