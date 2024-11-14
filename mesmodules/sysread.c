#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/syscall.h>

int main() {
    int fd = open("fichier.txt", O_RDONLY);  // Remplace par le fichier de ton choix
    if (fd == -1) {
        perror("Erreur d'ouverture du fichier");
        return 1;
    }

    char buffer[100];
    ssize_t bytesRead = syscall(SYS_read, fd, buffer, sizeof(buffer) - 1);

    if (bytesRead == -1) {
        perror("Erreur de lecture");
        close(fd);
        return 1;
    }

    buffer[bytesRead] = '\0';  // Terminer la chaîne de caractères
    printf("Contenu lu : %s\n", buffer);

    close(fd);
    return 0;
}

