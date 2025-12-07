#ifndef __CRYPTOCHANNEL_H__
#define __CRYPTOCHANNEL_H__

// Inclusões de Kernel comuns para o projeto
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>

// ==========================================================
// 1. Definições e Macros
// ==========================================================
#define DEVICE_NAME "cryptochannel"
#define MINOR_NUMBER_START 0
#define NUM_DEVICES 1
#define BUFFER_SIZE 4096 // Exemplo: 4KB de buffer


// ==========================================================
// 2. Variáveis de Estado/Estatísticas (Componente 2)
// ==========================================================

// Variáveis para a chave e modo
extern char crypto_key[16];
extern int crypto_mode;

// Variáveis de estatísticas
extern size_t total_messages_sent;
extern size_t total_bytes_encrypted;
extern size_t total_errors;

int cryptochannel_procfs_init(void);
void cryptochannel_procfs_exit(void);

// ==========================================================
// 3. Funções de Criptografia (Componente 2)
// ==========================================================

// Funções que você CHAMARÁ em read/write (Componente 1)
int encrypt_msg(const char *plaintext, size_t len, char *ciphertext);
int decrypt_msg(const char *ciphertext, size_t len, char *plaintext);


// ==========================================================
// 4. Estrutura do Dispositivo (Componente 1)
// ==========================================================

struct cryptochannel_dev {
    struct cdev cdev;
    dev_t dev_num;

    // Sincronização e Buffer
    struct kfifo buf;                 // Buffer para mensagens cifradas (Será alocado estaticamente)
    struct mutex lock;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
};

// DECLARE_KFIFO(crypto_buffer_data, char, BUFFER_SIZE);

// Declara a instancia principal do dispositivo como EXTERN para que outros .c possam acessa-la
extern struct cryptochannel_dev crypto_dev;

#endif // __CRYPTOCHANNEL_H_
