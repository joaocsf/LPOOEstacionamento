/** Version: 1.0
  * Author: Grupo T1G09
  *
  */

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "viatura.h"

#define DIRECTORY_LENGTH 4096
#define FILE_LENGTH 255

int n_total_lugares;
int lugares_ocupados = 0;
int t_abertura;
char encerrou = 0;
int fileLog = 0;
int tempoInicial;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void * arrumador_thread(void * args);

void * controlador_thread(void * args){

  int nr,fd, fd_dummy;
  pthread_t tid;

  if((nr = mkfifo((char *)args, 0644)) == -1){//Creating FIFO
    perror((char *)args);
    return NULL;
  }

  if((fd = open((char *)args,O_RDONLY)) != -1){//Opening FIFO with read only
    perror((char *)args);
    return NULL;
  }

  if((fd_dummy = open((char *)args,O_WRONLY)) != -1){//Opening FIFO with write only
    perror((char *)args);
    close(fd);
    unlink((char *)args);
    return NULL;
  }



  while(!encerrou){
    //Ler viaturas
    if(pthread_create(&tid, NULL , arrumador_thread , NULL)){
      printf("Error Creating Thread!\n");
      close(fd);
      close(fd_dummy);
      unlink((char *)args);
      return NULL;
    }
    pthread_detach(tid);
    //Ler continuamente as viaturas que vao chegando
  }
  //Fechou
  //Ler as restantes viaturas;

  if((nr = unlink((char *)args)) == -1){//Deleting FIFO
    perror((char *)args);
    close(fd);
    close(fd_dummy);
    unlink((char *)args);
    return NULL;
  }

  close(fd);
  close(fd_dummy);
  unlink((char *)args);
  return NULL;
}

Viatura* lerViatura(int fd){
  Viatura* v = (Viatura *)malloc(sizeof(Viatura *));
  read(fd,v,sizeof(Viatura));
  return v;
}

void * arrumador_thread(void * args){

  int resposta = 0;
  Viatura * v = (Viatura *)args;
  //Criar FIFO
  pthread_mutex_lock(&mutex);//Secção Critica
  if(encerrou){
    resposta = RES_ENCERRADO;
  } else {
    if(n_total_lugares == lugares_ocupados){
      resposta = RES_CHEIO;
    }else {
      lugares_ocupados++;
      resposta = RES_ENTRADA;
    }
  }
  pthread_mutex_unlock(&mutex);

  //Enviar a resposta para o FIFO

  if(resposta != RES_ENTRADA){
    //Terminar a thread;
    return NULL;
  }
  //turn on local temporizador;
  //Saida
  pthread_mutex_lock(&mutex);//Seccção Critica
  lugares_ocupados--;
  pthread_mutex_unlock(&mutex);
  return NULL;
}

int main(int argc, char *argv[]){

  if(argc != 3){
    printf("Error <Usage>: %s <N_LUGARES> <T_ABERTURA>\n",argv[0]);
    exit(1);
  }

  char path[DIRECTORY_LENGTH + FILE_LENGTH];
  realpath(".", path);
  sprintf(path, "%s/%s", path, "parque.log");

  if( (fileLog = open(path, O_CREAT | O_WRONLY | O_TRUNC , S_IRWXU)) == -1){
    printf("Error Creating Thread!\n");
    exit(2);
  }

  write(fileLog, "t(ticks) ; n_lug ; id_viat ; observ\n" ,37);

  n_total_lugares = atoi(argv[1]);
  t_abertura = atoi(argv[2]);
  clock_t tempoInicial = clock();

  pthread_t tN, tS, tE, tO;
  if (pthread_create(&tN, NULL, controlador_thread, "/tmp/fifoN")){
    printf("Error Creating Thread!\n");
    exit(3);
  }
  if (pthread_create(&tS, NULL, controlador_thread, "/tmp/fifoS")){
    printf("Error Creating Thread!\n");
    exit(3);
  }
  if (pthread_create(&tE, NULL, controlador_thread, "/tmp/fifoE")){
    printf("Error Creating Thread!\n");
    exit(3);
  }
  if (pthread_create(&tO, NULL, controlador_thread, "/tmp/fifoO")){
    printf("Error Creating Thread!\n");
    exit(3);
  }
  pthread_join(tN,NULL);
  pthread_join(tS,NULL);
  pthread_join(tE,NULL);
  pthread_join(tO,NULL);

  close(fileLog);
  return 0;
}
