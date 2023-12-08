#include "../Externals.h"
#include "../colour.h"
#include "Headers.h"


int main(int argc, char *argv[])
{
    // Initialize the client
    printf(GRN"[+]Client Initialized\n"reset);

    // Create a socket
    int iClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    CheckError(iClientSocket, "[-]Error in creating socket");

    // Specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_CLIENT_PORT);
    server_address.sin_addr.s_addr = inet_addr(NS_IP);

    // Connect to the server
    int iConnectionStatus;
    while(CheckError(iConnectionStatus = connect(iClientSocket, (struct sockaddr *) &server_address, sizeof(server_address)), "[-]Error in connecting to server"))
    {
        printf(RED"[-]Client: Connection to server failed. Retrying...\n"reset);
        sleep(1);
    }

    printf(GRN"[+]Connected to the server\n"reset);
    printf(YEL"[+]Press enter to continue..."reset);
    getchar();

    clearScreen();

    // Receive data from the server
    char sServerResponse[MAX_BUFFER_SIZE];
    recv(iClientSocket, &sServerResponse, sizeof(sServerResponse), 0);
    printf(GRN"[+]Server: %s\n"reset, sServerResponse);

    // Send data to the server
    char sClientMessage[MAX_BUFFER_SIZE];
    printf(YEL"[+]Client: "reset);
    scanf("%[^\n]%*c", sClientMessage);
    send(iClientSocket, sClientMessage, sizeof(sClientMessage), 0);

    // Close the socket
    close(iClientSocket);

    return 0;
}