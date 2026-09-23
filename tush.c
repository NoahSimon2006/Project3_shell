#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "parser.h" // Include the professor's parsing module

#define MAX_LINE 1024 // Maximum characters allowed for a single command input

// Checks if the command is a built-in function that the shell must run itself
static int handle_builtin(char **args)
{
    // If the user types "exit", terminate the shell process
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // If the user types "cd", change the shell's working directory
    // This must be a built-in because chdir only affects the calling process[cite: 7, 8]
    if (strcmp(args[0], "cd") == 0) {
        const char *dir;
        
        // If no directory is provided, default to the HOME environment variable
        if (args[1] == NULL) {
            dir = getenv("HOME");
        } else {
            dir = args[1];
        }

        // Handle errors if HOME isn't set or the directory doesn't exist
        if (dir == NULL) {
            fprintf(stderr, "tush: cd: HOME not set\n");
        } else if (chdir(dir) != 0) {
            perror("tush: cd");
        }
        return 1; // Indicates a built-in was handled
    }
    return 0; // Indicates this is not a built-in command
}

// Creates a new process to run standard commands
static void run_command(const char *path, char **args)
{
    // Create a duplicate of the shell process[cite: 7, 8]
    pid_t pid = fork();

    if (pid < 0) {
        perror("tush: fork"); // Fork failed
        return;
    }

    // Child process branch: where the actual command runs[cite: 7]
    if (pid == 0) {
        execv(path, args); // Replace the child process with the new program[cite: 7, 8]
        perror("tush"); // This line is only reached if execv fails to load the program[cite: 7, 8]
        exit(1);
    }

    // Parent process branch: pause the shell and wait for the child to finish[cite: 7, 8]
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("tush: waitpid");
    }
}

// Searches the PATH environment variable to find the absolute path of a command
char *find_executable_path(const char *command, const char *path_env)
{
    if (path_env == NULL) {
        return NULL;
    }
        
    // Make a copy of PATH because strtok modifies the string it parses[cite: 7]
    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        perror("tush: strdup");
        return NULL;
    }
    
    char candidate[4096]; // Buffer to hold the combined directory and command string
    char *result = NULL;
    
    // Loop through each directory separated by a colon
    char *dir = strtok(path_copy, ":");
    while (dir != NULL) {
        // Glue the directory and command together (e.g., /usr/bin + / + ls)[cite: 7]
        snprintf(candidate, sizeof(candidate), "%s/%s", dir, command);
        
        // Check if the resulting file path exists and is executable[cite: 7, 8]
        if (access(candidate, X_OK) == 0) {
            result = strdup(candidate); // Save the valid path
            break;
        }
        dir = strtok(NULL, ":"); // Move to the next directory
    }
    
    free(path_copy); // Clean up the copy to prevent memory leaks[cite: 7]
    return result;
}

int main(void)
{
    char line[MAX_LINE]; // Buffer to store the raw text the user types

    // Infinite loop to continuously prompt the user for input until they exit[cite: 7, 8]
    while (1) {
        printf("tush> ");
        fflush(stdout); // Forces the prompt to display immediately without waiting

        // Read the user's input from standard input
        // If it returns NULL (pressing Ctrl+D), break the loop to exit
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Parse the input line into a structured format using the provided parser
        CommandLine *cl = parse_command_line(line);
        
        // If the line was empty or just spaces, skip the rest and prompt again
        if (cl == NULL) {
            continue; 
        }

        // print_command_line(cl); // Component 1 output, turned off for the self-check

        char **args = cl->left.argv;
        
        // Only create a new process if the command is not a built-in[cite: 7]
        if (!handle_builtin(args)) {
            const char *path = args[0];
            char *found = NULL;
            
            // If there's no slash in the command, search the PATH for it[cite: 7]
            if (strchr(args[0], '/') == NULL) {
                found = find_executable_path(args[0], getenv("PATH"));
                if (found == NULL) {
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