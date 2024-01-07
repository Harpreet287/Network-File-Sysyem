#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include "./Headers.h"
#include "./Trie.h"
#include "./ErrorCodes.h"
#include "../Externals.h"
#include "../colour.h"

int NS_Write_Socket;
Trie *File_Trie;

FILE *Log_File;
CLOCK *Clock;

/**
 * @brief Initializes the clock object.
 * @return: A pointer to the clock object on success, NULL on failure.
 **/
CLOCK *InitClock()
{
    CLOCK *C = (CLOCK *)malloc(sizeof(CLOCK));
    if (CheckNull(C, "[-]InitClock: Error in allocating memory"))
    {
        fprintf(Log_File, "[-]InitClock: Error in allocating memory \n");
        exit(EXIT_FAILURE);
    }

    C->bootTime = 0;
    C->bootTime = GetCurrTime(C);
    if (CheckError(C->bootTime, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Log_File, "[-]InitClock: Error in getting current time\n");
        free(C);
        exit(EXIT_FAILURE);
    }

    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &C->Btime);
    if (CheckError(err, "[-]InitClock: Error in getting current time"))
    {
        fprintf(Log_File, "[-]InitClock: Error in getting current time\n");
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
double GetCurrTime(CLOCK *Clock)
{
    if (CheckNull(Clock, "[-]GetCurrTime: Invalid clock object"))
    {
        fprintf(Log_File, "[-]GetCurrTime: Invalid clock object\n");
        return -1;
    }
    struct timespec time;
    int err = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    if (CheckError(err, "[-]GetCurrTime: Error in getting current time"))
    {
        fprintf(Log_File, "[-]GetCurrTime: Error in getting current time\n");
        return -1;
    }
    return (time.tv_sec + time.tv_nsec * 1e-9) - (Clock->bootTime);
}


/**
 * @brief Recursive Helper function to populate the trie with the contents of the cwd.
 * @param root: The root node of the trie.
 * @return: 0 on success, -1 on failure.
*/
int Populate_Trie(Trie* root, char* dir)
{
    // Open the directory and add all files and folders to the trie
    struct dirent *entry, **namelist;
    int Num_Entries = scandir(dir, &namelist, NULL, alphasort);
    if (CheckError(Num_Entries, "[-]scandir: Error in getting directory entries"))
    {
        fprintf(Log_File, "[-]scandir: Error in getting directory entries");
        return -1;
    }

    for(int i = 0; i < Num_Entries; i++)
    {
        // Get the name of the file/folder
        char* name = namelist[i]->d_name;

        // Ignore the current and parent directory
        if (strncmp(name, ".",1) == 0 || strncmp(name, "..",2) == 0)
            continue;

        // Get the path of the file/folder
        char path[MAX_BUFFER_SIZE];
        char dir_path[MAX_BUFFER_SIZE];
        memset(path, 0, MAX_BUFFER_SIZE);
        snprintf(path, MAX_BUFFER_SIZE, "%s/%s", dir, name);
        strncpy(dir_path, path, MAX_BUFFER_SIZE);

        // Add the file/folder to the trie
        int err = trie_insert(root, path);
        if (CheckError(err, "[-]Populate_Trie: Error in adding file/folder to trie"))
        {
            fprintf(Log_File, "[-]Populate_Trie: Error in adding file/folder to trie\n");
            return -1;
        }

        // If the entry is a folder, then recursively add it's contents to the trie
        if (namelist[i]->d_type == DT_DIR)
        {
            err += Populate_Trie(root, dir_path);
            if (CheckError(err, "[-]Populate_Trie: Error in populating trie"))
            {
                printf("Error Path: %s\n", path);
                fprintf(Log_File, "[-]Populate_Trie: Error in recursively adding folder contents to trie (Path: %s) [Time Stamp: %f]\n", path ,GetCurrTime(Clock));
            }

        }

        if(err < 0)
        {
            printf("[+]Populate_Trie: Error Detected while populating %d paths\n", -1*err);
            fprintf(Log_File, "[+]Populate_Trie: Error Detected while populating %d paths [Time Stamp: %f]\n", -1*err, GetCurrTime(Clock));
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Initializes the file trie with the contents of the cwd.
 * @return: A pointer to the root node of the trie on success, NULL on failure.
 * @note: The trie is populated with the all contents of the cwd(recursively) 
*/
Trie* Initialize_File_Trie()
{
    // Initialize the trie
    Trie* root =  trie_init();
    if (CheckNull(root, "[-]Initialize_File_Trie: Error in initializing trie"))
    {
        fprintf(Log_File, "[-]Initialize_File_Trie: Error in initializing trie\n");
        return NULL;
    }
    strcpy(root->path_token, "Mount");
    // Get the cwd
    char cwd[MAX_BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        fprintf(Log_File, "[-]Initialize_File_Trie: Error in getting cwd\n");
        return NULL;
    }
    printf("[+]Initialize_File_Trie: Trie initialized at path: %s (CWD)\n", cwd);

    // Populate the trie with the contents of the cwd (recursive)
    int err = Populate_Trie(root, ".");
    if (CheckError(err, "[-]Initialize_File_Trie: Error in populating trie"))
    {
        fprintf(Log_File, "[-]Initialize_File_Trie: Error in populating trie\n");
        return NULL;
    }

    char buffer[MAX_BUFFER_SIZE];
    memset(buffer, 0, MAX_BUFFER_SIZE);
    err = trie_print(root, buffer, 0);
    if (CheckError(err, "[-]Initialize_File_Trie: Error in getting mount paths"))
    {
        fprintf(Log_File, "[-]Initialize_File_Trie: Error in getting mount paths\n");
        return NULL;
    }
    printf("[+]Initialize_File_Trie: Mount Paths Hosted: \n%s\n", buffer);
    return root;
}

/**
 * @brief Thread to listen and handle requests from the Naming Server.
 * @param arg: The port number to listen on.
 * @return: NULL
 **/
void* NS_Listner_Thread(void* arg)
{
    int NSPort = *(int*)arg;

    // Socket for listening to Name Server
    int NS_Listen_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (CheckError(NS_Listen_Socket, "[-]NS_Listner_Thread: Error in creating socket for listening to Name Server"))
    {
        fprintf(Log_File, "[-]NS_Listner_Thread: Error in creating socket for listening to Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    // Address of Socket
    struct sockaddr_in NS_Listen_Addr;
    NS_Listen_Addr.sin_family = AF_INET;
    NS_Listen_Addr.sin_port = htons(NSPort);
    NS_Listen_Addr.sin_addr.s_addr = INADDR_ANY;
    memset(NS_Listen_Addr.sin_zero, '\0', sizeof(NS_Listen_Addr.sin_zero));

    // Bind the socket to the address
    int err = bind(NS_Listen_Socket, (struct sockaddr *)&NS_Listen_Addr, sizeof(NS_Listen_Addr));
    if (CheckError(err, "[-]NS_Listner_Thread: Error in binding socket to address"))
    {
        fprintf(Log_File, "[-]NS_Listner_Thread: Error in binding socket to address [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    err = listen(NS_Listen_Socket, 5);
    if (CheckError(err, "[-]NS_Listner_Thread: Error in listening for connections"))
    {
        fprintf(Log_File, "[-]NS_Listner_Thread: Error in listening for connections [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    // Accept connections
    struct sockaddr_in NS_Client_Addr;
    socklen_t NS_Client_Addr_Size = sizeof(NS_Client_Addr);
    int NS_Client_Socket = accept(NS_Listen_Socket, (struct sockaddr *)&NS_Client_Addr, &NS_Client_Addr_Size);

    char* ns_IP = inet_ntoa(NS_Client_Addr.sin_addr);
    int ns_Port = ntohs(NS_Client_Addr.sin_port);

    if (CheckError(NS_Client_Socket, "[-]NS_Listner_Thread: Error in accepting connections"))
    {
        fprintf(Log_File, "[-]NS_Listner_Thread: Error in accepting connections [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    else if(strncmp(ns_IP, NS_IP, IP_LENGTH) != 0)
    {
        printf(RED"[-]NS_Listner_Thread: Connection Rejected from %s:%d\n"CRESET, ns_IP, ns_Port);
        fprintf(Log_File, "[-]NS_Listner_Thread: Connection Rejected from %s:%d [Time Stamp: %f]\n", ns_IP, ns_Port, GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    printf(GRN"[+]NS_Listner_Thread: Connection Established with Naming Server\n"CRESET);
    fprintf(Log_File, "[+]NS_Listner_Thread: Connection Established with Naming Server [Time Stamp: %f]\n", GetCurrTime(Clock));

    while(1)
    {
        REQUEST_STRUCT NS_Response;
        REQUEST_STRUCT *NS_Response_Struct = &NS_Response;

        // Receive the request from the Name Server
        int err = recv(NS_Client_Socket, NS_Response_Struct, sizeof(RESPONSE_STRUCT), 0);
        if (CheckError(err, "[-]NS_Listner_Thread: Error in receiving data from Name Server"))
        {
            fprintf(Log_File, "[-]NS_Listner_Thread: Error in receiving data from Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
            exit(EXIT_FAILURE);
        }
        else if(err == 0)
        {
            printf(RED"[-]NS_Listner_Thread: Connection with Name Server Closed\n"CRESET);
            fprintf(Log_File, "[-]NS_Listner_Thread: Connection with Name Server Closed [Time Stamp: %f]\n", GetCurrTime(Clock));
            break;
        }

        // Print the request received from the Name Server
        printf(GRN"[+]NS_Listner_Thread: Request Received from Name Server\n"CRESET);
        fprintf(Log_File, "[+]NS_Listner_Thread: Request Received from Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
        printf("Request Operation: %d\n", NS_Response_Struct->iRequestOperation);
        printf("Request Path: %s\n", NS_Response_Struct->sRequestPath);
        printf("Request Flag: %d\n", NS_Response_Struct->iRequestFlags);
        printf("Request Client ID: %lu\n", NS_Response_Struct->iRequestClientID);
    }
    return NULL;
}

/**
 * @brief Thread to listen and handle requests from the Client.
 * @param arg: The port number to listen on.
 * @return: NULL
 **/
void* Client_Listner_Thread(void* arg)
{
    return NULL;
}

/**
 * @brief Thread to flush the logs to the log file.
 * @note: The logs are flushed every LOG_FLUSH_INTERVAL seconds.
*/
void* Log_Flusher_Thread()
{
    while(1)
    {
        sleep(LOG_FLUSH_INTERVAL);
        printf(BBLK"[+]Log Flusher Thread: Flushing logs\n"reset);

        fprintf(Log_File, "[+]Log Flusher Thread: Flushing logs [Time Stamp: %f]\n", GetCurrTime(Clock));
        fprintf(Log_File, "------------------------------------------------------------\n");
        char buffer[MAX_BUFFER_SIZE];
        memset(buffer, 0, MAX_BUFFER_SIZE);
        int err = trie_print(File_Trie, buffer, 0);
        if (CheckError(err, "[-]Log_Flusher_Thread: Error in getting mount paths"))
        {
            fprintf(Log_File, "[-]Log_Flusher_Thread: Error in getting mount paths [Time Stamp: %f]\n", GetCurrTime(Clock));
            exit(EXIT_FAILURE);
        }
        fprintf(Log_File, "%s\n", buffer);
        fprintf(Log_File, "------------------------------------------------------------\n");
        
        fflush(Log_File);
    }
    return NULL;
}

/**
 * @brief Exit handler for the server.
 * @note: destroys the trie and closes the log file.
*/
void exit_handler()
{
    printf(BRED"[-]Server Exiting\n"reset);
    fprintf(Log_File, "[-]Server Exiting [Time Stamp: %f]\n", GetCurrTime(Clock));
    trie_destroy(File_Trie);
    fclose(Log_File);
    return;
}

int main()
{
    printf("Enter (2)Port Number You want to use for Communication:\t");
    int NSPort, ClientPort;
    scanf("%d %d", &NSPort, &ClientPort);

    // Register the exit handler
    atexit(exit_handler);

    // Initialize the log file
    Log_File = fopen("./SSlog.log", "w");
    if (Log_File == NULL)
    {
        fprintf(stderr, "Error opening log file\n");
        exit(1);
    }

    // Initialize the clock
    Clock = InitClock();
    if (CheckNull(Clock, "[-]main: Error in initializing clock"))
    {
        fprintf(Log_File, "[-]main: Error in initializing clock [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    fprintf(Log_File, "[+]Server Initialized [Time Stamp: %f]\n", GetCurrTime(Clock));
    printf("[+]Server Initialized\n");

    // Create a thread to flush the logs periodically
    pthread_t tLogFlusherThread;
    int iThreadStatus = pthread_create(&tLogFlusherThread, NULL, Log_Flusher_Thread, NULL);
    if(CheckError(iThreadStatus, "[-]Error in creating thread")) return 1;

    // Initialize trie for storing and saving all files exposed by the server (includes entire cwd structure)
    // If a folder is exposed, then it's children are also exposed
    // If a file is exposed, then it's path is stored in the trie
    File_Trie = Initialize_File_Trie();
    if (CheckNull(File_Trie, "[-]main: Error in initializing file trie"))
    {
        fprintf(Log_File, "[-]main: Error in initializing file trie [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    // Address to Name Server
    struct sockaddr_in NS_Addr;
    NS_Addr.sin_family = AF_INET;
    NS_Addr.sin_port = htons(NS_SERVER_PORT);
    NS_Addr.sin_addr.s_addr = inet_addr(NS_IP);
    memset(NS_Addr.sin_zero, '\0', sizeof(NS_Addr.sin_zero));
    // Socket for sending data to Name Server
    NS_Write_Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (CheckError(NS_Write_Socket, "[-]main: Error in creating socket for sending data to Name Server"))
    {
        fprintf(Log_File, "[-]main: Error in creating socket for sending data to Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    int err = connect(NS_Write_Socket, (struct sockaddr *)&NS_Addr, sizeof(NS_Addr));
    if (CheckError(err, "[-]main: Error in connecting to Name Server"))
    {
        fprintf(Log_File, "[-]main: Error in connecting to Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    STORAGE_SERVER_INIT_STRUCT Packet;
    STORAGE_SERVER_INIT_STRUCT *SS_Init_Struct = &Packet;

    /* 
    STORAGE_SERVER_INIT_STRUCT *SS_Init_Struct = (STORAGE_SERVER_INIT_STRUCT *)malloc(sizeof(STORAGE_SERVER_INIT_STRUCT));
    if (CheckNull(SS_Init_Struct, "[-]main: Error in allocating memory"))
    {
        fprintf(Log_File, "[-]main: Error in allocating memory [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    */

    SS_Init_Struct->sServerPort_Client = ClientPort;
    SS_Init_Struct->sServerPort_NServer = NSPort;
    char root_path[MAX_BUFFER_SIZE] = "./"; 
    memset(SS_Init_Struct->MountPaths, 0, MAX_BUFFER_SIZE);
    err = trie_paths(File_Trie, SS_Init_Struct->MountPaths, root_path);
    if (CheckError(err, "[-]main: Error in getting mount paths"))
    {
        fprintf(Log_File, "[-]main: Error in getting mount paths [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }

    printf("[+]main: Sent Mounted Paths: \n%s\n", SS_Init_Struct->MountPaths);

    err = send(NS_Write_Socket, SS_Init_Struct, sizeof(STORAGE_SERVER_INIT_STRUCT), 0);
    if (err != sizeof(STORAGE_SERVER_INIT_STRUCT))
    {
        fprintf(Log_File, "[-]main: Error in sending data to Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    fprintf(Log_File, "[+]Initialization Packet Sent to Name Server [Time Stamp: %f]\n", GetCurrTime(Clock));
    // free(SS_Init_Struct);

    printf("[+]Connection Established with Naming Server\n");

    // Setup Listner for Name Server
    pthread_t NS_Listner;
    err = pthread_create(&NS_Listner, NULL, NS_Listner_Thread, (void *)&NSPort);
    if (CheckError(err, "[-]main: Error in creating thread for Name Server Listner"))
    {
        fprintf(Log_File, "[-]main: Error in creating thread for Name Server Listner [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    fprintf(Log_File, "[+]Name Server Listner Thread Created [Time Stamp: %f]\n", GetCurrTime(Clock));

    // Setup Listner for Client
    pthread_t Client_Listner;
    err = pthread_create(&Client_Listner, NULL, Client_Listner_Thread, (void *)&ClientPort);
    if (CheckError(err, "[-]main: Error in creating thread for Client Listner"))
    {
        fprintf(Log_File, "[-]main: Error in creating thread for Client Listner [Time Stamp: %f]\n", GetCurrTime(Clock));
        exit(EXIT_FAILURE);
    }
    fprintf(Log_File, "[+]Client Listner Thread Created [Time Stamp: %f]\n", GetCurrTime(Clock));

    // Wait for the threads to finish
    pthread_join(NS_Listner, NULL);
    pthread_join(Client_Listner, NULL);

    return 0;
}
