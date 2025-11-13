#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdlib.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/socket.h>
#include <stdint.h>

typedef struct {
    long long trials;
    double result;
} mc_task_t;

typedef struct net {
    long long trials;
    double result;
} net;

static inline double rand_unit() {
    return (double)rand() / (double)RAND_MAX;
}

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#endif