#include <stdio.h>
#include <cuda_runtime.h>

int main() {
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("sm_%d%d\n", prop.major, prop.minor);
    return 0;
}
