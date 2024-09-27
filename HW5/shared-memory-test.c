#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MB(n) ((n) * 1024 * 1024)

int main() {
    printf("Test 1\n");

    pid_t pid;
    int segment_id, segment_id_2;
    int *randNumSharedMemory;
    const int randNumSharedMemorySize = MB(100) / sizeof(int); // Number of integers

    // Integers for in and out indexes
    int *indexSharedMemory;

    // Allocate shared memory segments
    segment_id = shmget(IPC_PRIVATE, randNumSharedMemorySize * sizeof(int), S_IRUSR | S_IWUSR);
    segment_id_2 = shmget(IPC_PRIVATE, 4 * sizeof(int), S_IRUSR | S_IWUSR); // Extra int for exit signal

    // Attach the shared memory segments
    randNumSharedMemory = (int *) shmat(segment_id, NULL, 0);
    indexSharedMemory = (int *) shmat(segment_id_2, NULL, 0);

    // Seed the random number generator
    srand((unsigned int) time(NULL));

    // Initialize in and out indexes
    indexSharedMemory[0] = 0; // In
    indexSharedMemory[1] = 0; // Out
    indexSharedMemory[2] = 0; // Exit signal
    indexSharedMemory[3] = 0; // Child time

    // Generate the child process
    time_t start, end;
    // Track the time it takes to execute the random num task
    start = clock();

    pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork Failed\n");
        return 1;
    } else if (pid == 0) { // Child process
        while (1) {
            int in = indexSharedMemory[0];
            int out = indexSharedMemory[1];

            // Check for exit condition
            if (in == out && indexSharedMemory[2] == 1) {
                break; // Exit if both in and out are the same and exit signal is set
            }

            // Read and print the number if available
            if (in != out) {
                printf("%d ", randNumSharedMemory[out]);
                indexSharedMemory[1] = (out + 1) % randNumSharedMemorySize; // Update out index
            }
        }

        indexSharedMemory[2] = 0; // Reset the exit signal

        printf("\n");

        /* The indexes will be the same after exiting the loop. The Child process 
            becomes a producer and will start writing to the circular buffer.*/

        for (int i = 0; i < randNumSharedMemorySize; i++) {
            int in = indexSharedMemory[0];
            int out = indexSharedMemory[1];

            // Wait for an empty slot (producer logic)
            while (((in + 1) % randNumSharedMemorySize) == out) {
                // Busy waiting (could be improved with a better synchronization mechanism)
            }

            randNumSharedMemory[in] = rand() % 100; // Generate a random number
            indexSharedMemory[0] = (in + 1) % randNumSharedMemorySize; // Update in index
        }

        // Signal completion
        indexSharedMemory[2] = 1; // Set exit signal for the parent

        end = clock();
        double elapsedTime = (end - start) / (CLOCKS_PER_SEC/1000);
        indexSharedMemory[3] = elapsedTime;

    } else { // Parent process
        for (int i = 0; i < randNumSharedMemorySize; i++) {
            int in = indexSharedMemory[0];
            int out = indexSharedMemory[1];

            // Wait for an empty slot (producer logic)
            while (((in + 1) % randNumSharedMemorySize) == out) {
                // Busy waiting (could be improved with a better synchronization mechanism)
            }

            randNumSharedMemory[in] = rand() % 100; // Generate a random number
            indexSharedMemory[0] = (in + 1) % randNumSharedMemorySize; // Update in index
        }

        // Signal completion
        indexSharedMemory[2] = 1; // Set exit signal for the child

        while (indexSharedMemory[2] == 1) 
            ; // Wait till child finishes consuming

        /* The indexes will be the same after exiting the loop. The parent process becomes a consumer
            and waits for the first value that the child process writes.*/

        while (1) {
            int in = indexSharedMemory[0];
            int out = indexSharedMemory[1];

            // Check for exit condition
            if (in == out && indexSharedMemory[2] == 1) {
                break; // Exit if both in and out are the same and exit signal is set
            }

            // Read and print the number if available
            if (in != out) {
                printf("%d ", randNumSharedMemory[out]);
                indexSharedMemory[1] = (out + 1) % randNumSharedMemorySize; // Update out index
            }
        }

        printf("\n");
        end = clock();
        double elapsedTime = (end - start) / (CLOCKS_PER_SEC/1000);
        printf("Parent process took %.2f ms \n", elapsedTime);

        // Wait for child process to finish
        wait(NULL);

        printf("The child process took %.2f ms \n", indexSharedMemory[3]);

        printf("Processed %d integers", randNumSharedMemorySize * 2);

        // Detach from shared memory
        shmdt(randNumSharedMemory);
        shmdt(indexSharedMemory);

        // Remove shared memory segments
        shmctl(segment_id, IPC_RMID, NULL);
        shmctl(segment_id_2, IPC_RMID, NULL);
    }

    return 0;
}
