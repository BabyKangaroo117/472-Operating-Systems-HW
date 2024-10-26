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
#define READ_END 0
#define WRITE_END 1

int main () {

    const int SIZE = MB(0.1);
    char writeBufferOne[SIZE];
    char writeBufferTwo[SIZE];
    char readBuffer[SIZE];

    memset(writeBufferOne, 'A', SIZE - 1);
    writeBufferOne[SIZE - 1] = '\0';

    memset(writeBufferTwo, 'B', SIZE - 1);
    writeBufferTwo[SIZE - 1] = '\0';

    pid_t pid;
    int segment_id;
    int fd1[2];
    int fd2[2];
    
    // Hold the memory address of the shared memory
    int *childTime;

    // Allocate shared memory
    segment_id = shmget(IPC_PRIVATE, sizeof(int), S_IRUSR | S_IWUSR);

    // Attach the shared memory segment
    childTime = (int*)shmat(segment_id, NULL, 0);

    // Create pipe 1
    if (pipe(fd1) == -1) {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    // Create pipe 2
    if (pipe(fd2) == -1) {
        fprintf(stderr, "Pipe failed");
        return 1;
    }

    // Generate the child process
    time_t start, end;
    // Track the time it takes to execute the random num task
    start = clock();

    // Fork a child process
    pid = fork();

    if (pid < 0) { // Error occured
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if (pid == 0) { // Child process

        // Read from pipe 1

        // Close the unused end of the pipe
        close(fd1[WRITE_END]);
        // Read from the pipe
        read(fd1[READ_END], &readBuffer, SIZE);
        printf("%s\n ", readBuffer);

        // The pipe signals that it is done reading
        close(fd1[READ_END]);

        // Write to pipe 2

        // Close the unused end of the pipe
        close(fd2[READ_END]);

        // Write to the pipe
        write(fd2[WRITE_END], writeBufferTwo, SIZE);
    
        // The pipe signals that it is done writing
        close(fd2[WRITE_END]);

        end = clock();

        // Save the child time to be printed at end of both child and parent process
        *childTime = (end - start) / (CLOCKS_PER_SEC / 1000);

    }
    else { // Parent process

        // Write to pipe 1

        // Close the unused end of the pipe
        close(fd1[READ_END]);

        // Write to the pipe
        write(fd1[WRITE_END], writeBufferOne, SIZE);
     

        // The pipe signals that it is done writing
        close(fd1[WRITE_END]);

        // Read from pipe 2

        // Close the unused end of the pipe
        close(fd2[WRITE_END]);
    
        // Read from the pipe
        
        read(fd2[READ_END], readBuffer, SIZE);
        printf("%s\n ", readBuffer);
        

        // The pipe signals that it is done reading
        close(fd2[READ_END]);

        end = clock();
        double elapsedTime = (end - start) / (CLOCKS_PER_SEC / 1000);
        printf("\n");

        printf("Parent process took %.2f ms \n", elapsedTime);

        // Wait for child process to finish
        wait(NULL);

        printf("The child process took %.2f ms \n", *childTime);

        // Detach from shared memory
        shmdt(childTime);
    
        // Remove shared memory segments
        shmctl(segment_id, IPC_RMID, NULL);


    }
    return 0;
}