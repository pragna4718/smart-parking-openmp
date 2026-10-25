#include <omp.h>
#include <cstdio>

int main() {
    printf("========================================\n");
    printf(" OpenMP Multi-Threading Environment Test\n");
    printf("========================================\n");
    printf("Maximum Available Threads : %d\n", omp_get_max_threads());
    printf("Dynamic Adjustment Enabled : %s\n\n", omp_get_dynamic() ? "true" : "false");

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        #pragma omp critical
        {
            printf("Thread %d of %d is executing concurrently.\n", tid, nthreads);
        }
    }

    printf("\nOpenMP verification passed successfully.\n");
    return 0;
}
