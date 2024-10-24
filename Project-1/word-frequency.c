#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

// Project Params
#define NUM_FILES       7
#define MAX_FILE_LENGTH 10

int main () {

    
    char filePaths[NUM_FILES][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", "progl", "progp", "trans"};
    
    // Create process for each file

    pid_t pid;
    for (int i = 0; i < NUM_FILES; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Process creation failed");
        }
        // Child processes
        else if (pid == 0) {
            

            // Child process shouldn't create any more threads
            return 0;
        }

        // Parent process keeps creating more child processes
    }

    return 0;

}