#ifndef UTILS_H 
#define UTILS_H 

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MSG_WIRE_SIZE (sizeof(uint64_t) + sizeof(double) + 32)

typedef struct mc_task_t {
    uint64_t trials;
    double result;
} mc_task_t;

static inline void deserialize_msg(mc_task_t *m, uint8_t *buf) {
    size_t off = 0;

    memcpy(&m->trials, buf + off, sizeof m->trials);
    off += sizeof m->trials;

    memcpy(&m->result, buf + off, sizeof m->result);
    off += sizeof m->result;
}

#endif