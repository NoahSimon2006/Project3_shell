#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h" // Include the professor's parsing module

#define MAX_LINE 1024 // Maximum characters allowed for a single command input

int main(void)
{
    char line[MAX_LINE]; // Buffer to store the raw text the user types

    // The core REPL (Read-Eval-Print Loop) that keeps the shell running
    while (1) {
        printf("tush> "); 
        fflush(stdout); // Forces the prompt to display immediately without waiting

        // Read the user's input from standard input
        // If it returns NULL (e.g., user presses Ctrl+D for EOF), break the loop to exit
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // Pass the raw string to the provided parser to break it into commands and arguments
        CommandLine *cl = parse_command_line(line);
        
        // If the line was empty or just spaces, skip the rest and prompt again
        if (cl == NULL) {
            continue; 
        }

        // Print out the parsed structure as a sanity check for Component 1
        // (This will be commented out later when we actually run commands so the grader doesn't fail)
        print_command_line(cl);

        // Clean up the heap memory allocated by the parser to prevent memory leaks
        free_command_line(cl);
    }
    
    return 0; // Ensures successful exit status when the loop breaks
}