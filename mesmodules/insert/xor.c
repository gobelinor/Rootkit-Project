#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void custom_xor(char *dst, const char *src, char key, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        dst[i] = src[i] ^ key;
    }
    dst[len] = '\0'; // Assurez-vous que la chaîne de destination est terminée par '\0'
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <string>\n", argv[0]);
        return 1;
    }

    char key = 126; // La clé doit être un caractère si on travaille avec des `char`
    const char *src = argv[1];
    unsigned int len = strlen(src);

    // Allouer dynamiquement de la mémoire pour la chaîne XORée
    char *dst = malloc(len + 1);
    if (!dst) {
        perror("malloc");
        return 1;
    }

    custom_xor(dst, src, key, len);

    printf("%s\n", dst);

    // Libérer la mémoire allouée
    free(dst);

    return 0;
}

