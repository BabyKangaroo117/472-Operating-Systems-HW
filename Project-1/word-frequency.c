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
#define ALL_WORDS           1  
// #define CHOOSE_WORDS        1
#define NUM_FILES           7
#define NUM_WORDS           100
#define NUM_THREADS         4
#define MAX_FILE_LENGTH     10
#define MAX_WORD_LENGTH     20
#define TOP_50              50
#include <fcntl.h>



void* readFileSection(void* arg);
void* readFile(void *args);
void FindAllWords(FILE *file, long fileSize);
int compare_word_counts(const void* a, const void* b);

typedef struct {
    char *filePath;
    long start;        // Start position for the thread
    long length;      // Length to read
} ThreadData;

// Structure to hold word and count pairs for sorting
typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordCount;


#ifdef CHOOSE_WORDS
    static char g_words[NUM_WORDS][MAX_WORD_LENGTH] = {"the", "be", "it"};
#elif ALL_WORDS
    char g_words[NUM_WORDS][MAX_WORD_LENGTH] = {0};
#endif


static int g_countWords[NUM_WORDS] = {0};
pthread_mutex_t mutexCountWords;

int main () {
    int pipefd[NUM_FILES][2];
    static char filePaths[NUM_FILES][MAX_FILE_LENGTH] = {"bib", "paper1", "paper2", "progc", "progl", "progp", "trans"};
    
    //fcntl(fd, F_SETPIPE_SZ, new_size);

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

            #ifdef ALL_WORDS
            WordCount sorted_words[NUM_WORDS];

            // Fill the array
            for (int i = 0; i < NUM_WORDS; i++) {
                strncpy(sorted_words[i].word, g_words[i], MAX_WORD_LENGTH);
                sorted_words[i].count = g_countWords[i];
            }

            // Sort in descending order
            qsort(sorted_words, NUM_WORDS, sizeof(WordCount), compare_word_counts);

            for (int j = 0; j < TOP_50; j++) {
                write(pipefd[i][1], sorted_words[j].word, MAX_WORD_LENGTH);  // Send word
                write(pipefd[i][1], &sorted_words[j].count, sizeof(int));    // Send count
            }
            return 0;
            #endif

            for (int j = 0; j < NUM_WORDS; j++) {
                write(pipefd[i][1], g_words[j], MAX_WORD_LENGTH);  // Send word
                write(pipefd[i][1], &g_countWords[j], sizeof(int));  // Send count
            }


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


    char word[MAX_WORD_LENGTH];
    int count;

    #ifdef ALL_WORDS
    for (int i = 0; i < NUM_FILES; i++) {
        printf("File: %s\n", filePaths[i]);
        for (int j = 0; j < TOP_50; j++) {
            read(pipefd[i][0], word, MAX_WORD_LENGTH);     // Read word
            read(pipefd[i][0], &count, sizeof(int));       // Read count
            printf("%s: %d\n", word, count);
        }
        printf("\n\n\n");
    }
    return 0;
    #endif

    for (int i = 0; i < NUM_FILES; i++) {
        printf("File: %s\n", filePaths[i]);
        for (int j = 0; j < NUM_WORDS; j++) {
            read(pipefd[i][0], word, MAX_WORD_LENGTH);     // Read word
            read(pipefd[i][0], &count, sizeof(int));       // Read count
            printf("%s: %d\n", word, count);
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

    #ifdef ALL_WORDS
    fseek(file, 0, SEEK_SET);
    FindAllWords(file, fileSize);
    #endif


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

void FindAllWords(FILE *file, long fileSize) {
    char buffer[fileSize + 1];
    // Process buffer word by word
    char* pos = buffer;
    char* word;
    int wordCount = 0;

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    buffer[bytesRead] = '\0';  // Ensure null termination
    // Use strtok to split buffer into words
    word = strtok(pos, " \t\n\r\f\v.,;:!?\"'()[]{}"); // Common delimiters
    while (word != NULL && wordCount < NUM_WORDS) {  // Assuming MAX_WORDS is defined
        // Check if word is already in g_words
        int isDuplicate = 0;
        for (int i = 0; i < wordCount; i++) {
            if (strcmp(g_words[i], word) == 0) {
                isDuplicate = 1;
                break;
            }
        }
        
        // If not a duplicate, add to g_words
        if (!isDuplicate) {

            strcpy(g_words[wordCount], word);
            wordCount++;

        }
        
        word = strtok(NULL, " \t\n\r\f\v.,;:!?\"'()[]{}");
    }
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

// Comparison function for sorting
int compare_word_counts(const void* a, const void* b) {
    return ((WordCount*)b)->count - ((WordCount*)a)->count;
}