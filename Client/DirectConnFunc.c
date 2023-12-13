#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

// Custom Header Files
#include "../Externals.h"
#include "../colour.h"
#include "./Headers.h"
#include "./Hash.h"
#include "./ErrorCodes.h"

void Rcmd(char* arg, int ServerSockfd)
{
    if(CheckNull(arg, ErrorMsg("Invalid Argument\nUSAGE: READ <Path>", CMD_ERROR_INVALID_ARGUMENTS)))
    {
        fprintf(Clientlog, "[-]Rcmd: Invalid Argument [Time Stamp: %f]\n", GetCurrTime(Clock));
        return;
    }

    arg = strtok(arg, " \t\n");

    if(CheckNull(strtok(NULL, " \t\n"), ErrorMsg("Invalid Argument Count\nUSAGE: READ <Path>", CMD_ERROR_INVALID_ARGUMENTS_COUNT)))
    {
        fprintf(Clientlog, "[-]Rcmd: Invalid Argument Count [Time Stamp: %f]\n", GetCurrTime(Clock));
        return;
    }

    char* path = arg;
    fprintf(Clientlog, "[+]Rcmd: Reading Path %s [Time Stamp: %f]\n", path, GetCurrTime(Clock));

    REQUEST_STRUCT* req = (REQUEST_STRUCT*)(sizeof(REQUEST_STRUCT));
    memset(req, 0, sizeof(REQUEST_STRUCT));

    req->iRequestOperation = CMD_READ;
    req->iRequestClientID = iClientID;
    strncpy(req->sRequestPath, path, MAX_BUFFER_SIZE);
    // req->iRequestFlags = 0;

    if(CheckError(send(ServerSockfd, req, sizeof(REQUEST_STRUCT), 0), ErrorMsg("Failed to send request", CMD_ERROR_SEND_FAILED)))
    {
        fprintf(Clientlog, "[-]Rcmd: Failed to send request [Time Stamp: %f]\n", GetCurrTime(Clock));
        return;
    }
    

}
void Wcmd(char* arg, int ServerSockfd)
{
    return;
}
void Icmd(char* arg, int ServerSockfd)
{
    return;
}