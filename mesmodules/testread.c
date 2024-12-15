#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd = open("fichier.txt", O_RDONLY);
    if (fd < 0) return 1;

    char buf[128];
    read(fd, buf, 4);
    write(1, buf, 4);

    close(fd);
    return 0;
}

