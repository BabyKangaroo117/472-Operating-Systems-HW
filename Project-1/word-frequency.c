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

#define NUM_FILE_PATHS  7
#define MAX_FILE_LENGTH 10
#define NUM_WORDS       2
#define MAX_WORD_LENGTH 20
#define MAX_LINE_LENGTH 512
#define MAX_WORDS       100

typedef struct {
    const char *filePath;
    const char *word;
} ThreadParams;

int processText(const char *filePath, const char words[][MAX_WORD_LENGTH], uint8_t wordCount);
void* countWords(void *args);
int createThreadParams(ThreadParams ***threadParamsList, const char words[][MAX_WORD_LENGTH], const char *filePath, int wordCount);


int main() {
    pid_t pid;
    const char filePaths[NUM_FILE_PATHS][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", 
                                                  "progl", "progp", "trans"};
    printf("Check 1\n");
    const char words[NUM_WORDS][MAX_WORD_LENGTH] = {"the", "hello"};

    for (int i = 0; i < NUM_FILE_PATHS; i++) {
        pid = fork();
        if (pid < 0) {
            // Fork failed
            perror("Fork failed");
            return 1;
        } else if (pid == 0) {
            // Child process
            printf("Child Process (PID: %d) - Path number: %d\n", getpid(), i);
            processText(filePaths[i], words, NUM_WORDS);
            return 0; // Child should exit here to avoid forking again
        }
         
    }

    // Wait for each process to finish
    for (int i = 0; i < NUM_FILE_PATHS; i++) {
        wait(NULL);
    }
    printf("Process exiting\n");
    return 0;

}

int processText(const char *filePath, const char words[][MAX_WORD_LENGTH], uint8_t wordCount) {
    pthread_t threadIDS[wordCount];
    
    ThreadParams **threadParamsList;
    createThreadParams(&threadParamsList, words, filePath, wordCount);
    for (int i = 0; i < wordCount; i++) {
        pthread_create(&threadIDS[i], NULL, countWords, threadParamsList[i]);
    }

    for (int i = 0; i < wordCount; i++) {
        pthread_join(threadIDS[i], NULL);
    }
    
    


   
    return 0;
}

int createThreadParams(ThreadParams ***threadParamsList, const char words[][MAX_WORD_LENGTH], const char *filePath, int wordCount) {
    
    *threadParamsList = malloc(wordCount * sizeof(ThreadParams *));
    if (threadParamsList == NULL) {
        perror("Failed to allocate memory for thread params list");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < wordCount; i++) {
        // Allocate memory for ThreadParams
        (*threadParamsList)[i] = malloc(sizeof(ThreadParams));
        if ((*threadParamsList)[i] == NULL) {
            perror("Failed to allocate memory for thread params");
            return -1; // Handle memory allocation failure
        }

        // Set parameters for the thread
        (*threadParamsList)[i]->filePath = filePath;
        (*threadParamsList)[i]->word = words[i];
    }

    return 0;

}

int finished;
void* countWords(void *args) {
    ThreadParams* threadParams = (ThreadParams*)args;
    char line[MAX_LINE_LENGTH];
    int wordCount = 0;
    FILE *file = fopen(threadParams->filePath, "r");
    if (file == NULL) { // Check if the file was opened successfully
        perror("Failed to open file");
    }

    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " \n");
        while (token != NULL) {
            if (strcmp(token, threadParams->word) == 0) {
                wordCount++;
            }
            token = strtok(NULL, " \n");
        }
    }
    finished++;
    while (finished != NUM_WORDS);
    fclose(file);
    printf("Document: %s, Word: %s, Count: %d\n", threadParams->filePath, threadParams->word, wordCount);
    free(threadParams);
    pthread_exit(NULL);
    
}
