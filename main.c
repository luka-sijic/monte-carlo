#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
sizeof -> size_t
%zu - size_t
%zd - ssize_t
%lu - unsigned long
%ld - long
*/


// __attribute__((packed)) - removes padding from struct?
typedef struct __attribute__((packed)) {
    uint8_t a;
    int32_t b;
} node;

typedef struct {
    uint8_t a;
    int32_t b;
} node2;


int main() {
    printf("Size of struct: %zu\n", sizeof(node));
    printf("Size of struct: %zu\n", sizeof(node2));
    return 0;
}