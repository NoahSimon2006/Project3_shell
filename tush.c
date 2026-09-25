#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h"

#define MAX_LINE 1024 // 1024 char limit for user input to prevent buffer overflow

// Checks if the command is a built-in function that the shell must run itself
static int handle_builtin(char **args)
{
    // If the user types "exit", terminate the shell process
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // If the user types "cd", change the shell's working directory
    if (strcmp(args[0], "cd") == 0) {
        const char *dir;
        
        // If type "cd" with no args, default to home folder 
        if (args[1] == NULL) {
            dir = getenv("HOME");
        } else {
            dir = args[1];
        }

        // If the dir is NULL, print error; otherwise, attempt to change the directory and print error if it fails
        if (dir == NULL) {
            fprintf(stderr, "tush: cd: HOME not set\n");
        } else if (chdir(dir) != 0) {
            perror("tush: cd");
        }
        return 1; // Indicates a built-in was handled
    }
    return 0; // Indicates this is not a built-in command
}

// Clones the shell, load the new program into the child process, 
// and pause the shell until the child process finishes executing
static void run_command(const char *path, char **args)
{
    // Asks the OS to create a dup of the current process 
    //PID for child is 0, for parent is > 0, and < 0 if fork fails
    pid_t pid = fork();

    if (pid < 0) { 
        perror("tush: fork"); // Error for fork failure
        return;
    }

    // Runs in the child process branch: replace the child with the new program
    if (pid == 0) {
        execv(path, args); // Wipes child's memory and loads the new program into it
        perror("tush"); // If execv returns, it failed, so print an error message
        exit(1);
    }

    // Parent process branch: pause the shell and wait for the child to finish
    // If this didn't exist, the tush> prompt would return in the middle of the ls output
    int status;
    if (waitpid(pid, &status, 0) < 0) { // Wait for the child process to finish and store its exit status
        perror("tush: waitpid");
    }
}

// Using execv requires a strict path, so we need to search the PATH environment variable for the command
char *find_executable_path(const char *command, const char *path_env)
{
    if (path_env == NULL) { // If PATH is not set, we cannot search for the command
        return NULL;
    }
        
    // We need to make a copy of the PATH string because strtok modifies the string it processes
    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        perror("tush: strdup");
        return NULL;
    }
    
    char candidate[4096]; // Buffer to hold full path, 4096 is common max path length
    char *result = NULL; // Pointer to hold the valid path if found
    
    // Loop through each directory separated by a colon
    char *dir = strtok(path_copy, ":");
    while (dir != NULL) { // For each directory in PATH
        // Glues the directory and command together to form a candidate path
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, command);
        
        // Asks kernel if file exist and do I have exec perms
        if (access(candidate, X_OK) == 0) {
            result = strdup(candidate); // Save the valid path 
            break;
        }
        dir = strtok(NULL, ":"); // Move to the next directory
    }
    
    free(path_copy); // Clean up temp string to avoid memory leaks
    return result;
}

int main(void)
{
    char line[MAX_LINE]; // Buffer to store the raw text the user types

    // Infinite loop to continuously prompt the user for input until they exit
    while (1) {
        printf("tush> ");
        fflush(stdout); // Forces the prompt to display immediately without waiting for a newline that tush> doesn't print

        // Read the user's input from standard input
        // If it returns NULL (pressing Ctrl+D), break the loop to exit
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Hands the raw text over to the parser to break it into a structured format
        CommandLine *cl = parse_command_line(line);
        
        // If the line was empty or just spaces, skip the rest and prompt again
        if (cl == NULL) {
            continue; 
        }


        char **args = cl->left.argv; // Get the array of arguments from the parsed command line
        
        // If the command is not a built-in, search for it in the PATH and run it
        if (!handle_builtin(args)) {
            const char *path = args[0];
            char *found = NULL;
            
            // If there's no slash in the command, search the PATH for it
            if (strchr(args[0], '/') == NULL) {
                // If simple command, trigger search loop to find abs path
                found = find_executable_path(args[0], getenv("PATH")); 
                if (found == NULL) { //Error if command is not found
                    fprintf(stderr, "tush: %s: command not found\n", args[0]); 
                }
                path = found;
            }
            
            // If we found a valid path, run the command
            if (path != NULL) {
                run_command(path, args);
            }
            
            free(found); // Clean up the path string
        }

        // Clean up the heap memory allocated by the parser to prevent memory leaks
        free_command_line(cl);
    }
    
    return 0; // Ensures successful exit status when the loop breaks
}