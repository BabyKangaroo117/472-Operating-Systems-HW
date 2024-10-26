#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 5
#define CONSUMERS   3

int buffer[BUFFER_SIZE];
sem_t mutex, producerControl;
int completed = CONSUMERS;
int myind = 0;
int count = 0;
// Holds the current state of the consumer
int trackConsumers[CONSUMERS];


void *producer(void *arg) {
    int id = *(int *)arg;
    int item;
    while (true) {
        item = rand() % 100 + 1; // Generate a random item
        sem_wait(&producerControl);
        // Once all consumers have recieved message, access the buffer.
        while (completed < CONSUMERS);
        completed = 0;
        sem_wait(&mutex); // Get exclusive access to the buffer
        myind++;
        buffer[myind % BUFFER_SIZE] = item; // Add item to the buffer
        printf("Producer %d Produced %d. Buffer: [", id,item);
        for (int j = 0; j < BUFFER_SIZE; j++) {
            printf("%d ", buffer[j]);
        }
        printf("]\n");
        count++;
        // Allows consumer access to mutex for buffer.
        memset(&trackConsumers, 0, sizeof(int) * CONSUMERS);
        sem_post(&mutex); // Release the mutex
        sem_post(&producerControl);
        // Remainder section
        usleep((rand() % 400 + 100) * 1000); // Simulate work
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    int item;
    while(true) {

        // Blocks itself until there is more data
        while (trackConsumers[id] == 1);
        sem_wait(&mutex); // Get exclusive access to the buffer
        item = buffer[myind % BUFFER_SIZE]; // Remove and consume the first item
        printf("Consumer %d Consumed %d. Buffer: [", id,item);
        for (int j = 0; j < BUFFER_SIZE; j++) {
            printf("%d ", buffer[j]);
        }
        printf("]\n");
        completed++;
        trackConsumers[id] = 1;
        sem_post(&mutex); // Release the mutex
        // Remainder Section
        usleep((rand() % 400 + 100) * 1000); // Simulate work
    }
    pthread_exit(NULL);
}

int main() {
    memset(&trackConsumers, 0, sizeof(int) * CONSUMERS);
    pthread_t producers[4], consumers[3];
    sem_init(&mutex, 0, 1);
    sem_init(&producerControl, 0, 1);
    
    int producer_ids[4] = {0, 1, 2, 3}; // Unique IDs for each producer
    int consumer_ids[3] = {0, 1, 2}; // Unique IDs for each producer


    for (int i = 0; i < 4; i++) {
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);

    }

    for (int i = 0; i < 3; i++) {
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);

    }
    

    for (int i = 0; i < 4; i++) {
        pthread_join(producers[i], NULL);

    }

    for (int i = 0; i < 2; i++) {
        pthread_join(consumers[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&producerControl);

    return 0;
}