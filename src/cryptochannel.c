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
static struct cryptochannel_dev crypto_dev;

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
    printk(KERN_INFO "cryptochannel: Chamada de READ.\n");
    return 0;
}

ssize_t cryptochannel_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_INFO "cryptochannel: Chamada de WRITE. %zu bytes.\n", count);
    return count;
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

    // 3. Inicializar o KFIFO (Usando alocação dinâmica kfifo_alloc)
    ret = kfifo_alloc(&crypto_dev.buf, BUFFER_SIZE, GFP_KERNEL);
    if (ret) {
    	printk(KERN_ALERT "cryptochannel: Falha ao alocar kfifo.\n");
    	unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES);
    	return ret;
    }


    // 4. Inicializar e adicionar a estrutura cdev
    cdev_init(&crypto_dev.cdev, &cryptochannel_fops);
    crypto_dev.cdev.owner = THIS_MODULE;

    ret = cdev_add(&crypto_dev.cdev, crypto_dev.dev_num, NUM_DEVICES);
    if (ret < 0) {
        unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES);
        printk(KERN_ALERT "cryptochannel: Falha ao adicionar o dispositivo.\n");
        return ret;
    }

    printk(KERN_INFO "cryptochannel: Modulo carregado com sucesso. Major: %d.\n", MAJOR(crypto_dev.dev_num));
    return 0;
}

// Função chamada na remoção do módulo (rmmod)
static void __exit cryptochannel_exit(void)
{
    printk(KERN_INFO "cryptochannel: Removendo o modulo...\n");

    // 1. Desalocar o kfifo
    kfifo_free(&crypto_dev.buf);

    // 2. Remover o dispositivo do kernel
    cdev_del(&crypto_dev.cdev);

    // 3. Liberar os Major/Minor numbers
    unregister_chrdev_region(crypto_dev.dev_num, NUM_DEVICES);

    printk(KERN_INFO "cryptochannel: Modulo descarregado.\n");
}

module_init(cryptochannel_init);
module_exit(cryptochannel_exit);
