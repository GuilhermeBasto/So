#include "header.h"



int main(){
    int i;
    char buf[MAX_BUF];
    //pipesem=sem_open("pipesem",O_CREAT| O_EXCL,0777,0);
    // Opens the pipe for writing
    if ((fd=open(PIPE_NAME, O_WRONLY)) < 0){
        perror("Cannot open pipe for writing: ");
        exit(0);
    }
    
    printf("Entrada:");
    while (1) {
          pthread_mutex_unlock(&mutexPipe);
          printf("[Cliente]-> ");
          fgets(buf,sizeof(buf), stdin);
          //printf("%s\n",buf );
          write(fd, buf, sizeof(buf));
          printf("Numero->%d\n",numero);


      }

  return 0;
}
