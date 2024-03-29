// 2. Write a multi-threaded application to process a really large file which has integer
// numbers stored in it. Each thread should work on a separate section of the file and
// should find out unique numbers from that section and add it to the global unique
// number list. Each thread keeps adding the unique numbers from its section to the
// global list. Make sure the unique number should not repeat in the global list.
// At the end of processing the whole file, print the list of unique numbers from the
// global list.


//Solution

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREADS 4
#define MAX_NUMBERS 1000000

int numbers[MAX_NUMBERS];
int uniqueNumbers[MAX_NUMBERS];
int uniqueCount = 0;

typedef struct {
    int start;
    int end;
} ThreadArgs;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* processNumbers(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int start = args->start;
    int end = args->end;
    int i,j;
    for (i = start; i < end; i++) {
        int number = numbers[i];
        int isUnique = 1;

        // Checking if the number is already in the local unique list
        for (j = 0; j < uniqueCount; j++) {
            if (uniqueNumbers[j] == number) {
                isUnique = 0;
                break;
            }
        }

        // If not found, adding it to the local unique list and the global unique list
        if (isUnique) {
            if (pthread_mutex_lock(&mutex) != 0) {
                perror("Error locking mutex");
                exit(EXIT_FAILURE);
            }

            uniqueNumbers[uniqueCount++] = number;

            if (pthread_mutex_unlock(&mutex) != 0) {
                perror("Error unlocking mutex");
                exit(EXIT_FAILURE);
            }
        }
    }

    pthread_exit(NULL);
}

int main() {
    
    FILE* file = fopen("integers_list.txt", "r");
    if (file == NULL) {
        printf("Error opening file %s",file);
        return EXIT_FAILURE;
    }

    
    int i = 0;
    while (fscanf(file, "%d", &numbers[i]) == 1 && i < MAX_NUMBERS) {
        i++;
    }

    if (ferror(file)) {
        printf("Error reading from file %s", file);
        fclose(file);
        return EXIT_FAILURE;
    }

    // Calculate the section size for each thread
    int fileSize = i;  // Adjust the file size based on the actual number of elements read
    int sectionSize = fileSize / MAX_THREADS;

    // Create an array of thread IDs
    pthread_t threads[MAX_THREADS];

    // Create thread arguments
    ThreadArgs threadArgs[MAX_THREADS];

  
    for (i = 0; i < MAX_THREADS; i++) {
        threadArgs[i].start = i * sectionSize;
        threadArgs[i].end = (i == MAX_THREADS - 1) ? fileSize : (i + 1) * sectionSize;

        if (pthread_create(&threads[i], NULL, processNumbers, (void*)&threadArgs[i]) != 0) {
            perror("Error creating thread");
            fclose(file);
            return EXIT_FAILURE;
        }
    }

    // Waiting for all threads to finish
    for ( i = 0; i < MAX_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            fclose(file);
            return EXIT_FAILURE;
        }
    }

    // Printing the unique numbers
    for (i = 0; i < uniqueCount; i++) {
        printf("Unique Numbers List\n");
        printf("%d\n", uniqueNumbers[i]);
    }

   
    if (fclose(file) != 0) {
        perror("Error closing file");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

