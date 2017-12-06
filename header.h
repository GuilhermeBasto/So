#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>
#include <ctype.h>
#include <errno.h>
#include <sys/select.h>
#include <assert.h>

#define MAX_BUF 1024
#define PIPE_NAME "input_pipe"
#define MQ -3


typedef struct Config{
    int triagem; //numero de threades
    int n_doutores; //numero de processos
    int dur_turnos; //duração do turno em segundos
    int max_fila; //tamanho maximo da fila de atendimento

}Config;

typedef struct Paciente{
    char *nome;
    int n_chegada;
    int temp_triagem;
    int temp_atendimento;
    int prioridade;
}Paciente;



//lista ligada para servir de fila de espera
typedef struct lnode * Lista;
typedef struct lnode{
    Paciente paciente;
    Lista next;
}Lista_node;

typedef struct Estatisticas{
    int n_pacientes_triados;
    int n_pacientes_atendidos;
    int t_antes_triagem;
    int t_entre_triagem_atendimento;
    int tempo_total;
    pid_t id_doutores[10];
}Estatisticas;

//Estrutra da MQ
typedef struct Mymsg{
    long mtype;
    char nome[MAX_BUF];
    int temp_triagem,temp_atendimento;
}Mymsg;


Paciente p;
Lista fila_espera;
Estatisticas *stats;
Config conf;
int shmid;
pthread_t *my_thread;
int * id_threads;
Mymsg mymsg;
int mq_id;
int fd;
struct msqid_ds *buf;


//thread_mutex para controlar a escrita da estatistica na memoria partilhada
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexListaLigada = PTHREAD_MUTEX_INITIALIZER;

//Semafora para controlar quantos doutores ja acabaram o turno.
//struct msqid_ds buf

sem_t *doutoresFim;
sem_t *Triagem;
sem_t *Atendimento;
Lista destroi_lista(Lista lista);
void destroi_memoria_partilhada();
void cria_pipe();
