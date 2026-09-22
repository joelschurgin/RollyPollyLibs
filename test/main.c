#include <stdio.h>
#include <unistd.h>

float func(float a) {
    return a * 0.3f;
}

int main() {
    pid_t pid = getpid();
    printf("test pid: %lu\n", pid);

    //float (*func_ptr)(float) = func;
    //float c = func_ptr(12.0f);

    for (int i = 0; i < 10; i++) {
        float a = 0.1f;
        float b = func(a);
    }

    return 0;
}
