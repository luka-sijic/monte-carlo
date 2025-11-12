#include "stdio.h"
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#define POOL_SIZE 10

_Atomic double d = 0.0;

typedef struct {
    long long trials;
    double result;
} mc_task_t;

double rand_unit() {
    return (double)rand() / (double)RAND_MAX;
}

double monte_carlo(long long trials) {
    srand((unsigned int)time(NULL));

    long long inside = 0;
    
    for (long long i = 0;i < trials;i++) {
        double x = rand_unit();
        double y = rand_unit();

        double d2 = x * x + y * y;
        if (d2 <= 1.0) {
            inside++;
        }
    }
    double pi_estimate = 4.0 * (double)inside / (double)trials;

    printf("trials: %lld\n", trials);
    printf("inside: %lld\n", inside);
    printf("pi ~= %.10f\n", pi_estimate);
    
    double old = atomic_load(&d);
    old += pi_estimate;
    atomic_store(&d, old);
    return pi_estimate;
}

// void pointer as argument (generic), we cast it back to a struct b4 using it
static void *mc_thread(void *arg) {
    mc_task_t *t = (mc_task_t*)arg;
    t->result = monte_carlo(t->trials);
    return NULL;
}

int main() {
    pthread_t t1;
    pthread_t t2;

    pthread_t tid[POOL_SIZE];
    //task args[POOL_SIZE];

    /*for (int i = 0;i < POOL_SIZE;i++) {
        tid[i] = pthread_create(&tid[i], NULL, mc_thread, &task);
    }*/
    mc_task_t task = { .trials = 1LL<<50, .result = 0.0 };
    pthread_create(&t1, NULL, mc_thread, &task);
    pthread_create(&t2, NULL, mc_thread, &task);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    /*for (int i = 0;i < POOL_SIZE;i++) {
        pthread_join(tid[i], NULL);
    }*/

    printf("pi ≈ %.6f\n", task.result);

    double val = atomic_load(&d);
    printf("FINAL: pi ≈ %.6f\n", val / 2);

    return 0;
}