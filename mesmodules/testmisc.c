#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define VERSION_MODIFIED _IOR('v', 1, bool)
#define VERSION_RESET _IO('v', 2)

int main() {
    int fd, ret;
    int modified;

    fd = open("/dev/version", O_RDWR);
    if (fd < 0) {
        perror("Erreur lors de l'ouverture du p..riph..rique");
        return 1;
    }

    ret = ioctl(fd, VERSION_MODIFIED, &modified);
    if (ret < 0) {
        perror("Erreur lors de l'appel ioctl VERSION_MODIFIED");
        close(fd);
		return 1;
	}

	if (modified) {
		printf("Version modifie\n");
	} else {
		printf("Version non modifie\n");
	}

	ret = ioctl(fd, VERSION_RESET);
	if (ret < 0) {
		perror("Erreur lors de l'appel ioctl VERSION_RESET");
		close(fd);
		return 1;
	}
}


