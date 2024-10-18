#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
sem_t mutex, empty, data, consumed;
int myind = 0;
int count = 0;
bool waiting[3];
bool lock;
int mutexVal;
int inLine;
void *producer(void *arg) {
    int id = *(int *)arg;
    int item;
    while (count < 20) {
        item = rand() % 100 + 1; // Generate a random item
        sem_wait(&empty);  // Wait for an empty slot
        sem_wait(&consumed); // Wait for a consumer to consume before another producer gets access
        sem_wait(&mutex); // Get exclusive access to the buffer
        myind++;
        buffer[myind % BUFFER_SIZE] = item; // Add item to the buffer
        printf("Producer %d Produced %d. Buffer: [", id,item);
        for (int j = 0; j < BUFFER_SIZE; j++) {
            printf("%d ", buffer[j]);
        }
        printf("]\n");
        sem_post(&mutex); // Release the mutex
        sem_post(&data); // Notify that a slot is filled
        // Remainder section
        usleep((rand() % 400 + 100) * 1000); // Simulate work
    }
    pthread_exit(NULL);
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    int item;
    while(count < 20) {
        sem_wait(&data); // Wait for a filled slot
        sem_wait(&mutex); // Get exclusive access to the buffer
        
        item = buffer[myind % BUFFER_SIZE]; // Remove and consume the first item
        printf("Consumer %d Consumed %d. Buffer: [", id,item);
        for (int j = 0; j < BUFFER_SIZE; j++) {
            printf("%d ", buffer[j]);
        }
        printf("]\n");
        count++;
        sem_post(&consumed);
        sem_post(&mutex); // Release the mutex
        sem_post(&empty); // Notify that a slot is empty
        // Remainder Section
        usleep((rand() % 400 + 100) * 1000); // Simulate work
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t producers[4], consumers[3];
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&data, 0, 0);
    sem_init(&consumed, 0, 1);
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
    sem_destroy(&empty);
    sem_destroy(&data);

    return 0;
}