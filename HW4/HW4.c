#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

int main() {
    printf("Entry into main \n");

    pid_t child1_pid = fork();

    if (child1_pid == -1) {

        perror("Fork failed");
        return 1;

    // Child process is executing
    } else if (child1_pid == 0) {

        // Replace the child process with the ls command
        execl("/bin/ls", "ls", "-l", (char *)NULL);

        perror("exec failed");

        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Child1 process has completed \n");
    }

    pid_t child2_pid = fork();

    if (child2_pid == -1) {

        perror("Fork failed");
        return 1;

    // Child process is executing
    } else if (child2_pid == 0) {

        // Replace the child process with the date command
        execl("/bin/date", "date", (char *)NULL);

        perror("exec failed");

        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Child2 process has completed \n");
    }


    return 0;

}