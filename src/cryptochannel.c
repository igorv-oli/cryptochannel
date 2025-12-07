#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kfifo.h>

// Inclui o cabeçalho comum que define as estruturas e interfaces
#include "cryptochannel.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniel Suzuki Naves e Igor Oliveira");
MODULE_DESCRIPTION("Modulo cryptochannel para SO.");

// Instância global do nosso dispositivo
struct cryptochannel_dev crypto_dev;

// Variáveis de estado e criptografia (apenas stubs para compilar)
// Estas variáveis precisam ser definidas em *algum lugar* para que o linker não reclame.
char crypto_key[16] = {0};
int crypto_mode = 0;
size_t total_messages_sent = 0;
size_t total_bytes_encrypted = 0;
size_t total_errors = 0;


// Protótipos das funções
static int cryptochannel_open(struct inode *inode, struct file *file);
static int cryptochannel_release(struct inode *inode, struct file *file);
ssize_t cryptochannel_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos);
ssize_t cryptochannel_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos);
int cryptochannel_procfs_init(void);
void cryptochannel_procfs_exit(void);

// Tabela de operações do arquivo
static const struct file_operations cryptochannel_fops = {
    .owner = THIS_MODULE,
    .open = cryptochannel_open,
    .release = cryptochannel_release,
    .read = cryptochannel_read,
    .write = cryptochannel_write,
};

// Funções de operação (stubs)
static int cryptochannel_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "cryptochannel: Dispositivo aberto.\n");
    return 0;
}

static int cryptochannel_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "cryptochannel: Dispositivo fechado.\n");
    return 0;
}

ssize_t cryptochannel_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    char *kbuf_cipher;  // Buffer kernel para texto CIFRADO lido do kfifo
    char *kbuf_plain;   // Buffer kernel para texto DECIFRADO para o usuário
    int ret;
    int bytes_cifrados;
    int decrypted_len;

    // Bloqueia o acesso ao buffer
    if (mutex_lock_interruptible(&crypto_dev.lock)) {
        return -ERESTARTSYS;
    }

    // 1. Loop de leitura (Consumidor bloqueante)
    while (kfifo_len(&crypto_dev.buf) == 0) {
        mutex_unlock(&crypto_dev.lock);
        if (file->f_flags & O_NONBLOCK) return -EAGAIN; // Non-blocking

        // Espera ser acordado por um escritor (produtor)
        if (wait_event_interruptible(crypto_dev.read_queue, kfifo_len(&crypto_dev.buf) > 0)) {
            return -ERESTARTSYS;
        }

        if (mutex_lock_interruptible(&crypto_dev.lock)) {
            return -ERESTARTSYS;
        }
    }

    // 2. Alocar buffers
    bytes_cifrados = kfifo_len(&crypto_dev.buf);

    // Para simplificar a integração com a criptografia, vamos ler TUDO que está no buffer para
    // garantir que uma mensagem completa seja decifrada (pois kfifo_out não sabe o tamanho da mensagem)

    kbuf_cipher = kmalloc(bytes_cifrados, GFP_KERNEL);
    kbuf_plain = kmalloc(bytes_cifrados, GFP_KERNEL); // Assume que o decifrado tem tamanho <= cifrado

    if (!kbuf_cipher || !kbuf_plain) {
        kfree(kbuf_cipher);
        kfree(kbuf_plain);
        mutex_unlock(&crypto_dev.lock);
        return -ENOMEM;
    }

    // 3. Ler os dados CIFRADOS do kfifo
    ret = kfifo_out(&crypto_dev.buf, kbuf_cipher, bytes_cifrados);

    // 4. CHAMADA DE DECIFRAGEM (Componente 2)
    decrypted_len = decrypt_msg(kbuf_cipher, ret, kbuf_plain);

    if (decrypted_len < 0) {
        printk(KERN_ALERT "cryptochannel: Falha na decifragem.\n");
        // TODO: Incrementar total_errors
        kfree(kbuf_cipher);
        kfree(kbuf_plain);
        mutex_unlock(&crypto_dev.lock);
        return decrypted_len;
    }

    // 5. Despertar escritores (produtores)
    wake_up_interruptible(&crypto_dev.write_queue);

    // 6. Copiar dados DECIFRADOS para o espaço do usuário
    if (copy_to_user(buf, kbuf_plain, decrypted_len)) {
        kfree(kbuf_cipher);
        kfree(kbuf_plain);
        mutex_unlock(&crypto_dev.lock);
        return -EFAULT;
    }

    // 7. Liberar e sair
    mutex_unlock(&crypto_dev.lock);
    kfree(kbuf_cipher);
    kfree(kbuf_plain);

    return decrypted_len; // Retorna o número de bytes DECIFRADOS enviados
}

ssize_t cryptochannel_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    char *kbuf_in;     // Buffer kernel para texto PURO recebido do usuário
    char *kbuf_crypto; // Buffer kernel para texto CIFRADO
    int ret;
    int encrypted_len;

    if (count == 0 || count > BUFFER_SIZE) return -EINVAL; // Verifica limites de tamanho

    // 1. Alocar buffers temporários no kernel
    kbuf_in = kmalloc(count, GFP_KERNEL);
    kbuf_crypto = kmalloc(BUFFER_SIZE, GFP_KERNEL); // Alocamos o tamanho máximo para o cifrado

    if (!kbuf_in || !kbuf_crypto) {
        kfree(kbuf_in);
        kfree(kbuf_crypto);
        return -ENOMEM;
    }

    // 2. Copiar dados do espaço do usuário (Texto Puro) para o kernel
    if (copy_from_user(kbuf_in, buf, count)) {
        kfree(kbuf_in);
        kfree(kbuf_crypto);
        return -EFAULT;
    }

    // 3. CHAMADA DE CRIPTOGRAFIA (Componente 2)
    // O igor implementará esta função para usar a chave e o modo configurados.
    encrypted_len = encrypt_msg(kbuf_in, count, kbuf_crypto);

    if (encrypted_len < 0) {
        printk(KERN_ALERT "cryptochannel: Falha na criptografia.\n");
        // TODO: Incrementar total_errors
        kfree(kbuf_in);
        kfree(kbuf_crypto);
        return encrypted_len;
    }

    // Bloqueia o acesso ao buffer e às estatísticas
    if (mutex_lock_interruptible(&crypto_dev.lock)) {
        kfree(kbuf_in);
        kfree(kbuf_crypto);
        return -ERESTARTSYS;
    }

    // Loop de escrita no kfifo (Produtor bloqueante)
    while (kfifo_avail(&crypto_dev.buf) < encrypted_len) {
        mutex_unlock(&crypto_dev.lock);

        // Espera ser acordado por um leitor (consumidor)
        if (wait_event_interruptible(crypto_dev.write_queue, kfifo_avail(&crypto_dev.buf) >= encrypted_len)) {
            kfree(kbuf_in);
            kfree(kbuf_crypto);
            return -ERESTARTSYS;
        }

        if (mutex_lock_interruptible(&crypto_dev.lock)) {
            kfree(kbuf_in);
            kfree(kbuf_crypto);
            return -ERESTARTSYS;
        }
    }

    // 4. Escrever a mensagem CIFRADA no kfifo
    ret = kfifo_in(&crypto_dev.buf, kbuf_crypto, encrypted_len);

    // 5. Despertar leitores e atualizar estatísticas
    wake_up_interruptible(&crypto_dev.read_queue);

    // Atualizar estatísticas (Componente 2 usa estas variáveis)
    total_bytes_encrypted += ret;
    total_messages_sent++;

    // 6. Liberar e sair
    mutex_unlock(&crypto_dev.lock);
    kfree(kbuf_in);
    kfree(kbuf_crypto);

    return count; // Retorna o número de bytes DE TEXTO PURO que foram aceitos
}

// Funcao auxiliar para limpar o dispositivo de caractere
static void crypto_cleanup_device(void)
{
    cdev_del(&crypto_dev.cdev);
    unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES);
}

// Funcao auxiliar para limpar TUDO (chamada apenas no exit)
static void crypto_full_cleanup(void)
{
    // 1. Remover a interface /proc (Se foi inicializada)
    cryptochannel_procfs_exit();

    // 2. Limpar o dispositivo (cdev, Major/Minor)
    crypto_cleanup_device();

    // 3. Liberar o kfifo (necessario se usou kfifo_alloc)
    kfifo_free(&crypto_dev.buf);
}

// Função chamada na inserção do módulo (insmod)
static int __init cryptochannel_init(void)
{
    int ret;

    printk(KERN_INFO "cryptochannel: Inicializando o modulo...\n");

    // 1. Alocação dinâmica de Major/Minor numbers
    ret = alloc_chrdev_region(&crypto_dev.dev_num, MINOR_NUMBER_START, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "cryptochannel: Falha ao alocar major/minor numbers.\n");
        return ret;
    }

    // 2. Inicializar Mutex e Filas de Espera
    mutex_init(&crypto_dev.lock);
    init_waitqueue_head(&crypto_dev.read_queue);
    init_waitqueue_head(&crypto_dev.write_queue);

    // 3. Inicializar o KFIFO (Alocação dinâmica)
    ret = kfifo_alloc(&crypto_dev.buf, BUFFER_SIZE, GFP_KERNEL);
    if (ret) {
        printk(KERN_ALERT "cryptochannel: Falha ao alocar kfifo.\n");
        unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES); // <--- LIMPEZA 1
        return ret;
    }

    // 4. Inicializar e adicionar a estrutura cdev
    cdev_init(&crypto_dev.cdev, &cryptochannel_fops);
    crypto_dev.cdev.owner = THIS_MODULE;

    ret = cdev_add(&crypto_dev.cdev, crypto_dev.dev_num, NUM_DEVICES);
    if (ret < 0) {
        kfifo_free(&crypto_dev.buf); // <--- LIMPEZA 2
        unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES); 
        printk(KERN_ALERT "cryptochannel: Falha ao adicionar o dispositivo.\n");
        return ret;
    }

    // 5. Inicializar a interface /proc
    ret = cryptochannel_procfs_init();
    if (ret) {
        // Se o procfs falhar, DESFAZER TUDO O QUE FOI FEITO.
        crypto_cleanup_device(); // Limpa cdev e Major/Minor
        kfifo_free(&crypto_dev.buf); // Limpa o kfifo
        printk(KERN_ALERT "cryptochannel: Falha ao inicializar /proc.\n");
        return ret;
    }

    printk(KERN_INFO "cryptochannel: Modulo carregado com sucesso. Major: %d.\n", MAJOR(crypto_dev.dev_num));
    return 0;
}

// Função chamada na remoção do módulo (rmmod)
static void __exit cryptochannel_exit(void)
{
    printk(KERN_INFO "cryptochannel: Removendo o modulo...\n");

    crypto_full_cleanup();

    printk(KERN_INFO "cryptochannel: Modulo descarregado.\n");
}

module_init(cryptochannel_init);
module_exit(cryptochannel_exit);
EXPORT_SYMBOL(crypto_dev);
