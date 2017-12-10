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
#include <sys/mman.h>

#define MAX_BUF 1024
#define PIPE_NAME "input_pipe"
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


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
    double t_antes_triagem;
    double t_entre_triagem_atendimento;
    double tempo_total;
    int teste;
    pid_t id_doutores[20];
    pthread_mutex_t mutex;

}Estatisticas;

//Estrutra da MQ
typedef struct Mymsg{
    long mtype;
    char nome[MAX_BUF];
    int temp_triagem,temp_atendimento;
    double antes_triagem;
}Mymsg;


Paciente p;
Lista fila_espera;
Estatisticas *stats;
Config conf;
int shmid;
pthread_t *my_thread;
int * id_threads;
Mymsg mymsg;Paciente p;
Lista fila_espera;
Estatisticas *stats;
Config conf;
int shmid;
pthread_t *my_thread;
int mq_id;
int fd;
struct msqid_ds buf;
clock_t start,end;
double antes_triagem,entre_triagem_atendimento;

//thread_mutex para controlar a escrita da estatistica na memoria partilhada
pthread_mutex_t mutexListaLigada = PTHREAD_MUTEX_INITIALIZER;

//Semafora para controlar quantos doutores ja acabaram o turno.
sem_t *doutoresFim;
sem_t *Triagem;
sem_t *Atendimento;
/*----------------------------------------------------------------------------
  Funções
-----------------------------------------------------------------------------*/
void print_conf(Config *conf);
void le_config(Config *conf);
void cleanup();
void termina(int sign);
void print_stats(int sign);
void criar_memoria_partilhada();
void destroi_memoria_partilhada();
void cria_pipe();
void cria_mq();
void destroi_mq();
char *my_itoa(int num, char *str);
Lista cria_lista();
Lista destroi_lista(Lista lista);
int inserir_fila(Paciente *p,Lista fila_espera);
void imprime_fila(Lista aux);
void* le_pipe(void *N);
void ver_MQ();
void delete();
void* triagem(void* id);
void trabalho_doc(int i);
void criar_doutores();
void* substituirDoutor (void *id);
void criar_threads();
void inicio();
