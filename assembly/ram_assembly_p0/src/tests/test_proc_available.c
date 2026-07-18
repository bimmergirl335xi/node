#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    char byte;
    const int descriptor = open("/proc/self/stat", O_RDONLY | O_CLOEXEC);

    if (descriptor < 0) {
        return 2;
    }
    if (read(descriptor, &byte, 1U) != 1) {
        (void)close(descriptor);
        return 2;
    }
    return close(descriptor) == 0 ? 0 : 2;
}
