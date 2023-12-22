#include "./Trie.h"
#include "./Headers.h"
#include "../Externals.h"

#define PRIME 31
/**
 * @brief Initializes a Reader_Writer_Lock Object
 * @param None
 * @return a pointer to Reader_Writer_Lock_Object
 * @note the lock maintains a service queue to prevent starvation
*/
Reader_Writer_Lock *RW_Lock_Init()
{
    Reader_Writer_Lock *Lock = (Reader_Writer_Lock *)malloc(sizeof(Reader_Writer_Lock));
    Lock->Reader_Count = 0;
    pthread_mutex_init(&Lock->Service_Q_Lock, NULL);
    pthread_mutex_init(&Lock->Read_Init_Lock, NULL);
    pthread_mutex_init(&Lock->Write_Lock, NULL);

    return Lock;    
}
// Reader Accquire Lock
void Read_Lock(Reader_Writer_Lock *Lock)
{
    pthread_mutex_lock(&Lock->Service_Q_Lock);
    pthread_mutex_lock(&Lock->Read_Init_Lock);
    Lock->Reader_Count++;
    if(Lock->Reader_Count == 1)
    {
        pthread_mutex_lock(&Lock->Write_Lock);
    }
    pthread_mutex_unlock(&Lock->Read_Init_Lock);
    pthread_mutex_unlock(&Lock->Service_Q_Lock);
}
// Reader Release Lock
void Read_Unlock(Reader_Writer_Lock *Lock)
{
    pthread_mutex_lock(&Lock->Read_Init_Lock);
    Lock->Reader_Count--;
    if(Lock->Reader_Count == 0)
    {
        pthread_mutex_unlock(&Lock->Write_Lock);
    }
    pthread_mutex_unlock(&Lock->Read_Init_Lock);
}
// Writer Accquire Lock
void Write_Lock(Reader_Writer_Lock *Lock)
{
    pthread_mutex_lock(&Lock->Service_Q_Lock);
    pthread_mutex_lock(&Lock->Write_Lock);
    pthread_mutex_unlock(&Lock->Service_Q_Lock);
}
// Writer Release Lock
void Write_Unlock(Reader_Writer_Lock *Lock)
{
    pthread_mutex_unlock(&Lock->Write_Lock);
}



unsigned int hash(char *path_token)
{
    int hash = 0;
    for(int i = 0; i < strlen(path_token); i++)
    {
        // Use polynomial rolling hash function
        hash = (hash * PRIME + path_token[i]) % MAX_SUB_FILES;
    }
    return hash % MAX_SUB_FILES;
}

/**
 * @brief Initializes a Trie_Node Object to store lock corresponding to paths
 * @param None
 * @return a pointer to Trie_Node_Object
 * @note called as a subroutine of trie_insert
*/
Trie* trie_init()
{
    Trie* File_Trie = (Trie*)malloc(sizeof(Trie));
    File_Trie->path_token[0] = '\0';
    for(int i = 0; i < MAX_SUB_FILES; i++)
    {
        File_Trie->children[i] = NULL;
    }
    File_Trie->Lock = RW_Lock_Init();

    return File_Trie;
}

/**
 * @brief Inserts a path into the trie
 * @param File_Trie the trie to be inserted into
 * @param path the path to be inserted
 * @return 0 on success, -1 on failure
*/
int trie_insert(Trie* File_Trie, char *path)
{
    char *path_token = strtok(path, "/"); 
    // Ignore the first token as it is the cwd
    path_token = strtok(NULL, "/");

    Trie* curr = File_Trie;
    while(path_token != NULL)
    {
        int index = hash(path_token);
        if(curr->children[index] == NULL)
        {
            curr->children[index] = trie_init();
            if(CheckNull(curr->children[index], "trie_insert: Error initializing trie node"))
            {
                fprintf(Log_File, "trie_insert: Error initializing trie node [Time Stamp: %f]\n",GetCurrTime(Clock));
                return -1;
            }
            strncpy(curr->children[index]->path_token, path_token, TOKEN_SIZE);
        }
        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }
    return 0;
}

/**
 * @brief Gets the lock corresponding to a path in the trie
 * @param File_Trie the trie to be searched
 * @param path the path to be inserted
 * @return a pointer to the lock corresponding to the path
 * @note returns NULL if path not found
*/
Reader_Writer_Lock* trie_get_path_lock(Trie* File_Trie, char* path)
{
    char *path_token = strtok(path, "/"); 
    // Ignore the first token as it is the cwd
    path_token = strtok(NULL, "/");

    Trie* curr = File_Trie;
    while(path_token != NULL)
    {
        int index = hash(path_token);
        if(curr->children[index] == NULL)
        {
            return NULL;
        }
        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }
    return &curr->Lock;
}

/**
 * @brief Deletes a path from the trie
 * @param File_Trie the trie to be deleted from
 * @param path the path to be deleted
 * @return 0 on success, -1 on failure
*/
int trie_delete(Trie* File_Trie, char *path)
{
    char *path_token = strtok(path, "/"); 
    // Ignore the first token as it is the cwd
    path_token = strtok(NULL, "/");

    Trie* curr = File_Trie;
    while(path_token != NULL)
    {
        int index = hash(path_token);
        if(curr->children[index] == NULL)
        {
            return -1;
        }
        path_token = strtok(NULL, "/");
        if(path_token == NULL)
        {
            Trie* temp = curr->children[index];
            curr->children[index] = NULL;
            curr = temp;
        }
        else
        {
            curr = curr->children[index];
        }
    }
    
    // Delete all children
    for(int i = 0; i < MAX_SUB_FILES; i++)
    {
        if(curr->children[i] != NULL)
        {
            int status = trie_destroy(curr->children[i]);
            if(CheckError(status, "trie_delete: Error deleting children"))
            {
                fprintf(Log_File, "trie_delete: Error deleting children [Time Stamp: %f]\n",GetCurrTime(Clock));
                return -1;
            }
            curr->children[i] = NULL;
        }
    }
    // Delete the current node
    free(curr);
    return 0;
}

/**
 * @brief Recursively deletes a trie
 * @param File_Trie the trie to be destroyed
 * @return 0 on success, -1 on failure
 * @note This function is called as a subroutine of trie_delete
*/
int trie_destroy(Trie* File_Trie)
{
    // Delete all children
    for(int i = 0; i < MAX_SUB_FILES; i++)
    {
        if(File_Trie->children[i] != NULL)
        {
            int status = trie_destroy(File_Trie->children[i]);
            if(CheckError(status, "trie_destroy: Error deleting children"))
            {
                fprintf(Log_File, "trie_destroy: Error deleting children [Time Stamp: %f]\n",GetCurrTime(Clock));
                return -1;
            }
            File_Trie->children[i] = NULL;
        }
    }
    // Delete the current node
    free(File_Trie);
    return 0;

} 

/**
 * @brief Outputs the trie structure to a buffer
 * @param File_Trie the trie to be printed
 * @param buffer the buffer to be printed to
 * @return 0 on success, -1 on failure
*/
int trie_print(Trie* File_Trie, char* buffer, int level)
{
    if(File_Trie == NULL)
    {
        return 0;
    }
    
    // Print the current node
    int status = 0;
    for(int i = 0; i < level; i++)
    {
        status |= sprintf(buffer, "%s--", buffer);
    }
    status |= sprintf(buffer, "|%s%s\n", buffer, File_Trie->path_token);    
    if(CheckError(status , "trie_print: Error printing to buffer"))
    {
        fprintf(Log_File, "trie_print: Error printing to buffer [Time Stamp: %f]\n",GetCurrTime(Clock));
        return -1;
    }

    for(int i = 0; i < MAX_SUB_FILES; i++)
    {
        if(File_Trie->children[i] != NULL)
        {
            status = trie_print(File_Trie->children[i], buffer, level + 1);
            if(CheckError(status , "trie_print: Error printing to buffer"))
            {
                fprintf(Log_File, "trie_print: Error printing to buffer [Time Stamp: %f]\n",GetCurrTime(Clock));
                return -1;
            }
        }
    }
    return 0;
}
