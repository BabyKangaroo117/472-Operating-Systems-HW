#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MB(n) ((n) * 1024 * 1024)

int main() {

    pid_t pid;
    int segment_id, segment_id_2;
    char *messageSharedMemory;
    const int SIZE = MB(3);
    char messageBufferOne[SIZE];
    char messageBufferTwo[SIZE];

    memset(messageBufferOne, 'A', SIZE - 1);
    messageBufferOne[SIZE - 1] = '\0';

    memset(messageBufferTwo, 'B', SIZE - 1);
    messageBufferOne[SIZE - 1] = '\0';

    // Integers for in and out indexes
    int *indexSharedMemory;

    // Allocate shared memory segments
    segment_id = shmget(IPC_PRIVATE, SIZE, S_IRUSR | S_IWUSR);
    segment_id_2 = shmget(IPC_PRIVATE, 4 * sizeof(int), S_IRUSR | S_IWUSR); // Extra int for exit signal

    // Attach the shared memory segments
    messageSharedMemory = shmat(segment_id, NULL, 0);
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

        // Write a message to memory
        sprintf(messageSharedMemory, "%s", messageBufferOne);
         
        // Finished writing
        indexSharedMemory[2] = 1;

        while(indexSharedMemory[2] == 1)
            ; // Wait till parent is done writing

        // Read a message from memory
        printf("%s", messageSharedMemory);

        end = clock();
        double elapsedTime = (end - start) / (CLOCKS_PER_SEC/1000);
        indexSharedMemory[3] = elapsedTime;

    } else { // Parent process
    
        while(indexSharedMemory[2] == 0) 
            ; // Wait till child is finished writing

        printf("%s", messageSharedMemory);

        // Write a message to memory
        sprintf(messageSharedMemory, "%s", messageBufferTwo);

        // Finished writing
        indexSharedMemory[2] = 0;
        

        end = clock();
        double elapsedTime = (end - start) / (CLOCKS_PER_SEC/1000);
        
        // Wait for child to finish
        wait(NULL);

        printf("\n");
        printf("Parent process took %.2f ms \n", elapsedTime);

        printf("The child process took %.2f ms \n", indexSharedMemory[3]);

        // Detach from shared memory
        shmdt(messageSharedMemory);
        shmdt(indexSharedMemory);

        // Remove shared memory segments
        shmctl(segment_id, IPC_RMID, NULL);
        shmctl(segment_id_2, IPC_RMID, NULL);
    }

    return 0;
}
