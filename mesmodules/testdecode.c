#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>

static void custom_xor(u8 *dst, const u8 *src, u8 key, unsigned int len) {
    unsigned int i;
    for (i = 0; i < len; i++) {
        dst[i] = src[i] ^ key;
    }
}

static int __init xor_ip_decode_init(void) {
    // Adresse IP originale
    u8 original_ip[] = "192.168.1.37";  // Exemple
    u8 key = 126;                      // Clé utilisée pour le XOR
    size_t ip_len = strlen(original_ip);
    u8 *encoded_ip;
    u8 *decoded_ip;

    // Allouer de la mémoire pour l'adresse XORée et décodée
    encoded_ip = kmalloc(ip_len, GFP_KERNEL);
    decoded_ip = kmalloc(ip_len + 1, GFP_KERNEL); // +1 pour le terminateur null
    if (!encoded_ip || !decoded_ip) {
        pr_err("Failed to allocate memory\n");
        kfree(encoded_ip);
        kfree(decoded_ip);
        return -ENOMEM;
    }

    // Encoder avec XOR
    custom_xor(encoded_ip, original_ip, key, ip_len);
    pr_info("Encoded IP: %.*s\n", (int)ip_len, encoded_ip);

    // Décoder avec XOR
    custom_xor(decoded_ip, encoded_ip, key, ip_len);
    decoded_ip[ip_len] = '\0'; // Terminateur null pour affichage

    pr_info("Decoded IP: %s\n", decoded_ip);

    // Libérer la mémoire
    kfree(encoded_ip);
    kfree(decoded_ip);

    return 0;
}

static void __exit xor_ip_decode_exit(void) {
    pr_info("XOR IP decode module unloaded\n");
}

module_init(xor_ip_decode_init);
module_exit(xor_ip_decode_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("gogoliomax");
MODULE_DESCRIPTION("Custom XOR function to decode an IP address");

