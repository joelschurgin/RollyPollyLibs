#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = getpid();
    printf("test pid: %lu\n", pid);

    return 0;
}
