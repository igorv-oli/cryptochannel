#include <linux/kernel.h>
#include <linux/string.h>
#include "cryptochannel.h"

/* Variáveis globais */
char crypto_key[16] = "123456789ABCDEF";
int crypto_mode = 0;

/* Exemplo temporário: cifra XOR simples (placeholder) */
int crypto_encrypt(const char *in, char *out, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        out[i] = in[i] ^ 0xAA;

    return 0;
}

int crypto_decrypt(const char *in, char *out, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        out[i] = in[i] ^ 0xAA;

    return 0;
}