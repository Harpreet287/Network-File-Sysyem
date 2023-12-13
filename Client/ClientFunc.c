#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

// Custom Header Files
#include "../Externals.h"
#include "../colour.h"
#include "Headers.h"
#include "Hash.h"
#include "ErrorCodes.h"

  

/**
 * @brief Exits the  client after closing the connection
 * @param arg The argument to the command
 * @param ServerSockfd The socket to the server currently connected to
*/
void Ecmd(char* arg, int ServerSockfd)
{
    CheckNull(arg, ErrorMsg("Invalid Argument\nUSAGE: EXIT", CMD_ERROR_INVALID_ARGUMENTS_COUNT));
    fprintf(Clientlog, "[+]Ecmd: Exiting Client [Time Stamp: %f]\n", GetCurrTime(Clock));
    printf("[+]Exiting\n");
    clearScreen();

    printf("Thank you for using this Network File System\n");

    fclose(Clientlog);
    close(ServerSockfd);
    destroyHashTable(table);
    exit(EXIT_SUCCESS);
}
/**
 * @brief prints the Help menu
 * @param arg The argument to the command
 * @param ServerSockfd The socket to the server currently connected to
 * @return void
*/
void Hcmd(char* arg, int ServerSockfd)
{
    if(CheckNull(arg, ErrorMsg("Invalid Argument\nUSAGE: HELP", CMD_ERROR_INVALID_ARGUMENTS_COUNT)))
        return;
    fprintf(Clientlog, "[+]Hcmd: Printing Help Menu [Time Stamp: %f]\n", GetCurrTime(Clock));
    
    printf(GRNHB"=====================HELP MENU==================f==="reset"\n");
    printf(YEL"Avaliable Commands:\n"reset
            BGRN
            "1. READ <Path>: Reads the file at the given path\n"
            "2. WRITE <Flag> <Path>: Writes to the file at the given path. Flag can set to either \'O\': Overwrite or to \'A\': Append\n"
            "3. COPY <Source Path> <Destination Path>: Copies the file(s) from the source path to the destination path (Note: If source path is a Directory, Everthing Under the source path is copied)\n"
            "4. MOVE <Source Path> <Destination Path>: Moves the file(s) from the source path to the destination path (Note: If source path is a Directory, Everthing Under the source path is moved)\n"   
            "5. DELETE <Path>: Deletes the file at the given path (Note: If source path is a Directory, Everthing Under the source path is deleted)\n"
            "6. CREATE <Flag> <Path>: Creates a file at the given path. Flag can set to either \'F\': File or to \'D\': Directory\n" 
            "7. RENAME <Source Path> <Target Name>: Renames the file/directory at the source path to the target name\n"
            "8. INFO <Path>: Prints the information about the file/directory at the given path\n"
            "9. LS <Path>: Lists the contents of the directory at the given path (Note: If no path is provided lists the entire mount directory\n"
            "10. CLEAR: Clears the screen\n"
            "11. HELP: Prints the help menu\n"
            "12. EXIT: Exits the client\n"
            reset);
    printf(GRNHB"=================================================="reset"\n");


    printf("Press any key to continue\n");
    getchar();
    clearScreen();
    return;
}
/**
 * @brief Clears the screen
 * @param arg The argument to the command
 * @param ServerSockfd The socket to the server currently connected to
 * @return void
*/
void CScmd(char* arg, int ServerSockfd)
{
    CheckNull(arg, ErrorMsg("Invalid Argument\nUSAGE: CLEAR", CMD_ERROR_INVALID_ARGUMENTS_COUNT));
    fprintf(Clientlog, "[+]CScmd: clearing screen [Time Stamp: %f]\n", GetCurrTime(Clock));
    clearScreen();
    return;
}