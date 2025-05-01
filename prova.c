#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd = open(argv[2], O_RDONLY);
    const char str[10];
    read(fd, (void *) str, 10);
    printf("%s", str);
}
