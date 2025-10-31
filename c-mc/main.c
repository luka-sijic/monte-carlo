#include "stdio.h"
#include <stdlib.h>
#include <time.h>

double rand_unit() {
    return (double)rand() / (double)RAND_MAX;
}

int main() {
    srand((unsigned int)time(NULL));
    
    long long trials = 100000000;
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
    return 0;
}