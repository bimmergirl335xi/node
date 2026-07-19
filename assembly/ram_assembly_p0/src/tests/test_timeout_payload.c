#include <time.h>

int main(void)
{
    struct timespec delay = {.tv_sec = 30, .tv_nsec = 0};

    while (nanosleep(&delay, &delay) != 0) {
    }
    return 0;
}
