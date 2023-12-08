#include "Trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// local helper functions
TrieNode *getNode() // returns a new node
{
    TrieNode *node = (TrieNode *)calloc(1, sizeof(TrieNode));
    return node;
}

int Hash(char *path_token) // returns the index of the child in the children array with given token
{
    if (path_token == NULL)
        return -1;

    // djb2 algorithm
    unsigned long hash = 5381;
    int c;
    while (c = *path_token++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % MAX_CHILDREN;
}
int Recursive_Delete(TrieNode *root) // deletes the subtree for the given node
{
    if (root == NULL)
        return -1;
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (root->children[i] != NULL)
        {
            Recursive_Delete(root->children[i]);
            root->children[i] = NULL;
        }
    }
    free(root);
    return 0;
}
char *Get_Directory_Tree_Full(TrieNode *root, char *cur_dir, int lvl) // returns a string with the full tree path
{
    if (root == NULL)
        return NULL;
    for (int i = 0; i < lvl; i++)
    {
        cur_dir = realloc(cur_dir, strlen(cur_dir) + 2);
        strcat(cur_dir, "-");
    }
    cur_dir = realloc(cur_dir, strlen(cur_dir) + strlen(root->path_token) + 2);
    strcat(cur_dir, root->path_token);
    strcat(cur_dir, "\n");
    // printf("%s (Server Handle: %p)\n",root->path_token,root->Server_Handle);
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (root->children[i] != NULL)
        {
            cur_dir = realloc(cur_dir, strlen(cur_dir) + 2);
            strcat(cur_dir, "|");
            Get_Directory_Tree_Full(root->children[i], cur_dir, lvl + 1);
        }
    }
    return cur_dir;
}

// global functions
/**
 * @brief Initializes the trie
 * @return: The root node of the empty trie
 */
TrieNode *Init_Trie() // returns the root node of the empty trie
{
    TrieNode *root = getNode();
    return root;
}
/**
 * @brief Inserts the path in the trie
 * @param root: The root node of the trie
 * @param path: The path to be inserted
 * @param Server_Handle: The server handle of the path
 * @return: 0 on success, -1 on failure
 */
int Insert_Path(TrieNode *root, char *path, void *Server_Handle)
{
    if (root == NULL || path == NULL || Server_Handle == NULL)
        return -1;

    TrieNode *curr = root;
    char *path_token = strtok(path, "/");

    while (path_token != NULL)
    {
        int index = Hash(path_token);

        // Check if the current node has a valid path_token
        // if (curr->Server_Handle == NULL) {
        //     strcpy(curr->path_token, path_token);
        //     curr->Server_Handle = Server_Handle;
        // }

        if (curr->children[index] == NULL)
        {
            curr->children[index] = getNode();
            strcpy(curr->children[index]->path_token, path_token);
            curr->children[index]->Server_Handle = Server_Handle;
        }

        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }

    // Set the final node's Server_Handle
    curr->Server_Handle = Server_Handle;

    return 0;
}
/**
 * @brief Returns the server handle of the path
 * @param root: The root node of the trie
 * @param path: The path for which the server handle is to be returned
 * @return: The server handle of the path
 * @note: Returns NULL if the path is not present in the trie
 */
void *Get_Server(TrieNode *root, char *path) // returns the server handle of the path
{
    if (root == NULL || path == NULL)
        return NULL;
    TrieNode *curr = root;
    char *path_cpy = (char *)calloc(strlen(path) + 1, sizeof(char));
    strcpy(path_cpy, path);
    char *path_token = strtok(path_cpy, "/");
    path_token = strtok(NULL, "/");
    while (path_token != NULL)
    {
        int index = Hash(path_token);
        if (curr->children[index] == NULL)
            return NULL;
        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }
    free(path_cpy);
    return curr->Server_Handle;
}
/**
 * @brief Deletes the path from the trie
 * @param root: The root node of the trie
 * @param path: The path to be deleted
 * @return: 0 on success, -1 on failure
 * @note: Deletes the subtree for the given path
 */
int Delete_Path(TrieNode *root, char *path) // deletes the path from the trie
{
    if (root == NULL || path == NULL)
        return -1;
    TrieNode *curr = root;
    TrieNode *prev = NULL;
    char *path_token = strtok(path, "/");
    int index;
    while (path_token != NULL)
    {
        index = Hash(path_token);
        if (curr->children[index] == NULL)
            return -1;
        prev = curr;
        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }

    // Delete the subtree for the given path
    prev->children[index] = NULL;
    return Recursive_Delete(curr);
}
/**
 * @brief Deletes the trie
 * @param root: The root node of the trie
 * @return: 0 on success, -1 on failure
 * @note: Deletes the trie recursively
 */
int Delete_Trie(TrieNode *root) // deletes the trie
{
    if (root == NULL)
        return -1;
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (root->children[i] != NULL)
        {
            Delete_Trie(root->children[i]);
        }
    }
    free(root);
    return 0;
}

// helper global functions
/**
 * @brief Prints the trie
 */
void Print_Trie(TrieNode *root, int lvl) // prints the trie
{
    if (root == NULL)
        return;
    for (int i = 0; i < lvl; i++)
        printf("-");
    printf("%s (Server Handle: %p)\n", root->path_token, root->Server_Handle);
    for (int i = 0; i < MAX_CHILDREN; i++)
    {
        if (root->children[i] != NULL)
        {
            printf("|");
            Print_Trie(root->children[i], lvl + 1);
        }
    }
    return;
}
/**
 * @brief Returns a string with subtree path for a given path
 * @param root: The root node of the trie
 * @param path: The path for which the subtree is to be returned
 * @return: A string with subtree path for a given path
 */
char *Get_Directory_Tree(TrieNode *root, char *path) // returns a string with subtree path for a given path
{
    if (root == NULL || path == NULL)
        return NULL;
    // itterate to the node for the given path
    TrieNode *curr = root;
    char *path_cpy = (char *)calloc(strlen(path) + 1, sizeof(char));
    strcpy(path_cpy, path);
    char *path_token = strtok(path_cpy, "/");
    path_token = strtok(NULL, "/");
    while (path_token != NULL)
    {
        int index = Hash(path_token);
        if (curr->children[index] == NULL)
            return NULL;
        curr = curr->children[index];
        path_token = strtok(NULL, "/");
    }
    free(path_cpy);

    // Recursively get the subtree path

    return Get_Directory_Tree_Full(curr, (char *)calloc(1, sizeof(char)), 0);
}
