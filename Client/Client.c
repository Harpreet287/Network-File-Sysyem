// Standard Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>


// Custom Header Files
#include "../Externals.h"
#include "../colour.h"
#include "./Headers.h"
#include "./Hash.h"
#include "./ErrorCodes.h"

FILE *Clientlog;
HashTable *table;
unsigned long iClientID;
CLOCK* Clock;

/**
 * @brief Initializes the clock object.
 * @return: A pointer to the clock object on success, NULL on failure.
**/
CLOCK* InitClock()
{
    CLOCK* C = (CLOCK*) malloc(sizeof(CLOCK));
    if(CheckNull(C, "[-]InitClock: Error in allocating memory"))
    {
        fprintf(Clientlog, "[-]InitClock: Error in allocating memory \n");
        exit(EXIT_FAILURE);
    }
    
    C->bootTime = 0;
    C->bootTime = GetCurrTime(C);
    if(CheckError(C->bootTime, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Clientlog, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }

    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &C->Btime);
    if(CheckError(err, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Clientlog, "[-]InitClock: Error in getting current time\n");
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
        fprintf(Clientlog, "[-]GetCurrTime: Invalid clock object\n");
        return -1;
    }
    struct timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    if(CheckError(err, "[-]GetCurrTime: Error in getting current time"))
    {
        fprintf(Clientlog, "[-]GetCurrTime: Error in getting current time\n");
        return -1;
    }
    return (time.tv_sec + time.tv_nsec * 1e-9) - (Clock->bootTime);
}


/**
 * @brief Polls the socket to check if it is online and reconnects if it is not
 * @param sockfd The socket to poll
 * @param ip The ip address of the server
 * @param port The port of the server
 * @return value of sockfd with a connection to the server
*/
int pollServer(int sockfd, char* ip, int port)
{
    fprintf(Clientlog, "[+]pollServer: Polling server [Time Stamp: %f]\n",GetCurrTime(Clock));

    fd_set write_fds;
    struct timeval timeout;

    FD_ZERO(&write_fds);
    FD_SET(sockfd, &write_fds);

    timeout.tv_sec = POLL_TIME;
    timeout.tv_usec = 0;

    // Check if the socket is writable
    int result = select(sockfd + 1, NULL, &write_fds, NULL, &timeout);

    if (result == -1)
    {
        printf(RED "[-]pollServer: Error in select\n" reset);
        perror("select");
        fprintf(Clientlog, "[-]pollServer: Error in select [Time Stamp: %f]\n",GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    else if (result == 0)
    {
        // Timeout occurred, socket is not writable
        // close the socket and open a new connection
        close(sockfd);

        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = inet_addr(ip);
        memset(server_address.sin_zero, '\0', sizeof(server_address.sin_zero));

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (CheckError(sockfd, "[-]pollServer: Error in creating socket"))
        {
            fprintf(Clientlog, "[-]Error in creating socket [Time Stamp: %f]\n",GetCurrTime(Clock));
            exit(EXIT_FAILURE);
        }

        int iConnectionStatus;
        while (CheckError(iConnectionStatus = connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)), "[-]pollServer: Error in reconnecting to server"))
        {
            printf(RED "Retrying in %d seconds...\n" reset, SLEEP_TIME);
            sleep(SLEEP_TIME);
        }

        // Connection established, receive identification ID (Client ID)
        int iRecvStatus;
        printf("[+]Getting ID from Server\n");
        do
        {
            sleep(SLEEP_TIME);
            printf("[-]recv: Error in receiving Client ID.Retrying\n");
            iRecvStatus = recv(sockfd, &iClientID, sizeof(int), 0);
            if(iRecvStatus == 0)
            {
                printf(RED "[-]Client: Connection to server failed\n" reset);
                fprintf(Clientlog, "[-]Client: Connection to server failed [Time Stamp: %f]\n",GetCurrTime(Clock));
                exit(EXIT_FAILURE);
            }
        } while (CheckError(iRecvStatus, "[-]Error in receiving Client ID"));

        printf(GRN "[+]pollServer: Reconnected to the server with ID-%lu\n" reset,iClientID);
        fprintf(Clientlog, "[+]pollServer: Reconnected to the server with ID-%lu [Time Stamp: %f]\n",iClientID,GetCurrTime(Clock));
    }
    else
    {
        // Socket is writable
        fprintf(Clientlog, "[+]pollServer: Server is online [Time Stamp: %f]\n",GetCurrTime(Clock));
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    // Initialize the client Setup the client CLI
    table = createHashTable(FUNCTION_COUNT);

    // Client Side Commands
    insert(table, &Hcmd, "HELP");
    insert(table, &CScmd, "CLEAR");
    insert(table, &Ecmd, "EXIT");
    // Direct Connection Commands
    insert(table, &Rcmd, "READ");
    insert(table, &Wcmd, "WRITE");
    insert(table, &Icmd, "INFO");
    // Server Side Commands
    insert(table, &LScmd, "LIST");
    // Indirect Connection Commands
    insert(table, &Dcmd, "DELETE");
    insert(table, &Ccmd, "CREATE");
    insert(table, &Cpycmd, "COPY");
    insert(table, &Mvcmd, "MOVE");
    insert(table, &Rncmd, "RENAME");

    // Initialize the client log
    Clientlog = fopen("Clientlog.log", "w");
    // Initialize the clock
    Clock = InitClock();

    printf(GRN "[+]Client Initialized\n" reset);
    fprintf(Clientlog, "[+]Client Initialized [Time Stamp: %f]\n",GetCurrTime(Clock));


    // Create a socket
    int iClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(CheckError(iClientSocket, "[-]Error in creating socket"))
    {
        fprintf(Clientlog, "[-]Error in creating socket [Time Stamp: %f]\n",GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    // Specify an address for the socket
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_CLIENT_PORT);
    server_address.sin_addr.s_addr = inet_addr(NS_IP);

    // Connect to the server
    int iConnectionStatus;
    while (CheckError(iConnectionStatus = connect(iClientSocket, (struct sockaddr *)&server_address, sizeof(server_address)), "[-]Error in connecting to server"))
    {
        printf(RED "Retrying...\n" reset);
        sleep(1);
    }

    printf("[+]Connected to the server\n");
    fprintf(Clientlog, "[+]Connected to the server [Time Stamp: %f]\n",GetCurrTime(Clock));

    // Connection established, receive identification ID (Client ID)
    int iRecvStatus  = recv(iClientSocket, &iClientID, sizeof(unsigned long), 0);
    printf("[+]Getting ID from Server\n");
    
    if(CheckError(iRecvStatus, "[-]recv: Error in receiving Client ID"))
    {
        fprintf(Clientlog, "[-]recv: Error in receiving Client ID [Time Stamp: %f]\n",GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    else if(iRecvStatus == 0)
    {
        printf(RED "[-]Client: Connection to server failed\n" reset);
        fprintf(Clientlog, "[-]Client: Connection to server failed [Time Stamp: %f]\n",GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }


    printf(GRN "[+]Connected to the server. Connection ID: %lu\n" reset, iClientID);
    fprintf(Clientlog, "[+]Connected to the server. Connection ID: %lu [Time Stamp: %f]\n", iClientID,GetCurrTime(Clock));
    printf(YEL "[+]Press enter to continue..." reset);
    getchar();

    clearScreen();

    // Setup the input command system for the client

    while (iClientSocket = pollServer(iClientSocket, NS_IP, NS_CLIENT_PORT))
    {
        prompt();
        // Get input from the user
        char cInput[INPUT_SIZE];
        scanf("%[^\n]%*c", cInput);

        // Parse the input
        char *cCommand = strtok(cInput, " ");
        char *cArgs = strtok(NULL, "\0");

        // Execute the command

        functionPointer CMD = lookup(table, cCommand);
        char* Msg = ErrorMsg("Command not found", CMD_ERROR_INVALID_COMMAND);
        if (CheckNull(CMD,Msg ))
        {
            fprintf(Clientlog, "[-]Command not found [Time Stamp: %f]\n",GetCurrTime(Clock));
            continue;
        }

        CMD(cArgs, iClientSocket);
        fflush(Clientlog);
    }

    return 0;
}