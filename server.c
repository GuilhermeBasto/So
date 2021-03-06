
#include "header.h"
//gcc -o main main.c -lpthread
void print_conf(Config *conf){
  printf("Numero de threads: %d\nNumero de processos: %d\nDuracao de cada processo: %d\nTamanho maximo da fila de espera: %d \n",conf->triagem,conf->n_doutores,conf->dur_turnos,conf->max_fila );
}

void le_config(Config *conf){
  FILE *f;
  int aux[5];
  char str[100];

  f=fopen("config.txt","r");
  if (!f){
    printf("erro ao abrir o ficheiro config");
    return;
  }
  for (int i=0;i<4;i++){
    fgets(str,100,f);
    strtok(str, "=");
    aux[i]=atoi(strtok(NULL, "\n"));
  }
  conf->triagem=aux[0];
  conf->n_doutores=aux[1];
  conf->dur_turnos=aux[2];
  conf->max_fila=aux[3];
}

void cleanup() {
  int i=0;


  old_size = size;
  size =charswriten;
  if ((wstats = mremap(wstats, old_size, size+1, MREMAP_MAYMOVE)) == MAP_FAILED){
    perror("Error extending mapping");
    close(slog);
    exit(1);
  }
  strcat(wstats,"\n");
  munmap(wstats,size);
  for(int i=0;i<conf.triagem+2;i++){
    pthread_cancel(my_thread[i]);
  }

  while(i<conf.n_doutores){
    wait(NULL);
    i++;
  }
  destroi_mq();
  //Elimina semáforos
  sem_unlink("doutoresFim");
  sem_destroy(doutoresFim);
  sem_unlink("Triagem");
  sem_destroy(Triagem);
  sem_unlink("Atendimento");
  sem_destroy(Atendimento);
  //Elimina mutexs

  pthread_mutex_destroy(&stats->mutex[0]);
  pthread_mutex_destroy(&stats->mutex[1]);
  pthread_mutex_destroy(&mutexListaLigada);
  destroi_memoria_partilhada();
  //fechar pipe
  unlink(PIPE_NAME);
  close(fd);

  destroi_lista(fila_espera);
}

void termina(int sign){
  //signal(SIGINT, termina);

  printf("\nTERMINA\n");
  cleanup();
  exit(0);
}
void print_stats(int sign){
  pthread_mutex_lock(&stats->mutex[0]);
  printf("\n--------ESTATISTICAS-----\n");
  printf("NUMERO PACIENTES TRIADOS: %d\n",stats->n_pacientes_triados);
  printf("NUMERO PACIENTES ATENDIDOS: %d\n",stats->n_pacientes_atendidos);
  printf("TEMPO ANTES DE TRIAGEM: %lf\n",stats->t_antes_triagem);
  printf("TEMPO ENTRE TRIAGEM E ATENDIMENTO: %lf\n",stats->t_entre_triagem_atendimento);
  printf("TEMPO TOTAL: %lf\n",stats->tempo_total);
  printf("--------------FIM---------------\n\n");
  pthread_mutex_unlock(&stats->mutex[0]);
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
  stats->teste=0;
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

void cria_mq(){
  if((mq_id = msgget(IPC_PRIVATE,IPC_CREAT|0700))<0){
    perror("Message Queue error");
  }
  else
  printf("Message Queue criada\n");
}
void destroi_mq(){
  if((mq_id = msgctl(mq_id, IPC_RMID, 0))<0){
    perror("Message Queue error");
  }
  else
  printf("Message Queue destroida\n");
}

char *my_itoa(int num, char *str){
  if(str == NULL)
  return NULL;
  else
  sprintf(str, "%d", num);
  return str;
}
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

int inserir_fila(Paciente *p,Lista fila_espera){
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
  printf("---------------LISTA-------------\n");
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
  printf("-------------LISTA_FIM-------------\n");
  printf("\n");

}

void* le_pipe(void *N){
  int i=1;
  int nova_thread;
  char buf[MAX_BUF];
  int triage,atendimento,prioridade,pessoas;
  char *nome,*n,*aux;
  fd_set read_set;
  FD_ZERO(&read_set);

  // Opens the pipe for writing
  if ((fd=open(PIPE_NAME, O_RDONLY)) < 0){
    perror("Cannot open pipe for writing: ");
    exit(0);
  }

  printf("Entrada:");

  while (1) {
    nome=(char *)malloc(sizeof(char)*MAX_BUF);
    aux=(char *)malloc(sizeof(char)*MAX_BUF);
    p.nome=(char*)malloc(sizeof(char)*MAX_BUF);
    n=(char *)malloc(sizeof(char)*5);


    FD_SET(fd,&read_set);
    if( select(fd+1, &read_set, NULL, NULL, NULL) > 0){

      if(FD_ISSET(fd,&read_set)){

          read(fd,buf,sizeof(buf));
          start=clock();
          aux=strtok(buf,"\n");
          if(isalpha(aux[0])){
            if (strcmp(strtok(aux,"="),"TRIAGE")!=0){
              nome=strtok(buf," ");
              triage=atoi(strtok(NULL, " "));
              atendimento=atoi(strtok(NULL, " "));
              prioridade=atoi(strtok(NULL, "\n"));
              strcpy(p.nome,nome);
              p.temp_triagem=triage;
              p.temp_atendimento=atendimento;
              p.prioridade=prioridade;
              p.n_chegada=i;
              inserir_fila(&p,fila_espera);
              sem_post(Triagem);
              //printf("Numero->%d\n",numero);
              //printf("paciente %s inserido com sucesso!\n",p.nome);
              i++;
            }else{
              printf("Alterar threads\n");
              nova_thread=atoi(strtok(NULL,"\n"));
              if(nova_thread<=conf.triagem){
                id_threads=realloc(id_threads,(nova_thread+2));
                my_thread=realloc(my_thread,(nova_thread+2));
                if(nova_thread!=1){
                for (int x=(nova_thread+2);x>=nova_thread+2;x--){

                    pthread_cancel(my_thread[x]);
                    pthread_mutex_lock(&stats->mutex[1]);
                    char* towrite =(char*)malloc(200*sizeof(char));
                    sprintf(towrite,"Thread %d acabou \n",id_threads[x]);
                    //verificar se ainda pode escrever
                    if(charswriten+strlen(towrite)<=size){
                      charswriten+=strlen(towrite);
                      strcat(wstats,towrite);
                      //enviar informaçao para o mmf
                      msync(wstats,size,MS_SYNC);
                      printf("Escrevi o fim da thread na mmf\n");
                      towrite[0]='\n';
                    }
                    //se nao tem espaço para escrever remapp
                    else{
                      printf("Remapping File !\n");
                      old_size = size;
                      size += sysconf(_SC_PAGE_SIZE);
                      if (ftruncate(slog, size) != 0){
                        perror("Error extending file");
                        close(slog);
                        exit(1);
                      }
                      if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
                        perror("Error extending mapping");
                        close(slog);
                        exit(1);
                      }

                      charswriten+=strlen(towrite);
                      strcat(wstats,towrite);
                      //enviar informaçao para o mmf
                      msync(wstats,size,MS_SYNC);
                      printf("Escrevi o fim da thread na mmf\n");
                      towrite[0]='\n';
                    }

                    pthread_mutex_unlock(&stats->mutex[1]);

                  }
                conf.triagem=nova_thread;
                }
              }
              else{
                //aumenta
                id_threads=realloc(id_threads,(nova_thread+2));
                my_thread=realloc(my_thread,(nova_thread+2));
                for (int x=conf.triagem+2;x<nova_thread+2;x++){
                  if(pthread_create(&my_thread[x],NULL,triagem,&id_threads[x])!=0){
                    perror ("pthread_create error");
                  }
                  pthread_mutex_lock(&stats->mutex[1]);
                  char* towrite =(char*)malloc(200*sizeof(char));
                  sprintf(towrite,"Thread %d começou \n",id_threads[x]);
                  //verificar se ainda pode escrever
                  if(charswriten+strlen(towrite)<=size){
                    charswriten+=strlen(towrite);
                    strcat(wstats,towrite);
                    //enviar informaçao para o mmf
                    msync(wstats,size,MS_SYNC);
                    printf("Escrevi o inicio da thread na mmf\n");
                    towrite[0]='\n';
                  }
                  //se nao tem espaço para escrever remapp
                  else{
                    printf("Remapping File !\n");
                    old_size = size;
                    size += sysconf(_SC_PAGE_SIZE);
                    if (ftruncate(slog, size) != 0){
                      perror("Error extending file");
                      close(slog);
                      exit(1);
                    }
                    if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
                      perror("Error extending mapping");
                      close(slog);
                      exit(1);
                    }

                    charswriten+=strlen(towrite);
                    strcat(wstats,towrite);
                    //enviar informaçao para o mmf
                    msync(wstats,size,MS_SYNC);
                    printf("Escrevi o inicio da thread na mmf\n");
                    towrite[0]='\n';
                  }

                  pthread_mutex_unlock(&stats->mutex[1]);
                  conf.triagem=nova_thread;

                }

            }
          }
          }
          else{
            nome=strtok(buf," ");
            pessoas=atoi(nome);
            triage=atoi(strtok(NULL, " "));
            atendimento=atoi(strtok(NULL, " "));
            prioridade=atoi(strtok(NULL, "\n"));
            for(int c=0;c<pessoas;c++){
              strcpy(nome,"grupo");
              n=my_itoa(i,n);
              strcat(nome,n);
              strcpy(p.nome,nome);
              p.temp_triagem=triage;
              p.temp_atendimento=atendimento;
              p.prioridade=prioridade;
              p.n_chegada=i;
              printf("nome->%s\n",p.nome );
              //printf("%s %d %d %d\n",p.nome,p.temp_triagem,p.temp_atendimento,p.prioridade );
              inserir_fila(&p,fila_espera);
              sem_post(Triagem);
            }
            i++;
            printf("grupo de %d pessoas\n",pessoas);
          }
        }else
        imprime_fila(fila_espera);
        close(fd);
        fd = open(PIPE_NAME,O_RDONLY|O_NONBLOCK);
    }
  }
}
void ver_MQ(){
  pid_t id;
  int msq;
  int num_msg;
  int aux=conf.max_fila;

  if((msq = msgctl(mq_id, IPC_STAT, &buf))<0){
    perror("Error msgctl");
  }
  num_msg = buf.msg_qnum;
  printf("NUMERO MSG ->%d\n",num_msg );
  int conta=(float)aux*(float)0.8;
  if ((num_msg >= conta) && stats->teste < 1){
    if ((id= fork()) < 0) {
      perror("fork");
      exit(1);
    }else if (id == 0) {
      trabalho_doc(conf.n_doutores);
    }
    printf("Doutor [%d] Extra \n",getpid());
    pthread_mutex_lock(&stats->mutex[0]);
    stats->id_doutores[conf.n_doutores]=id;
    stats->teste++;
    pthread_mutex_lock(&stats->mutex[0]);
  }
}
void delete(){
  int msq;
  int num_msg;
  int aux=conf.max_fila;
  if((msq = msgctl(mq_id, IPC_STAT, &buf))<0){
    perror("Error msgctl");
  }
  num_msg = buf.msg_qnum;
  printf("NUMERO MSG ->%d\n",num_msg );
  int conta=(float)aux*(float)0.8;
  if (num_msg<=conta){
    if(stats->teste>0){
      pthread_mutex_lock(&stats->mutex[0]);
      printf("Matar Doutor Extra\n");
      waitpid(stats->id_doutores[conf.n_doutores],NULL,0);
      stats->teste--;
      pthread_mutex_unlock(&stats->mutex[0]);
    }
  }
}
//Nesta função uma thread de cada vez (&mutexListaLigad vai ao primeiro elemento da lista e envia a estrutra para a MQ);
void* triagem(void* id){
  //(int*) id;

  pthread_mutex_lock(&mutexListaLigada);
  Lista aux,next;

  while (1) {

    sem_wait(Triagem);
    end=clock();
    antes_triagem=((double) (end - start)) / CLOCKS_PER_SEC;

    //printf("TEMPO ANTES DE TRIAGEM! -> %lf",antes_triagem);
    //printf("ID THREAD->%d\n",id_threads[*id]);
    aux=fila_espera;
    next = aux->next;
    printf("----------TRIAGEM-------------\n");
    if(next){

      mymsg.mtype=next->paciente.prioridade;
      strcpy(mymsg.nome,next->paciente.nome);
      mymsg.temp_triagem=next->paciente.temp_triagem;
      mymsg.temp_atendimento=next->paciente.temp_atendimento;
      mymsg.antes_triagem=antes_triagem;//msgrcv(mq_id, &mymsg,sizeof(mymsg)-sizeof(long), 0, 0);
      pthread_mutex_lock(&stats->mutex[0]);
      stats->n_pacientes_triados++;
      stats->t_antes_triagem=antes_triagem;
      pthread_mutex_unlock(&stats->mutex[0]);
      /* escrever os pacinete triados na mmf*/

      pthread_mutex_lock(&stats->mutex[1]);
      char* towrite =(char*)malloc(200*sizeof(char));
      sprintf(towrite,"Paciente  %s Triado\n",mymsg.nome);
      //verificar se ainda pode escrever
      if(charswriten+strlen(towrite)<=size){
        charswriten+=strlen(towrite);
        strcat(wstats,towrite);
        //enviar informaçao para o mmf
        msync(wstats,size,MS_SYNC);
        printf("Paciente  %s Triado\n",mymsg.nome);
        towrite[0]='\n';
      }
      //se nao tem espaço para escrever remapp
      else{
        printf("Remapping File !\n");
        old_size = size;
        size += sysconf(_SC_PAGE_SIZE);
        if (ftruncate(slog, size) != 0){
          perror("Error extending file");
          close(slog);
          exit(1);
        }
        if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
          perror("Error extending mapping");
          close(slog);
          exit(1);
        }

        charswriten+=strlen(towrite);
        strcat(wstats,towrite);
        //enviar informaçao para o mmf
        msync(wstats,size,MS_SYNC);
        printf("Escrevi o paciente triado na mmf\n");
        towrite[0]='\n';
      }
    }
    pthread_mutex_unlock(&stats->mutex[1]);

    msgsnd(mq_id,&mymsg,sizeof(Mymsg)-sizeof(long),0);
    ver_MQ();
    aux->next=next->next;
    free(next);

    sem_post(Atendimento);

  }

  printf("----------TRIAGEM_FIM-------------\n");
  pthread_mutex_unlock(&mutexListaLigada);

}

void trabalho_doc(int i){
  //printf("vou comecar o trabalho\n");
  sem_wait(Atendimento);
  printf("------------------ATENDIMENTO-----------------\n");
  pid_t pid = getpid();

  start=clock();
  pthread_mutex_lock(&stats->mutex[1]);
  char* towrite =(char*)malloc(200*sizeof(char));
  sprintf(towrite,"Inicio Doutor: %ld\n",(long)pid);
  //verificar se ainda pode escrever
  if(charswriten+strlen(towrite)<=size){
    charswriten+=strlen(towrite);
    strcat(wstats,towrite);
    //enviar informaçao para o mmf
    msync(wstats,size,MS_SYNC);
    printf("Escrevi o inicio do doutor na mmf\n");
    towrite[0]='\n';
  }
  //se nao tem espaço para escrever remapp
  else{
    printf("Remapping File !\n");
    old_size = size;
    size += sysconf(_SC_PAGE_SIZE);
    if (ftruncate(slog, size) != 0){
      perror("Error extending file");
      close(slog);
      exit(1);
    }
    if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
      perror("Error extending mapping");
      close(slog);
      exit(1);
    }

    charswriten+=strlen(towrite);
    strcat(wstats,towrite);
    //enviar informaçao para o mmf
    msync(wstats,size,MS_SYNC);
    printf("Escrevi o inicio do doutor na mmf\n");
    towrite[0]='\n';
  }

  pthread_mutex_unlock(&stats->mutex[1]);


  delete();
  msgrcv(mq_id, &mymsg,sizeof(Mymsg)-sizeof(long), -3, 0);
  end=clock();
  entre_triagem_atendimento=((double) (end - start)) / CLOCKS_PER_SEC;
  //printf("TEMPO ENTRE TRIAGEM E ATENDIMENTO ->%lf\n",entre_triagem_atendimento);
  printf("--MQ--\n");
  printf("Paciente-> %s %d %d\n",mymsg.nome,mymsg.temp_triagem,mymsg.temp_atendimento);
  printf("Doutor %d comecou a trabalhar\n",pid);
  if(mymsg.temp_atendimento<=conf.dur_turnos){
    printf("Consulta de %d segundos\n",mymsg.temp_atendimento);
    sleep(conf.dur_turnos);
  }
  else{
    printf("Consulta de %d segundos\n",mymsg.temp_atendimento);
    sleep(mymsg.temp_atendimento);
  }
  printf("Doutor %d acabou\n", pid);
  pthread_mutex_lock(&stats->mutex[0]);
  stats->tempo_total+=(mymsg.temp_atendimento+mymsg.temp_triagem+mymsg.antes_triagem+entre_triagem_atendimento);
  stats->n_pacientes_atendidos++;
  stats->t_entre_triagem_atendimento=entre_triagem_atendimento;
  stats->id_doutores[i]=-1;

  pthread_mutex_unlock(&stats->mutex[0]);
  //kill(pid,SIGUSR1);
  pthread_mutex_lock(&stats->mutex[1]);
  sprintf(towrite,"Fim Doutor: %ld\n",(long)pid);
  //verificar se ainda pode escrever
  if(charswriten+strlen(towrite)<=size){
    charswriten+=strlen(towrite);
    strcat(wstats,towrite);
    //enviar informaçao para o mmf
    msync(wstats,size,MS_SYNC);
    printf("Escrevi o fim do doutor na mmf\n");
    towrite[0]='\n';
  }
  //se nao tem espaço para escrever remapp
  else{
    printf("Remapping File !\n");
    old_size = size;
    size += sysconf(_SC_PAGE_SIZE);
    if (ftruncate(slog, size) != 0){
      perror("Error extending file");
      close(slog);
      exit(1);
    }
    if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
      perror("Error extending mapping");
      close(slog);
      exit(1);
    }

    charswriten+=strlen(towrite);
    strcat(wstats,towrite);
    //enviar informaçao para o mmf
    msync(wstats,size,MS_SYNC);
    printf("Escrevio fim do doutor na mmf\n");
    towrite[0]='\n';
  }

  pthread_mutex_unlock(&stats->mutex[1]);


  sem_post(doutoresFim);

  printf("------------------ATENDIMENTO_FIM-----------------\n");


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
      trabalho_doc(i);
    }
    else{
      pthread_mutex_lock(&stats->mutex[0]);
      stats->id_doutores[i]=id;
      pthread_mutex_unlock(&stats->mutex[0]);
    }
  }
}

void* substituirDoutor (void *id){
  int a=0;
  pid_t novo;

  while(1){
    sem_wait(doutoresFim);
    //printf("post doutor!!!\n");pthread_create(&my_thread[0],NULL,le_pipe,&id_threads[0])!=0){


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
          pthread_mutex_lock(&stats->mutex[0]);
          stats->id_doutores[a]=novo;
          pthread_mutex_unlock(&stats->mutex[0]);
          break;
        }
      }
    }
  }
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
void mmf(){

  slog = open("server.log", O_RDWR|O_CREAT|O_TRUNC,FILE_MODE);
  if(slog<0){
    printf("Error open file\n");
    exit(1);
  }
  status=fstat(slog, &s);
  if(status <0) {// To obtain file size
    perror("Error in fstat");
    exit(1);
  }
  size=s.st_size;
  if((wstats=mmap(0,size+1,PROT_READ|PROT_WRITE, MAP_SHARED,slog,0))==(caddr_t)-1){
    perror("Error in mmap\n");
    exit(1);
  }

  old_size = size;
  size += sysconf(_SC_PAGE_SIZE);

  if (ftruncate(slog, size) != 0){
    perror("Error extending file");
    close(slog);
    exit(1);
  }

  if ((wstats = mremap(wstats, old_size, size, MREMAP_MAYMOVE)) == MAP_FAILED){
    perror("Error extending mapping");
    close(slog);
    exit(1);
  }
}
void inicio(){
  signal(SIGINT,termina);
  signal(SIGUSR1,print_stats);
  signal(SIGTSTP,SIG_IGN);
  le_config(&conf);
  cria_pipe();
  cria_mq();
  fila_espera=cria_lista();
  criar_memoria_partilhada();
  pthread_mutexattr_t mattr;
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&stats->mutex[0], &mattr);
  pthread_mutex_init(&stats->mutex[1], &mattr);
  mmf();

  sem_unlink("doutoresFim");
  doutoresFim=sem_open("doutoresFim",O_CREAT| O_EXCL,0777,0);
  sem_unlink("Triagem");
  Triagem=sem_open("Triagem",O_CREAT| O_EXCL,0777,0);
  sem_unlink("Atendimento");
  Atendimento=sem_open("Atendimento",O_CREAT| O_EXCL,0777,0);
  criar_doutores();
  criar_threads();
  system("clear");
  printf("------------------------------------------------------------\n");
  while (wait(NULL)!=-1);
}

int main(int argc, char const *argv[]){
  inicio();
  //cleanup();
}
