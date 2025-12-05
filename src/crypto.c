#include <linux/kernel.h>
#include <linux/string.h>
#include "cryptochannel.h"

/* Exemplo temporário: cifra XOR simples (placeholder) */
int encrypt_msg(const char *plaintext, size_t len, char *ciphertext)
{
    size_t i;
    for (i = 0; i < len; i++)
        ciphertext[i] = plaintext[i] ^ 0xAA;

    return 0;
}

int decrypt_msg(const char *ciphertext, size_t len, char *plaintext)
{
    size_t i;
    for (i = 0; i < len; i++)
        plaintext[i] = ciphertext[i] ^ 0xAA;

    return 0;
}