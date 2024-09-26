//////////////////////////////// Includes ///////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

////////////////////////////////// Defines ////////////////////////////////

#define MB(n) ((n) * 1024 * 1024)

///////////////////////////// Function Prototypes /////////////////////////

void GenerateRandomIntegers(int *array, size_t size, int min, int max);
void write_int_array_to_buffer(const int *array, size_t size, char *buffer, size_t buffer_size);

//////////////////////////////////// Main ////////////////////////////////

int main () {

    // The identifier for the process
    pid_t pid;
    // The identifier for the shared memory segment
    int segment_id;
    // A pointer to the shared memory segment
    char *sharedMemory;
    const int sharedMemorySize = MB(10);

    // Allocate a shared memory segment
    segment_id = shmget(IPC_PRIVATE, sharedMemorySize, S_IRUSR | S_IWUSR);

    // Attach the shared memory segment
    sharedMemory = (char *) shmat(segment_id, NULL, 0);

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Write integer values to the shared memory buffer
    for (int i = 0; i < sharedMemorySize; i++) {
        sharedMemory[i] = rand() % 100;
    }

    // Generate the child process. Shares the parent address space.
    pid = fork();

    if (pid < 0) { // Error occured
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if (pid == 0) { // Child process
        // Print values from shared memory
        for (int i = 0; i < sharedMemorySize; i++) {
            printf("%d:%d ", i, sharedMemory[i]);
        }
        printf ("\n");

    }
    else { // Parent process
        printf("Inside the parent process \n");
        wait(NULL);

        printf("pid: %d \n", pid);
        
        // Detach from shared memory
        if (shmdt(sharedMemory) == -1) {
            perror("shmdt");
            return 1;
        }

        // Remove shared memory segment
        if (shmctl(segment_id, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            return 1;
        }
    }



    return 0;

}

/////////////////////////////////// Functions ///////////////////////////////