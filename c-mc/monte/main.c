#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include "client.h"
#include "utils.h"

#define POOL_SIZE 4

_Atomic double d = 0.0;

double monte_carlo(mc_task_t *task) {
    unsigned int state = task->seed;

    uint64_t trials = task->trials;
    uint64_t inside = 0;
    
    for (uint64_t i = 0;i < trials;i++) {
        double x = rand_unit(&state);
        double y = rand_unit(&state);

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
    t->result = monte_carlo(t);
    return NULL;
}

void send_all(int client_fd, mc_task_t *task) {
    uint8_t buf[MSG_WIRE_SIZE];
    serialize_message(task, buf);

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
}

int main() {
    int client_fd = connect_client();

    pthread_t tid[POOL_SIZE];
    mc_task_t args[POOL_SIZE];

    uint64_t start = now_ns();

    // Create threads
    for (int i = 0;i < POOL_SIZE;i++) {
        args[i].trials = 1LL << 20;
        args[i].result = 0.0;
        if (pthread_create(&tid[i], NULL, mc_thread, &args[i]) != 0) {
            perror("pthread_create"); exit(1);
        }
    }
    
    for (int i = 0;i < POOL_SIZE;i++) pthread_join(tid[i], NULL);
    uint64_t end = now_ns();
    uint64_t elapsed = end - start;
    printf("Completed in: %" PRIu64 " ns\n", elapsed);
    printf("Which is:     %.3f ms\n", elapsed / 1e6);

    double val = atomic_load(&d);
    printf("FINAL: pi ≈ %.6f\n", val / POOL_SIZE);
    double out = val / POOL_SIZE;

    mc_task_t output;
    output.result = out;
    output.trials = (1LL<<20) * POOL_SIZE;

    send_all(client_fd, &output);

    return 0;
}