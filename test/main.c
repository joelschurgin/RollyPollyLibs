#include <stdio.h>
#include <unistd.h>

float func(float a) {
    return a * 0.3f;
}

int main() {
    pid_t pid = getpid();
    printf("test pid: %lu\n", pid);

    for (int i = 0; i < 100000; i++) {
        float a = 0.1f;
        float b = func(a + pid);
    }

    return 0;
}
