#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "cryptochannel.h"

#define PROCFS_DIR "cryptochannel"
#define MAX_CONFIG_LEN 16 // Maximo de 16 bytes para chave/modo

static struct proc_dir_entry *cryptochannel_proc_dir;

// ==========================================================
// 1. Funções de Leitura para /proc/cryptochannel/stats (Somente Leitura)
// ==========================================================

// Função que formatará as estatísticas para o usuário
static int stats_show(struct seq_file *m, void *v)
{
    // Acesso seguro (pois as variaveis podem ser atualizadas a qualquer momento)
    // Usaremos o lock do dispositivo principal (crypto_dev.lock) para protecao
    if (mutex_lock_interruptible(&crypto_dev.lock)) {
        return -ERESTARTSYS;
    }

    seq_printf(m, "Mensagens Enviadas: %zu\n", total_messages_sent);
    seq_printf(m, "Bytes Cifrados: %zu\n", total_bytes_encrypted);
    seq_printf(m, "Erros Registrados: %zu\n", total_errors);

    mutex_unlock(&crypto_dev.lock);
    return 0;
}

// Open callback para o arquivo stats
static int stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, stats_show, NULL);
}

// ==========================================================
// 2. Funções de Leitura/Escrita para /proc/cryptochannel/config
// ==========================================================

// Função que lê a configuração atual (chave e modo)
static int config_show(struct seq_file *m, void *v)
{
    if (mutex_lock_interruptible(&crypto_dev.lock)) {
        return -ERESTARTSYS;
    }

    seq_printf(m, "Modo: %d\n", crypto_mode);
    seq_printf(m, "Chave: %s\n", crypto_key);

    mutex_unlock(&crypto_dev.lock);
    return 0;
}

// Função que recebe a nova configuração do usuário (chave e modo)
static ssize_t config_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[MAX_CONFIG_LEN];
    size_t len = count;

    if (len >= MAX_CONFIG_LEN || len == 0) return -EINVAL;

    // 1. Copia os dados do usuário (novo valor da chave)
    if (copy_from_user(kbuf, buf, len)) return -EFAULT;
    kbuf[len] = '\0'; // Garantir que a string é terminada em nulo

    // 2. Bloqueia e atualiza a chave
    if (mutex_lock_interruptible(&crypto_dev.lock)) return -ERESTARTSYS;

    // A lógica aqui é simples: assume que o usuário está fornecendo a nova chave.
    strncpy(crypto_key, kbuf, MAX_CONFIG_LEN - 1);
    crypto_key[MAX_CONFIG_LEN - 1] = '\0';

    printk(KERN_INFO "cryptochannel: Nova chave configurada via /proc.\n");

    mutex_unlock(&crypto_dev.lock);

    return count;
}

// Open callback para o arquivo config
static int config_open(struct inode *inode, struct file *file)
{
    return single_open(file, config_show, NULL);
}

// ==========================================================
// 3. Definições de fops para /proc
// ==========================================================

static const struct proc_ops stats_fops = {
    .proc_open = stats_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static const struct proc_ops config_fops = {
    .proc_open = config_open,
    .proc_read = seq_read,
    .proc_write = config_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// ==========================================================
// 4. Inicialização e Saída do /proc
// ==========================================================

int cryptochannel_procfs_init(void)
{
    int ret = 0;

    // 1. Criar o diretório /proc/cryptochannel
    cryptochannel_proc_dir = proc_mkdir(PROCFS_DIR, NULL);
    if (!cryptochannel_proc_dir) {
        printk(KERN_ALERT "cryptochannel: Falha ao criar diretorio /proc.\n");
        return -ENOMEM;
    }

    // 2. Criar o arquivo stats (somente leitura)
    if (!proc_create("stats", 0444, cryptochannel_proc_dir, &stats_fops)) {
        ret = -ENOMEM;
        goto error_dir;
    }

    // 3. Criar o arquivo config (leitura/escrita)
    if (!proc_create("config", 0666, cryptochannel_proc_dir, &config_fops)) {
        ret = -ENOMEM;
        goto error_stats;
    }

    printk(KERN_INFO "cryptochannel: Interface /proc criada com sucesso.\n");
    return 0;

error_stats:
    remove_proc_entry("stats", cryptochannel_proc_dir);
error_dir:
    remove_proc_entry(PROCFS_DIR, NULL);
    return ret;
}

void cryptochannel_procfs_exit(void)
{
    // Remove os arquivos e o diretorio na ordem inversa de criacao
    remove_proc_entry("config", cryptochannel_proc_dir);
    remove_proc_entry("stats", cryptochannel_proc_dir);
    remove_proc_entry(PROCFS_DIR, NULL);

    printk(KERN_INFO "cryptochannel: Interface /proc removida.\n");
}

EXPORT_SYMBOL(cryptochannel_procfs_init);
EXPORT_SYMBOL(cryptochannel_procfs_exit);
