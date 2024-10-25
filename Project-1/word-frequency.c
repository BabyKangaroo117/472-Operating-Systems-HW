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
#include <ctype.h>

// Project Params
#define NUM_FILES              7
#define NUM_WORDS              3
#define NUM_THREADS            1
#define MAX_FILE_LENGTH        10
#define MAX_WORD_LENGTH        10

void* readFileSection(void* arg);
void* readFile(void *args);

typedef struct {
    char *filePath;
    long start;        // Start position for the thread
    long length;      // Length to read
} ThreadData;

static char g_words[NUM_WORDS][MAX_WORD_LENGTH] = {"the", "be", "it"};
static int g_countWords[NUM_WORDS] = {0};
pthread_mutex_t mutexCountWords;

int main () {
    int pipefd[NUM_FILES][2];
    static char filePaths[NUM_FILES][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", "progl", "progp", "trans"};
    
    // Create a pipe for each process
    for (int i = 0; i < NUM_FILES; i++) {
        if (pipe(pipefd[i]) == -1) {
            perror("fork failed");
            exit(1);
        }
    }

    // Create a process for each file
    pid_t pid;
    for (int i = 0; i < NUM_FILES; i++) {
        pid = fork();
        if (pid < 0) {
            printf("Process creation failed");
        }
        // Child processes
        else if (pid == 0) {
            pthread_t readThread;
        
            pthread_mutex_init(&mutexCountWords, NULL);
            pthread_create(&readThread, NULL, readFile, filePaths[i]);
            pthread_join(readThread, NULL);

            // Close the read end of the pipe
            close(pipefd[i][0]);

            write(pipefd[i][1], g_countWords, sizeof(int) * NUM_WORDS);

            // Child process shouldn't create any more threads
            return 0;
        }
        // Parent Process

        // Close write end of pipe
        close(pipefd[i][1]);
    }

    // First parent process that each child branched from

    for (int i = 0; i < NUM_FILES; i++) {
        wait(NULL);
    }

    // Read pipe of each child process
    int readBuffer[NUM_WORDS] = {0};
    for (int i = 0; i < NUM_FILES; i++) {
        read(pipefd[i][0], readBuffer, sizeof(int) * NUM_WORDS);
        printf("File: %s\n", filePaths[i]);
        for (int j = 0; j < NUM_WORDS; j++) {
            printf("%s: %d\n", g_words[j], readBuffer[j]);
        }
    }

    return 0;

}

void* readFile(void *args) {
    char *filePath = (char*)args;
    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fclose(file);

    // Size of each section
    long sectionSize = fileSize / NUM_THREADS; 
    pthread_t threads[NUM_THREADS];
    ThreadData threadData[NUM_THREADS];

    // Split file into a section for each thread then create threads to count words
    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].filePath = filePath;
        threadData[i].start = i * sectionSize;
        threadData[i].length = (i == NUM_THREADS - 1) ? (fileSize - threadData[i].start) : sectionSize; // Last thread takes the remainder
        pthread_create(&threads[i], NULL, readFileSection, &threadData[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

void* readFileSection(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int countWords[NUM_WORDS] = {0};
    FILE *file = fopen(data->filePath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Allocate buffer for the section
    char *buffer = malloc(data->length + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    // Seek to the start position
    if (fseek(file, data->start, SEEK_SET) != 0) {
        perror("Error seeking in file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Read the section
    size_t bytesRead = fread(buffer, 1, data->length, file);
    buffer[bytesRead] = '\0';  // Ensure null termination

    // Process buffer
    for (int i = 0; i < NUM_WORDS; i++) {
        char* pos = buffer;
        while ((pos = strstr(pos, g_words[i])) != NULL) {
            // Check if word boundary before
            int validStart = (pos == buffer) || isspace(*(pos - 1)) || ispunct(*(pos - 1));
            
            // Check if word boundary after
            char* wordEnd = pos + strlen(g_words[i]);
            int validEnd = (*wordEnd == '\0') || isspace(*wordEnd) || ispunct(*wordEnd);
            
            if (validStart && validEnd) {
                countWords[i]++;
            }
            pos++; // Move to next character to continue search
        }
    }

    // Combine word counts from each thread
    pthread_mutex_lock(&mutexCountWords);
    for (int i = 0; i < NUM_WORDS; i++) {
        g_countWords[i] += countWords[i];
    }
    pthread_mutex_unlock(&mutexCountWords);

    free(buffer);
    fclose(file);
    return NULL;
}