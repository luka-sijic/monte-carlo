#include <stdio.h>
#include <stdlib.h>

#define TABLE_SIZE 20

typedef struct node {
    int data;
    struct node* next;
} node;

typedef struct {
    node** buckets;
    size_t capacity;
    size_t size;
} ht;

ht* createHashTable(int capacity) {
    ht *table = malloc(sizeof(ht));
    table->buckets = calloc((size_t)capacity, sizeof(*table->buckets));
    table->capacity = capacity;
    table->size = 0;
    return table;
}

int hash(int data) {
    return data % TABLE_SIZE;
}

void insert(ht* table, int data) {
    node* newNode = malloc(sizeof(node));
    newNode->data = data;
    if (table->buckets[data % TABLE_SIZE] != NULL) {
        newNode->next = table->buckets[data % TABLE_SIZE];
        table->buckets[data % TABLE_SIZE] = newNode;
    } else {
        table->buckets[data % TABLE_SIZE] = newNode;
    }
}

void list(ht* table) {
    for (int i = 0;i < TABLE_SIZE;i++) {
        if (table->buckets[i] != NULL) {
            node* curr = table->buckets[i];
            while (curr) {
                printf("%d : %d ", i, table->buckets[i]->data);
                curr = curr->next;
            }
            printf("\n");
        }
    }
}

/*
int main() {
    ht *table = createHashTable(TABLE_SIZE);
    insert(table, 10);
    insert(table, 20);
    list(table);
    printf("%zu\n", sizeof("Hello"));
    return 0;
}*/