#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    const int descriptor = open("/sys/kernel", O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (descriptor < 0) {
        return 2;
    }
    return close(descriptor) == 0 ? 0 : 2;
}
