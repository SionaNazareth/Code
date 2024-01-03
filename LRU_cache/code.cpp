#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_PATH_SIZE 256

typedef struct {
    char filepath[MAX_PATH_SIZE];
    time_t timestamp;
    size_t size;
    ino_t inode;
} FileMetadata;

typedef struct Node {
    FileMetadata data;
    struct Node* prev;
    struct Node* next;
} Node;

typedef struct {
    int capacity;
    int size;
    Node* head;
    Node* tail;
} LRUCache;

Node* createNode(const char* filepath) {
    struct stat fileStat;
    if (stat(filepath, &fileStat) == -1) {
        printf("Error getting file information of %s", filepath);
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(fileStat.st_mode)) {
        fprintf(stderr, "Error: '%s' is a directory.\n", filepath);
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        printf("Error opening file %s",file);
        exit(EXIT_FAILURE);
    }

    if (fseek(file, 0, SEEK_END) == -1) {
        printf("Error seeking file %s",file);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    long fileSize = ftell(file);
    if (fileSize == -1) {
        printf("Error getting file size of %s",file);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (fseek(file, 0, SEEK_SET) == -1) {
        printf("Error seeking file %s",file);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (fclose(file) == EOF) {
        perror("Error closing file");
        exit(EXIT_FAILURE);
    }

    if (fileSize == 0) {
        fprintf(stderr, "Warning: File size is 0 for file %s\n", filepath);
    }

    FileMetadata data;
    strncpy(data.filepath, filepath, MAX_PATH_SIZE - 1);
    data.filepath[MAX_PATH_SIZE - 1] = '\0';
    data.timestamp = time(NULL);
    data.size = (size_t)fileSize;
    data.inode = fileStat.st_ino;

    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    newNode->data = data;
    newNode->prev = NULL;
    newNode->next = NULL;

    return newNode;
}

LRUCache* createLRUCache(int capacity) {
    LRUCache* cache = (LRUCache*)malloc(sizeof(LRUCache));
    if (cache == NULL) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = NULL;
    cache->tail = NULL;
    return cache;
}

Node* searchLRUEntry(LRUCache* cache, const char* filepath) {
    Node* current = cache->head;
    while (current != NULL) {
        if (strcmp(current->data.filepath, filepath) == 0) {
            if (current->prev != NULL) {
                current->prev->next = current->next;
            }
            if (current->next != NULL) {
                current->next->prev = current->prev;
            }
            current->next = cache->head;
            if (cache->head != NULL) {
                cache->head->prev = current;
            }
            current->prev = NULL;
            cache->head = current;
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void addEntry(LRUCache* cache, const char* filepath) {
    Node* existingNode = searchLRUEntry(cache, filepath);
    if (existingNode != NULL) {
        printf("File '%s' already exists in the cache.\n", filepath);
        return;
    }

    printf("Adding file: %s\n", filepath);

    Node* newNode = createNode(filepath);

    if (cache->size == cache->capacity) {
        Node* lastNode = cache->tail;
        cache->tail = lastNode->prev;
        if (cache->tail != NULL) {
            cache->tail->next = NULL;
        }
        free(lastNode);
        cache->size--;
    }

    newNode->next = cache->head;
    if (cache->head != NULL) {
        cache->head->prev = newNode;
    }
    cache->head = newNode;

    if (cache->tail == NULL) {
        cache->tail = newNode;
    }

    cache->size++;
}

void removeLRUEntry(LRUCache* cache, const char* filepath) {
    Node* node = searchLRUEntry(cache, filepath);
    if (node != NULL) {
        cache->tail = (node->prev != NULL) ? node->prev : NULL;

        if (node->prev != NULL) {
            node->prev->next = node->next;
        }
        else {
            cache->head = node->next;
        }

        if (node->next != NULL) {
            node->next->prev = node->prev;
        }

        free(node);
        cache->size--;
        printf("File '%s' removed from the LRU cache.\n", filepath);
    }
    else {
        printf("File '%s' not found in the LRU cache.\n", filepath);
    }
}

void printLRUCache(LRUCache* cache) {
    if (cache->head == NULL) {
        printf("LRU Cache is empty. Add Entries to Display\n");
        return;
    }

    Node* current = cache->head;
    printf("LRU Cache Contents:\n");
    while (current != NULL) {
        printf("File: %s, Size: %zu, Timestamp: %ld,  Inode: %lu\n",
            current->data.filepath, current->data.size, current->data.timestamp,
            current->data.inode);
        current = current->next;
    }
}

void destroyLRUCache(LRUCache* cache) {
    Node* current = cache->head;
    Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(cache);
}

int displayMenu() {
    int choice;
    printf("\nLRU Cache Menu:\n");
    printf("1. Add File to Cache\n");
    printf("2. Search File in Cache\n");
    printf("3. Remove File from Cache\n");
    printf("4. Display Cache Contents\n");
    printf("5. Exit\n");
    printf("Enter your choice (1-5): ");
    scanf("%d", &choice);
    return choice;
}

int main() {
    LRUCache* cache = createLRUCache(3);

    int choice;
    do {
        choice = displayMenu();

        switch (choice) {
        case 1: {
            char filepath[MAX_PATH_SIZE];
            printf("Enter file path: ");
            scanf("%s", filepath);
            addEntry(cache, filepath);
            break;
        }
        case 2: {
            char filepath[MAX_PATH_SIZE];
            printf("Enter file path to search: ");
            scanf("%s", filepath);
            Node* searchedNode = searchLRUEntry(cache, filepath);
            if (searchedNode != NULL) {
                printf("File '%s' found in the LRU cache.\n", filepath);
            }
            else {
                printf("File '%s' not found in the LRU cache.\n", filepath);
            }
            break;
        }
        case 3: {
            char filepath[MAX_PATH_SIZE];
            printf("Enter file path to remove: ");
            scanf("%s", filepath);
            removeLRUEntry(cache, filepath);
            break;
        }
        case 4:
            printLRUCache(cache);
            break;
        case 5:
            printf("Exiting the program.\n");
            break;
        default:
            printf("Invalid choice. Please enter a number between 1 and 5.\n");
        }
    } while (choice != 5);

    destroyLRUCache(cache);

    return 0;
}
