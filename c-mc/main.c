#include "stdio.h"
#include <stdlib.h>
#include <sys/_pthread/_pthread_t.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

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
    old = (pi_estimate + old) / 2;
    atomic_store(&d, old);
    return pi_estimate;
}

static void *mc_thread(void *arg) {
    mc_task_t *t = (mc_task_t*)arg;
    t->result = monte_carlo(t->trials);
    return NULL;
}

int main() {
    pthread_t t1;
    pthread_t t2;
    mc_task_t task = { .trials = 1LL<<24, .result = 0.0 };
    pthread_create(&t1, NULL, mc_thread, &task);
    pthread_create(&t2, NULL, mc_thread, &task);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("pi ≈ %.6f\n", task.result);

    double val = atomic_load(&d);
    printf("FINAL: pi ≈ %.6f\n", val);

    return 0;
}