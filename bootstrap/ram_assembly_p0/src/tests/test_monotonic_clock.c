#include <stdint.h>
#include <time.h>

int main(void)
{
    struct timespec first;
    struct timespec second;
    struct timespec delay = {.tv_sec = 0, .tv_nsec = 1000000L};
    uint64_t first_ns;
    uint64_t second_ns;

    if (clock_gettime(CLOCK_MONOTONIC, &first) != 0) {
        return 2;
    }
    (void)nanosleep(&delay, NULL);
    if (clock_gettime(CLOCK_MONOTONIC, &second) != 0) {
        return 2;
    }
    first_ns = (uint64_t)first.tv_sec * UINT64_C(1000000000) +
               (uint64_t)first.tv_nsec;
    second_ns = (uint64_t)second.tv_sec * UINT64_C(1000000000) +
                (uint64_t)second.tv_nsec;
    return second_ns >= first_ns ? 0 : 2;
}
