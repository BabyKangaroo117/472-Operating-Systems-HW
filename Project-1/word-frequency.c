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
    char *filePath;
    char (*words)[MAX_WORD_LENGTH];
    int lockID;
} ThreadParams;

int processText(char *filePath, char words[][MAX_WORD_LENGTH], uint8_t wordCount);
void* distributeLines(void *args);
void* countWords(void *args);


int main() {
    pid_t pid;
    char filePaths[NUM_FILE_PATHS][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", 
                                                  "progl", "progp", "trans"};
    printf("Check 1\n");
    char words[NUM_WORDS][MAX_WORD_LENGTH] = {"the", "Hello"};

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

pthread_mutex_t mutexCheckIn;

int processText(char *filePath, char words[][MAX_WORD_LENGTH], uint8_t wordCount) {
    
    ThreadParams threadParams = {filePath, words};
    pthread_t threadID;
    pthread_mutex_init(&mutexCheckIn, NULL);
    pthread_create(&threadID, NULL, distributeLines, &threadParams);
    if (pthread_join(threadID, NULL) != 0) {
        perror("Failed to join the thread");
      }

    return 0;
}

char line[MAX_LINE_LENGTH];
int finishedThreads;
int finishedReading;
int locks[NUM_WORDS];
void* distributeLines(void *args) {
    
    ThreadParams *threadParams = (ThreadParams*)args;

    pthread_t wordThreads[NUM_WORDS];

    for (int i = 0; i < NUM_WORDS; i++) {
        threadParams->lockID = i;
        pthread_create(&wordThreads[i], NULL, countWords, threadParams);

    }


    FILE *file = fopen(threadParams->filePath, "r");
    if (file == NULL) { // Check if the file was opened successfully
        perror("Failed to open file");
    }
    finishedThreads = NUM_WORDS;
    finishedReading = 0;
    while (fgets(line, sizeof(line), file)) {
        while (finishedThreads < NUM_WORDS);
        finishedThreads = 0;
    }
   
    while (finishedThreads < NUM_WORDS);
    finishedReading = 1;

    
    for (int i = 0; i < NUM_WORDS; i++) {
        if (pthread_join(wordThreads[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }

}

void* countWords(void *args) {
    ThreadParams *threadParams = (ThreadParams*)args;
    int lock = threadParams->lockID;
    char *word = threadParams->words[lock];
    int wordCount = 0;
    int loops = 0;
   

        while (true) {

            while (unlock != 1 && finishedReading != 1);
            if (finishedReading == 1) {
                break;
            } 

            while(locks[lock])

            char *token = strtok(line, " \n");
            while (token != NULL) {
                if (strcmp(token, word) == 0) {
                    wordCount++;
                }
                token = strtok(NULL, " \n");
            }
            pthread_mutex_lock(&mutexCheckIn);
            finishedThreads++;
            pthread_mutex_unlock(&mutexCheckIn);

            loops++;
        }

    printf("Word: %s Count: %d, Loops: %d\n", word, wordCount, loops);

   

    pthread_exit(NULL);
    
}
