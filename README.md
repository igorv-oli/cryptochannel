# Projeto Final de Sistemas Operacionais: Módulo `cryptochannel`

## Visão Geral do Projeto

O módulo de núcleo Linux `cryptochannel` foi desenvolvido para implementar um canal de comunicação seguro e sincronizado entre processos de usuário (modelo Produtor/Consumidor). O módulo atua como um dispositivo de caractere que cifra as mensagens no momento da escrita e as decifra na leitura, utilizando um algoritmo configurável.

### Requisitos Funcionais Atendidos

1.  **Dispositivo de Caractere:** Cria o nó `/dev/cryptochannel` para operações de E/S.
2.  **Sincronização:** Implementa o modelo Produtor/Consumidor com bufferização (KFIFO) e controle de fluxo (Wait Queues).
3.  **Criptografia:** Cifra e decifra as mensagens usando um algoritmo XOR com chave, configurável em tempo de execução.
4.  **Interface `/proc`:** Expõe estatísticas de uso e parâmetros de configuração em `/proc/cryptochannel/`.

---

## Arquitetura e Estrutura do Código

O projeto é organizado em arquivos separados para garantir a modularidade e a clareza do código:

| Arquivo | Componente Principal | Descrição |
| :--- | :--- | :--- |
| `cryptochannel.c` | **Estrutura e Sincronização** | Gerenciamento do ciclo de vida do módulo, funções `read`/`write`, controle de exclusão mútua (`mutex`) e bloqueio (`wait_queues`, KFIFO). |
| `crypto.c` | **Lógica de Criptografia** | Implementação das funções `encrypt_msg` e `decrypt_msg` que utilizam a chave global do módulo. |
| `procfs.c` | **Interface `/proc`** | Criação do diretório `/proc/cryptochannel/` e funções para `stats` (leitura) e `config` (leitura/escrita). |

---

## ⚙️ Instruções de Compilação e Instalação

### Pré-requisitos

* Distribuição Linux (ambiente de desenvolvimento do kernel instalado).

### 1. Compilação do Módulo

Execute os comandos no diretório raiz do projeto:

```bash
# Limpa binários e módulos antigos
sudo make clean 

# Compila o módulo (gera cryptochannel.ko)
make

# Remove qualquer versão anterior do módulo
sudo rmmod cryptochannel 

# Insere o módulo no kernel
sudo insmod cryptochannel.ko

# Cria o nó do dispositivo de caractere (Major Number 244)
sudo mknod /dev/cryptochannel c 244 0

# Ajusta as permissões de acesso para o usuário atual
sudo chown $(whoami) /dev/cryptochannel
sudo chmod 660 /dev/cryptochannel

# Terminal 1 (Consumidor-Bloqueia)

cat /dev/cryptochannel
# O processo trava (espera na fila de leitura).

# Terminal 2 (Produtor-Desbloqueia)
echo "Teste de Criptografia em SO" > /dev/cryptochannel
# A mensagem é cifrada e o Consumidor no Terminal 1 é acordado e exibe o texto decifrado.

# Uso da interface /proc
# 1. Visualizar Estatísticas (/proc/cryptochannel/stats)
cat /proc/cryptochannel/stats

# Saída Exemplo (após um teste de E/S):
Mensagens Enviadas: 1
Bytes Cifrados: 30
Erros Registrados: 0

# 2. Configurar a Chave (/proc/cryptochannel/config)
# Visualiza a chave e o modo atuais
cat /proc/cryptochannel/config

# Altera a chave de criptografia para "nova_chave"
echo "nova_chave" | sudo tee /proc/cryptochannel/config

# Remoção e Limpeza
# Remove o módulo do kernel (aciona a função de saída)
sudo rmmod cryptochannel

# Remove o nó do dispositivo
sudo rm -f /dev/cryptochannel
