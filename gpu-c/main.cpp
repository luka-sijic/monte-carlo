#include <hip/hip_runtime.h>
#include <cstdio>
#include <cstdint>
#include <cmath>

// Very simple per-thread RNG (xorshift32 style)
__device__ uint32_t rng_next(uint32_t &state) {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

__device__ float rng_uniform(uint32_t &state) {
    // convert to [0,1)
    return (float)rng_next(state) / 4294967296.0f; // 2^32
}

// HIP kernel: each thread does 'trials_per_thread' samples
__global__ void mc_pi_kernel(uint64_t trials_per_thread,
                             uint64_t *counts,
                             uint64_t seed) {
    int gtid = blockIdx.x * blockDim.x + threadIdx.x;

    uint64_t inside = 0;
    uint32_t state = (uint32_t)(seed ^ (0x9e3779b9u * (uint32_t)gtid));

    for (uint64_t i = 0; i < trials_per_thread; i++) {
        float x = rng_uniform(state);
        float y = rng_uniform(state);
        float d2 = x * x + y * y;
        if (d2 <= 1.0f) inside++;
    }

    counts[gtid] = inside;
}

int main() {
    // --- config ---
    const int    blocks  = 256;
    const int    threads = 256;
    const int    total_threads = blocks * threads;
    const uint64_t trials_per_thread = 1ULL << 16; // 65536
    const uint64_t total_trials = (uint64_t)total_threads * trials_per_thread;
    const uint64_t seed = 123456789ULL;

    printf("Total threads: %d\n", total_threads);
    printf("Total trials : %llu\n",
           (unsigned long long)total_trials);

    // --- allocate device memory ---
    uint64_t *d_counts = nullptr;
    hipError_t err = hipMalloc(&d_counts, total_threads * sizeof(uint64_t));
    if (err != hipSuccess) {
        fprintf(stderr, "hipMalloc failed: %s\n", hipGetErrorString(err));
        return 1;
    }

    // --- launch kernel ---
    dim3 grid(blocks);
    dim3 block(threads);

    hipLaunchKernelGGL(mc_pi_kernel,
                       grid, block,
                       0, 0,                // sharedMem, stream
                       trials_per_thread,
                       d_counts,
                       seed);

    err = hipGetLastError();
    if (err != hipSuccess) {
        fprintf(stderr, "Kernel launch failed: %s\n", hipGetErrorString(err));
        hipFree(d_counts);
        return 1;
    }

    hipDeviceSynchronize();

    // --- copy back results ---
    uint64_t *h_counts = new uint64_t[total_threads];
    err = hipMemcpy(h_counts, d_counts,
                    total_threads * sizeof(uint64_t),
                    hipMemcpyDeviceToHost);
    if (err != hipSuccess) {
        fprintf(stderr, "hipMemcpy failed: %s\n", hipGetErrorString(err));
        hipFree(d_counts);
        delete[] h_counts;
        return 1;
    }

    // --- sum counts ---
    uint64_t total_inside = 0;
    for (int i = 0; i < total_threads; i++) {
        total_inside += h_counts[i];
    }

    double pi_est = 4.0 * (double)total_inside / (double)total_trials;
    printf("inside      : %llu\n", (unsigned long long)total_inside);
    printf("pi estimate : %.10f\n", pi_est);

    // --- cleanup ---
    hipFree(d_counts);
    delete[] h_counts;

    return 0;
}
