#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdlib.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>

#define MSG_WIRE_SIZE (sizeof(uint64_t) + sizeof(double) + 32)

typedef struct mc_task_t {
    uint64_t trials;
    double result;
    unsigned int seed;
} mc_task_t;


static inline void serialize_message(const mc_task_t *n, uint8_t *buf) {
    //uint32_t net_id = htonl(n->trials);

    size_t off = 0;

    memcpy(buf + off, &n->trials, sizeof n->trials);
    off += sizeof n->trials;

    memcpy(buf + off, &n->result, sizeof n->result);
    off += sizeof n->result;
}

static inline double rand_unit(unsigned int *state) {
    unsigned int r = rand_r(state);
    return (double)r / ((double)RAND_MAX + 1.0);
}

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#endif