#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include "client.c"
#include "utils.h"

#define POOL_SIZE 2

_Atomic double d = 0.0;

double monte_carlo(uint64_t trials) {
    srand((unsigned int)time(NULL));

    uint64_t inside = 0;
    
    for (uint64_t i = 0;i < trials;i++) {
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
    int client_fd = connect_client();
    pthread_t t1;
    pthread_t t2;

    pthread_t tid[POOL_SIZE];
    //task args[POOL_SIZE];

    /*for (int i = 0;i < POOL_SIZE;i++) {
        tid[i] = pthread_create(&tid[i], NULL, mc_thread, &task);
    }*/
    mc_task_t task = { .trials = 1LL<<20, .result = 0.0 };
    uint64_t start = now_ns();
    pthread_create(&t1, NULL, mc_thread, &task);
    pthread_create(&t2, NULL, mc_thread, &task);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    uint64_t end = now_ns();
    uint64_t elapsed = end - start;
    printf("Completed in: %" PRIu64 " ns\n", elapsed);
    printf("Which is:     %.3f ms\n", elapsed / 1e6);
    /*for (int i = 0;i < POOL_SIZE;i++) {
        pthread_join(tid[i], NULL);
    }*/

    //printf("pi ≈ %.6f\n", task.result);

    double val = atomic_load(&d);
    printf("FINAL: pi ≈ %.6f\n", val / 2);
    double out = val / 2;

    mc_task_t output;
    output.result = out;
    output.trials = (1LL<<20) * POOL_SIZE;

    uint8_t buf[sizeof output];
    serialize_message(&output, buf);

    size_t left = MSG_WIRE_SIZE;
    uint8_t *p = buf;

    while (left > 0) {
        ssize_t n = send(client_fd, p, left, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("send");
            break;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }



    return 0;
}