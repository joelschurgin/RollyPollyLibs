#include <stdio.h>

unsigned long calculate_factorial(unsigned long n) {
    unsigned long result = 1;
 
    for (unsigned long i = n; i >= 1; i--) {
        result = result * i; 
    }

    return result;
}

int main() {
    unsigned long number = 5;
    unsigned long fact = calculate_factorial(number);
 
    printf("The factorial of %d is: %d\n", number, fact);
    return 0;
}
