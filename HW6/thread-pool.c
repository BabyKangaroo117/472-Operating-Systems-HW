#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_TASKS 8
#define NUM_THREADS 4

bool finished = false;

typedef struct {
    char *url;
} WebTask;

void* CheckWebsite(WebTask *task);

// Shared mutex among all threads
pthread_mutex_t mutexQueue;

// Shared signal among all threads
pthread_cond_t condQueue;

// Shared task queue among all threads
WebTask taskQueue[MAX_TASKS];

// Shared count among all threads
int taskCount = 0;

void submitTask(WebTask task) {
  pthread_mutex_lock(&mutexQueue);
  taskQueue[taskCount] = task;
  taskCount++;
  pthread_mutex_unlock(&mutexQueue);
  pthread_cond_signal(&condQueue);
}

void* startThread(void* args) {
  while(true) {
    WebTask task;
    pthread_mutex_lock(&mutexQueue);
    // Wait until a task has been submitted
    while(taskCount == 0) {


      if (finished) {
              pthread_mutex_unlock(&mutexQueue);
              pthread_exit(NULL);
      }
      // Release the mutexQueue until condQueue has been signaled then reaquire
      // mutexQueue
      pthread_cond_wait(&condQueue, &mutexQueue);

    }

    // Copy the head of the queue to task
    task = taskQueue[0];
    int i;
    // Shifts the tasks down the queue
    for(i = 0; i < taskCount - 1; i++) {
      taskQueue[i] = taskQueue[i + 1];
    }
    taskCount--;
    pthread_mutex_unlock(&mutexQueue);
    CheckWebsite(&task);

  }
}

void *CheckWebsite(WebTask* task) {

  usleep(500000);
  // Prepare the command to check website response time
  char command[200];
  snprintf(command, sizeof(command), "curl -o /dev/null -s -w '%%{time_total}' %s", task->url);
  FILE *fp = popen(command, "r");
  if (fp == NULL) {
      printf("Failed to run command for %s\n", task->url);
      return NULL;
  }

  double response_time;
  fscanf(fp, "%lf", &response_time);
  pclose(fp);

  // Print the response time
  if (response_time >= 0) {
      printf("%s response time: %.3f seconds\n", task->url, response_time);
  } else {
      printf("%s is not reachable. Response time: infinite\n", task->url);
  }
  return NULL;
}

int main() {
    char *websites[] = {
        "http://www.google.com",
        "http://www.example.com",
        "http://www.facebook.com",
        "http://www.github.com",
        "http://www.stackoverflow.com",
        "http://www.amazon.com",
        "http://www.twitter.com",
        "http://www.microsoft.com"
    };

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);

    // Create threads that enter at the startThread function
    for(int i = 0; i < NUM_THREADS; i++) {
      if(pthread_create(&threads[i], NULL, &startThread, NULL) != 0) {
        perror("Failed to create the thread");
      }
    }

    // Create a webTask and submit to be run by thread
    for (int i = 0; i < MAX_TASKS; i++){
      WebTask task = {
        .url = websites[i]
      };
      submitTask(task);
    }

    // Indicate that no more tasks will be submitted
    pthread_mutex_lock(&mutexQueue);
    finished = true; // Set finished to true
    pthread_cond_broadcast(&condQueue); // Wake all waiting threads
    pthread_mutex_unlock(&mutexQueue);

    // Join threads in a thread pool
    for(int i = 0; i < NUM_THREADS; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
        perror("Failed to join the thread");
      }
    }

    pthread_mutex_destroy(&mutexQueue);
    pthread_cond_destroy(&condQueue);

    return 0;
}
