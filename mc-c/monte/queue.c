#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>

typedef struct node {
    int data;
    struct node* next;
} node;

typedef struct ll {
    node* head;
    node* tail;
    size_t size;
    pthread_mutex_t mu;
} ll;

typedef struct {
    ll* list;
    unsigned seed;
} worker_arg;

node* createNode(int val) {
    node* q = malloc(sizeof(node));
    q->data = val;
    q->next = NULL;
    return q;
}

ll* createLinkedList() {
    ll* list = malloc(sizeof(ll));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    pthread_mutex_init(&list->mu, NULL);
    return list;
}

void enqueue(ll* l, int val) {
    node* q = createNode(val);
    pthread_mutex_lock(&l->mu);
    if (l->head == NULL) {
        l->head = q;
        l->tail = q;
    } else {
        l->tail->next = q;
        l->tail = q;
    }
    l->size++;
    pthread_mutex_unlock(&l->mu);
}

int dequeue(ll* l) {
    pthread_mutex_lock(&l->mu);
    if (!l->head) {
        pthread_mutex_unlock(&l->mu);
        return -1;
    }
    int val = l->head->data;
    node* temp = l->head;
    l->head = l->head->next;
    if (!l->head) l->tail = NULL;
    l->size--;
    pthread_mutex_unlock(&l->mu);
    free(temp);
    return val;
}

int size(ll* l) {
    //printf("Size: %zu\n", l->size);
    pthread_mutex_lock(&l->mu);
    int val = l->size;
    pthread_mutex_unlock(&l->mu);
    return val;
}

void list(ll* l) {
    pthread_mutex_lock(&l->mu);
    node* curr = l->head;
    while (curr) {
        printf("%d ", curr->data);
        curr = curr->next;
    }
    printf("\n");
    pthread_mutex_unlock(&l->mu);
}

static void* worker(void* arg) {
    worker_arg* wa = (worker_arg*)arg;
    ll* l = wa->list;
    unsigned s = wa->seed;

    for (int i = 0;i < 100000;i++) {
        if (rand_r(&s) & 1) {
            enqueue(l, (int)(i ^ (int)(uintptr_t)pthread_self()));
        } else {
            (void)dequeue(l);
        }
        if ((i & 0xFF ) == 0) usleep(1);
    }
    return NULL;
}

int main() {
    ll* l = createLinkedList();
    
    const int THREADS = 8;
    pthread_t tid[THREADS];
    worker_arg args[THREADS];

    for (int i = 0;i < THREADS;i++) {
        args[i].list = l;
        args[i].seed = (unsigned)time(NULL) ^ (i * 2654435761u);
        if (pthread_create(&tid[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create"); exit(1);
        }
    }
    for (int i = 0;i < THREADS;i++) pthread_join(tid[i], NULL);

    printf("Final Size: %zu\n", l->size);
    return 0;
}