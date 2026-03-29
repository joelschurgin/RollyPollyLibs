#include <stdio.h>
#include <unistd.h>

#define LOG(fmt, ...) printf(fmt, __VA_ARGS__)

int main() {
    pid_t pid = getpid();
    LOG("pid: %lu\n", pid);

    return 0;
}
