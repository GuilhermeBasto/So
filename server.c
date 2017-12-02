#include "header.h"
//gcc -o main main.c -lpthread


Lista cria_lista(){
  Lista aux;
  aux=(Lista)malloc(sizeof(Lista_node));
  while(aux==NULL){
    printf("Erro na criacao da lista!\n");
    return aux;
  }
  aux->next=NULL;

  return aux;
}

Lista destroi_lista(Lista lista){
  Lista aux;
  while (lista->next!=NULL) {
    aux=lista;
    lista=lista->next;
    free(aux);
  }
  free(lista);
  return NULL;
}

void le_config(Config *conf){
  FILE *f;
  int aux[5];
  char *lixo;
  char str[100];

  f=fopen("config.txt","r");
  if (!f){
    printf("erro ao abrir o ficheiro config");
    return;
  }
  for (int i=0;i<4;i++){
    fgets(str,100,f);
    lixo=strtok(str, "=");
    aux[i]=atoi(strtok(NULL, "\n"));
  }
  conf->triagem=aux[0];
  conf->n_doutores=aux[1];
  conf->dur_turnos=aux[2];
  conf->max_fila=aux[3];
}

void print_conf(Config *conf){
  printf("Numero de threads: %d\nNumero de processos: %d\nDuracao de cada processo: %d\nTamanho maximo da fila de espera: %d \n",conf->triagem,conf->n_doutores,conf->dur_turnos,conf->max_fila );
}


void escreve_shm(){
  stats->n_pacientes_triados+=1;
  stats->n_pacientes_atendidos+=1;
  stats->t_antes_triagem+=1;
  stats->t_entre_triagem_atendimento+=1;
  stats->tempo_total+=1;
  //   printf("escrito na shm\n");
}


void trabalho_doc(int i){
  //printf("vou comecar o trabalho\n");
  pid_t pid = getpid();
  printf("Doutor %d comecou a trabalhar\n",pid);
  sleep(20);

  printf("Doutor %d acabou\n", pid);
  pthread_mutex_lock(&mutex);

  escreve_shm();
  //escreve_shm();
  stats->id_doutores[i]=-1;
  pthread_mutex_unlock(&mutex);

  sem_post(doutoresFim);
  sem_wait(terminaDoutor);
  exit(0);
}

void criar_doutores(){
  pid_t id;
  int i;
  for (i = 0; i < conf.n_doutores; i++) {
    if ((id= fork()) < 0) {
      perror("fork");
      exit(1);
    }else if (id == 0) {
      printf("-----------------------\n");
      printf("filho\n");
      trabalho_doc(i);
    }
    else{
      printf("-----------------------\n");
      printf("pai\n");
      stats->id_doutores[i]=id;




    }
    //printf("pid doutor %d - %d\n",i,id);
  }
}

void* substituirDoutor (void *id){
  int a=0;
  pid_t novo;

  while(1){
    //printf("post doutor!!!\n");
    sem_wait(doutoresFim);
    for(a=0;a<conf.n_doutores;a++){
      if(stats->id_doutores[a]==-1){
        //printf("encontrei doutor morto\n");
        if ((novo = fork()) < 0) {
          perror("fork");
          exit(1);
        }else if (novo == 0) {
          //printf("doutor novo\n");

          trabalho_doc(a);
        }
        else{
          //printf("--->adicionei um doc\n");
          stats->id_doutores[a]=novo;


          break;
        }
      }
    }

  }
}



void cleanup() {
  int i=0;
  for(int i=0;i<conf.triagem+2;i++){
    pthread_cancel(my_thread[i]);
  }
  while(i<conf.n_doutores){
    wait(NULL);
    i++;
  }
  //Elimina o semáforo
  sem_unlink("doutoresFim");
  sem_destroy(doutoresFim);
  //Eliminar a memoria partilhada

  pthread_mutex_destroy(&mutexPipe);
  pthread_mutex_destroy(&mutex);
  destroi_memoria_partilhada();
  unlink(PIPE_NAME);
  close(fd);
  free(id_threads);
  free(my_thread);
  destroi_lista(fila_espera);
}

void termina(int sign){
  //signal(SIGINT, termina);

  printf("\nTERMINA\n");
  cleanup();
  exit(0);
}

void criar_memoria_partilhada(){
  if((shmid = shmget(IPC_PRIVATE, sizeof(Estatisticas),IPC_CREAT | 0766)) != -1){
    stats = (Estatisticas *) shmat(shmid,NULL,0);
    printf("Criacao shared memory\n");
  }
  else
  perror("Creating statistics shared memory\n");
  stats->n_pacientes_triados=0;
  stats->n_pacientes_atendidos=0;
  stats->t_antes_triagem=0;
  stats->t_entre_triagem_atendimento=0;
  stats->tempo_total=0;
}
void destroi_memoria_partilhada(){

  if(shmdt(stats) == 0){
    shmctl(shmid,IPC_RMID,NULL);
    printf("A destroir memoria partilhada!\n");
  }
  else
  perror("Destroindo memoria partilhada\n");



}


void cria_pipe(){
  if((mkfifo(PIPE_NAME,O_CREAT|O_EXCL|0600)<0) && (errno != EEXIST)){
    perror("Cannot create pipe: ");
    exit(0);
  }else
  printf("pipe criado\n");


}
char *my_itoa(int num, char *str){
  if(str == NULL)
  return NULL;
  else
  sprintf(str, "%d", num);
  return str;
}

int inserir_fila(Paciente *p){
  Lista lista,novo;
  lista=fila_espera;
  novo=(Lista)malloc(sizeof(Lista_node));
  novo->paciente.nome=malloc(sizeof(char)*50);

  if(novo!=NULL){
    strcpy(novo->paciente.nome,p->nome);
    novo->paciente.n_chegada=p->n_chegada;
    novo->paciente.temp_triagem=p->temp_triagem;
    novo->paciente.temp_atendimento=p->temp_atendimento;
    novo->paciente.prioridade=p->prioridade;
    //printf("%s %d %d %d\n",novo->paciente.nome,novo->paciente.temp_triagem,novo->paciente.temp_atendimento,novo->paciente.prioridade );
    //inserir no fim
    while (lista->next!=NULL) {
      lista=lista->next;
    }
      lista->next=novo;
      novo->next=NULL;
      //printf("%s %d %d %d\n",fila_espera->paciente.nome,fila_espera->paciente.temp_triagem,fila_espera->paciente.temp_atendimento,fila_espera->paciente.prioridade );

  }
  else {
    printf("Erro na alocacao de memoria!\n");
    return -1;
  }
  return 0;
}

void imprime_fila(Lista aux){
    aux=fila_espera->next;
    if(aux==NULL){
        printf("Nao ha pacientes em fila de espera!\n\n\n");
        return;
    }
    else{
        while(aux!=NULL){
          printf("%s %d %d %d\n",aux->paciente.nome,aux->paciente.temp_triagem,aux->paciente.temp_atendimento,aux->paciente.prioridade );
          aux=aux->next;
        }
      }
    printf("\n");

}

void* le_pipe(void *N){
  int i=1;
  char buf[MAX_BUF];
  int triagem,atendimento,prioridade,pessoas;
  char *nome,*n;
  fd_set read_set;
  FD_ZERO(&read_set);
  nome=(char *)malloc(sizeof(char)*MAX_BUF);
  p.nome=(char*)malloc(sizeof(char)*MAX_BUF);
  n=(char *)malloc(sizeof(char)*5);

  // Opens the pipe for writing
  if ((fd=open(PIPE_NAME, O_RDONLY)) < 0){
    perror("Cannot open pipe for writing: ");
    exit(0);
  }
  pthread_mutex_lock(&mutexPipe);
  printf("Entrada:");

  while (1) {

    FD_SET(fd,&read_set);
    if( select(MAX_BUF, &read_set, NULL, NULL, NULL) > 0){
        if(FD_ISSET(fd,&read_set)){
        if (numero<conf.max_fila) {
          pthread_mutex_unlock(&mutexMaxFila);

          read(fd,buf,sizeof(buf));
          nome=strtok(buf," ");
          if(isalpha(nome[0])){
            triagem=atoi(strtok(NULL, " "));
            atendimento=atoi(strtok(NULL, " "));
            prioridade=atoi(strtok(NULL, "\n"));
            strcpy(p.nome,nome);
            p.temp_triagem=triagem;
            p.temp_atendimento=atendimento;
            p.prioridade=prioridade;
            p.n_chegada=i;
            inserir_fila(&p);
            numero++;
            sem_post(Triagem);
            printf("Numero->%d\n",numero);
            printf("paciente %s inserido com sucesso!\n",p.nome);
            i++;
          }
          else{
            pessoas=atoi(nome);
            triagem=atoi(strtok(NULL, " "));
            atendimento=atoi(strtok(NULL, " "));
            prioridade=atoi(strtok(NULL, "\n"));
            for(int c=0;c<pessoas;c++){
              strcpy(nome,"grupo");
              n=my_itoa(i,n);
              strcat(nome,n);
              strcpy(p.nome,nome);
              p.temp_triagem=triagem;
              p.temp_atendimento=atendimento;
              p.prioridade=prioridade;
              p.n_chegada=i;
              //printf("%s %d %d %d\n",p.nome,p.temp_triagem,p.temp_atendimento,p.prioridade );
              inserir_fila(&p);
              numero++;
              sem_post(Triagem);
              printf("Numero->%d\n",numero);
            }
            i++;
          printf("grupo de %d pessoas\n",pessoas);
          }
        }else
            pthread_mutex_lock(&mutexMaxFila);

          imprime_fila(fila_espera);
          close(fd);
          fd = open(PIPE_NAME,O_RDONLY|O_NONBLOCK);
        }
      }

  }
}
//Nesta função uma thread de cada vez (&mutexListaLigad vai ao primeiro elemento da lista e envia a estrutra para a MQ);
void* triagem(void* A){
    pthread_mutex_lock(&mutexListaLigada);
    Lista ptr=fila_espera;
    while (1) {
       printf("----------TRIAGEM-------------\n");
        sem_wait(Triagem);
        if(ptr!=NULL){

          printf("Paciente%s\n",ptr->paciente.nome);
          /*ptr=ptr->next;
          ptr->next=fila_espera->next;*/
          ptr=ptr->next;
          pthread_mutex_unlock(&mutexListaLigada);
        }
        else
            printf("lista vazia\n");
        printf("----------TRIAGEM_FIM-------------\n");
        pthread_mutex_unlock(&mutexListaLigada);
    }

    /*if( != NULL){
        Paciente paciente = ptr->paciente;
        aux = aux->next;
        free(ptr);
        msgsnd(mq_id,&paciente,sizeof(paciente)-sizeof(long),0);

    }*/


}
void criar_threads(){


  int i=0;
  my_thread = (pthread_t*)malloc((conf.triagem+2)*sizeof(pthread_t));
  id_threads = malloc(sizeof(int)*conf.triagem+2);
  //cria thread para estar a espera do pipe
  if(pthread_create(&my_thread[0],NULL,le_pipe,&id_threads[0])!=0){
    perror ("pthread_create error");
  }
  //criar a thread responsavel por criar novos doutores quando uns acabam
  if(pthread_create(&my_thread[1],NULL,substituirDoutor,&id_threads[0])!=0){
    perror ("pthread_create error");
  }
  id_threads[1]=0;
  // create n threads para a triagem
  for (i = 2; i < conf.triagem+2; i++) {
    id_threads[i]=i;
    if(pthread_create(&my_thread[i], NULL, triagem, &id_threads[i])!=0){
      perror("Nao consegui criar uma thread triagem\n");
    }
  }


}

void inicio(){
  cria_pipe();
  le_config(&conf);
  fila_espera=cria_lista();
  criar_memoria_partilhada();
  sem_unlink("doutoresFim");
  doutoresFim=sem_open("doutoresFim",O_CREAT| O_EXCL,0777,0);
  sem_unlink("Triagem");
  Triagem=sem_open("Triagem",O_CREAT| O_EXCL,0777,1);
  assert(mq_id = msgget(IPC_PRIVATE,IPC_CREAT|0700));
  criar_doutores();
  criar_threads();
  while (wait(NULL) != -1);
}

int main(int argc, char const *argv[]){

  signal(SIGINT,termina);
  inicio();

}