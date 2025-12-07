#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include "cryptochannel.h"

// Garante que o linker saiba onde estão as variaveis globais
extern char crypto_key[16];
extern int crypto_mode; // Nao usaremos o modo por enquanto, mas ele existe.

/* * Função Auxiliar: Executa XOR usando a chave global (crypto_key)
 * Implementa uma variação simples do Vigenère/XOR com chave
 */
static void perform_xor_crypt(const char *input, size_t len, char *output, int mode)
{
    size_t i;
    int key_len = strlen(crypto_key);

    if (key_len == 0) {
        key_len = 1; // Usar chave minima se nao houver
    }

    // Aplica XOR com cada byte da chave de forma cíclica
    for (i = 0; i < len; i++) {
        // Usa o operador ternário para evitar divisao/modulo na chave se o i for menor
        char key_byte = crypto_key[i % key_len];

        // A operacao XOR é simetrica, entao serve para cifrar (mode=1) e decifrar (mode=0)
        output[i] = input[i] ^ key_byte;
    }
}


/* Cifra a mensagem usando a chave configurada */
int encrypt_msg(const char *plaintext, size_t len, char *ciphertext)
{
    if (len == 0) return 0;

    // A cifra XOR simples nao altera o tamanho, o que simplifica o I/O
    perform_xor_crypt(plaintext, len, ciphertext, 1);

    // Retorna o tamanho da mensagem cifrada (o mesmo do plaintext)
    return len;
}

/* Decifra a mensagem usando a chave configurada */
int decrypt_msg(const char *ciphertext, size_t len, char *plaintext)
{
    if (len == 0) return 0;

    perform_xor_crypt(ciphertext, len, plaintext, 0);

    // Retorna o tamanho da mensagem decifrada (o mesmo do ciphertext)
    return len;
}
