#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define NUM_FILE_PATHS  7
#define MAX_FILE_LENGTH 10
#define NUM_WORDS       2
#define MAX_WORD_LENGTH 20

int processText(const char *filePath, uint8_t pathLength, char words[][MAX_WORD_LENGTH], uint8_t wordCount);

int main() {
    int pid[NUM_FILE_PATHS];
    char filePaths[NUM_FILE_PATHS][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", 
                                                  "progl", "progp", "trans"};

    char words[NUM_WORDS][MAX_WORD_LENGTH] = {"Hello", "the"};

    for (int i = 0; i < NUM_FILE_PATHS; i++) {
        pid[i] = fork();
        if (pid[i] == 0) {
            processText(filePaths[i], MAX_FILE_LENGTH, words, NUM_WORDS);
            return 0;
        }
        
        
    }

    return 0;

}

int processText(const char *filePath, uint8_t pathLength, char words[][MAX_WORD_LENGTH], uint8_t wordCount) {
    
    FILE *file = fopen(filePath, "r");
    if (file == NULL) { // Check if the file was opened successfully
        perror("Failed to open file");
        return 1;
    }
    
}