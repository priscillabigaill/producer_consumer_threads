#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define LOWER_NUM 1
#define UPPER_NUM 10000
#define BUFFER_SIZE 100
#define MAX_COUNT 10000

int buffer[BUFFER_SIZE];
int buffer_index = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int producer_finished = 0;

// producer function -> generates the numbers and write them into all.txt
void *producer(void *arg) {
    clock_t start_time = clock();
    FILE *file = fopen("all.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_COUNT; i++) {
        int num = LOWER_NUM + rand() % (UPPER_NUM - LOWER_NUM + 1);
        pthread_mutex_lock(&lock); // acquire lock to access shared buffer
        while (buffer_index == BUFFER_SIZE) { // if buffer is full
            pthread_cond_wait(&cond, &lock);
        }
        buffer[buffer_index++] = num; // add num to buffer
        fprintf(file, "%d\n", num); // write num to file
        pthread_cond_signal(&cond); // signal consumers that new data is available
        pthread_mutex_unlock(&lock); // release lock
    }

    fclose(file);
    pthread_mutex_lock(&lock);
    producer_finished = 1; // indicate completion
    pthread_mutex_unlock(&lock);
    clock_t end_time = clock();
    printf("Producer thread execution time: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}

// consumer function -> read numbers from buffer and write them to correct file (even nums)
void *consumer_even(void *arg) {
    FILE *file = fopen("even.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&lock); // acquire lock
        while (!producer_finished && buffer_index == 0) { // if buffer empty, wait for producer
            pthread_cond_wait(&cond, &lock);
        }
        if (buffer_index > 0 && buffer[buffer_index - 1] % 2 == 0) { // check if last number in buffer is even
            fprintf(file, "%d\n", buffer[--buffer_index]); // write even num to file and remove from buffer
        }
        pthread_cond_signal(&cond); // signal producer tht data has been consumed
        pthread_mutex_unlock(&lock); // release lock
        if (producer_finished && buffer_index == 0) { // when producer is finished & buffer is empty
            break;
        }
    }

    fclose(file);
    pthread_exit(NULL);
}

// consumer function -> read numbers from buffer and write them to correct file (odd nums)
void *consumer_odd(void *arg) {
    FILE *file = fopen("odd.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        pthread_mutex_lock(&lock); // acquire lock
        while (!producer_finished && buffer_index == 0) { // if buffer empty, wait for producer
            pthread_cond_wait(&cond, &lock);
        }
        if (buffer_index > 0 && buffer[buffer_index - 1] % 2 != 0) { // check if last number in buffer is odd
            fprintf(file, "%d\n", buffer[--buffer_index]); // write odd num to file and remove from buffer
        }
        pthread_cond_signal(&cond); // signal producer tht data has been consumed
        pthread_mutex_unlock(&lock); // release lock
        if (producer_finished && buffer_index == 0) { // when producer is finished & buffer is empty
            break;
        }
    }

    fclose(file);
    pthread_exit(NULL);
}

int main() {
    pthread_t producer_thread, consumer_even_thread, consumer_odd_thread;
    srand(time(NULL)); // seed random number generator with current time

    clock_t start_total_time = clock();

    // create threads for producer & consumers
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_even_thread, NULL, consumer_even, NULL);
    pthread_create(&consumer_odd_thread, NULL, consumer_odd, NULL);

    // wait for all threads to finish
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_even_thread, NULL);
    pthread_join(consumer_odd_thread, NULL);

    clock_t end_total_time = clock();
    printf("Program completed successfully.\n");
    printf("Total execution time: %f seconds\n", (double)(end_total_time - start_total_time) / CLOCKS_PER_SEC);

    return 0;
}
