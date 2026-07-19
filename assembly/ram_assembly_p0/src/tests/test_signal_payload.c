#include <signal.h>

int main(void)
{
    if (raise(SIGTERM) != 0) {
        return 2;
    }
    return 2;
}
