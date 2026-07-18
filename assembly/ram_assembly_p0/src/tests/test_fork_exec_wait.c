#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    pid_t child;
    int status = 0;

    if (argc == 2 && strcmp(argv[1], "--child") == 0) {
        return 0;
    }
    child = fork();
    if (child < 0) {
        return 2;
    }
    if (child == 0) {
        execl(argv[0], argv[0], "--child", (char *)NULL);
        _exit(125);
    }
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            return 2;
        }
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 2;
}
