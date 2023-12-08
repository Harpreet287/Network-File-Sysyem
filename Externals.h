// Common Header Elements common to all code

#ifndef _EXTERNALS_H_
#define _EXTERNALS_H_

// Standard defines
#define LOCAL_MACHINE_IP "127.0.0.1"
#define MAX_BUFFER_SIZE 1024
#define IP_LENGTH 16

// NS IP and Port
#define NS_CLIENT_PORT 8080
#define NS_SERVER_PORT 8081
#define NS_IP LOCAL_MACHINE_IP

// Request and Response Structs

// Request Struct
typedef struct REQUEST_STRUCT
{
} REQUEST_STRUCT;

// Response Struct
typedef struct RESPONSE_STRUCT
{
} RESPONSE_STRUCT;

// Storage-Server Init Struct
typedef struct STORAGE_SERVER_INIT_STRUCT
{
} STORAGE_SERVER_INIT_STRUCT;

// ACK Struct
typedef struct ACK_STRUCT
{
} ACK_STRUCT;


// Function Prototypes
int CheckError(int iStatus, char *sErrorMsg);
int CheckNull(void *ptr, char *sErrorMsg);

#endif // _EXTERNALS_H_