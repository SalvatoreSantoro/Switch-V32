#include <stdio.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char *argv[]) {
    struct timespec ts;
    struct timeval tv;
    printf("%ld\n", sizeof(ts));
    printf("%ld\n", sizeof(tv));
}
