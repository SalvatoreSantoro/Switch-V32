#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

struct mock {
    int a;
    int b;
    short c;
    short x;
};

int main(int argc, char *argv[]) {
    struct mock *sm = malloc(sizeof(struct mock));
    sm->a=10;
    sm->b=11;
    sm->c=12;
    sm->x=13;
    printf("%d\n", sm->a);
    printf("%d\n", sm->b);
    printf("%d\n", sm->c);
    printf("%d\n", sm->x);
}
