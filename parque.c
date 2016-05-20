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
#include <semaphore.h>
#include <signal.h>

#include "viatura.h"

#define DIRECTORY_LENGTH 4096
#define FILE_LENGTH 255
#define MILLION 1000000

int n_total_lugares;
int lugares_ocupados = 0;
int t_abertura;
char encerrou = 0;
int fileLog = 0;
clock_t tempoInicial;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t * sem;

void sigPipe(int id){
  printf("SIG PIPEE!!!!\n");

}

void * arrumador_thread(void * args);
Viatura* lerViatura(int fd);

void debugLog(unsigned int tempo , unsigned int numLugares, unsigned int numeroViatura, char* obs){
  char text[DIRECTORY_LENGTH + FILE_LENGTH];

  sprintf(text, " %10d ; %5d ; %7d ; %s\n" , tempo, numLugares, numeroViatura, obs);

  write(fileLog, text, strlen(text));
}

void * controlador_thread(void * args){


  int nr,fd, fd_dummy;
  pthread_t tid;
  printf("CriarFifo\n");

  if((nr = mkfifo((char *)args, 0644)) == -1){//Creating FIFO
    perror((char *)args);
    return NULL;
  }

  printf("Abertura\n");
  if((fd = open((char *)args,O_RDONLY)) == -1){//Opening FIFO with read only
    perror((char *)args);
    return NULL;
  }
printf("Espera\n");
  if((fd_dummy = open((char *)args,O_WRONLY | O_NONBLOCK)) == -1){//Opening FIFO with write only
    perror((char *)args);
    close(fd);
    unlink((char *)args);
    return NULL;
  }

  Viatura* viaturaTemp;
  while( (viaturaTemp = lerViatura(fd)) !=NULL){
    printf("Nova Viatura\n");
    if( viaturaTemp->portaEntrada == 'X'){ //Se Terminou
      close(fd_dummy);
      printf("Terminar!\n");
      sem_wait(sem);
      continue;
    }

    if(pthread_create(&tid, NULL , arrumador_thread , viaturaTemp)){
      printf("Error Creating Thread!\n");
      close(fd);
      close(fd_dummy);
      unlink((char *)args);
      return NULL;
    }
    pthread_detach(tid);
  }

  close(fd);

  if((nr = unlink((char *)args)) == -1){//Deleting FIFO
    perror((char *)args);

    sem_post(sem);
    return NULL;
  }

  printf("Libertar!\n");
  sem_post(sem);
  return NULL;
}

Viatura* lerViatura(int fd){
  Viatura* v = (Viatura *)malloc(sizeof(Viatura *));
  int returnValue = read(fd,v,sizeof(Viatura));
  if( returnValue == -1 || returnValue == 0){ //Caso nao consiga ler viaturas
    free(v);
    return NULL;
  } else {
    return v;
  }
}

void mySleep(int ticks){//Recebe os ticks para dormir
  double myS = ticks / sysconf(_SC_CLK_TCK);
  struct timespec * req, * rem;
  req = malloc(sizeof(struct timespec));
  rem = malloc(sizeof(struct timespec));
  req->tv_sec = myS / 1;
  req->tv_nsec = (long)(myS - req->tv_sec) * MILLION;
  if(nanosleep(req,rem) != 0){
    nanosleep(rem,NULL);
  }
  free(req);
  free(rem);
  return;
}

void * arrumador_thread(void * args){

  char resposta = 0;
  Viatura * v = (Viatura *)args;

//Criar FIFO
  char fifoViatura[DIRECTORY_LENGTH + FILE_LENGTH] ;
  sprintf(fifoViatura, "/tmp/viatura%d", v->numeroID);
  int fdViatura = 0;


  if( (fdViatura = open(fifoViatura, O_WRONLY) ) == -1){
    perror(fifoViatura);
    free(v);
    return NULL;
  }

  pthread_mutex_lock(&mutex);//Secção Critica
  if(encerrou){
    resposta = RES_ENCERRADO;
    debugLog(clock() - tempoInicial , n_total_lugares -  lugares_ocupados , v->numeroID, "encerrado");
  } else {
    if(n_total_lugares == lugares_ocupados){
      resposta = RES_CHEIO;

      debugLog(clock() - tempoInicial , n_total_lugares -  lugares_ocupados , v->numeroID, "cheio");
    }else {
      lugares_ocupados++;
      resposta = RES_ENTRADA;
      debugLog(clock() - tempoInicial , n_total_lugares -  lugares_ocupados , v->numeroID, "estacionamento");
    }
  }
  pthread_mutex_unlock(&mutex);

//Enviar a resposta para o FIFO
  write(fdViatura, &resposta, sizeof(char));

  if(resposta != RES_ENTRADA){
//Terminar a thread;
    close(fdViatura);
    free(v);
    return NULL;
  }
//turn on local temporizador;

  //clock_t tickInicial = clock();
  //while(clock() - tickInicial < v->tempoEstacionamento);
  mySleep(v->tempoEstacionamento);

  //Saida
  resposta = RES_SAIDA;
  write(fdViatura, &resposta, sizeof(char) );
  close(fdViatura);

  pthread_mutex_lock(&mutex);//Seccção Critica

  debugLog(clock() - tempoInicial , n_total_lugares -  lugares_ocupados , v->numeroID, "saida");
  lugares_ocupados--;

  pthread_mutex_unlock(&mutex);

  free(v);
  return NULL;
}

int main(int argc, char *argv[]){

  signal(SIGPIPE, sigPipe);

  if(argc != 3){//Verificação dos argumentos
    printf("Error <Usage>: %s <N_LUGARES> <T_ABERTURA>\n",argv[0]);
    exit(1);
  }

  char path[DIRECTORY_LENGTH + FILE_LENGTH];
  realpath(".", path);
  sprintf(path, "%s/%s", path, "parque.log");

  if( (fileLog = open(path, O_CREAT | O_WRONLY | O_TRUNC , S_IRWXU)) == -1){//Criaçao e abertura do ficheiro log
    printf("Error Creating Thread!\n");
    exit(2);
  }

  write(fileLog, "t(ticks) ; n_lug ; id_viat ; observ\n" ,37);

/*Tentiva de abertura de um semaforo do menos nome para verificar se ja existe um com esse nome*/
  if( (sem = sem_open("/semaforo", 0, S_IRWXU, 1)) != SEM_FAILED){
    printf("Semaforo Existe!\n");
    sem_unlink("/semaforo");
    sem_destroy(sem);
  }

  /*Abertura e criacao de um semaforo para sincronizacao de ambos os programas*/
  if((sem = sem_open("/semaforo",O_CREAT, S_IRWXU,1)) == SEM_FAILED){
    perror("/semaforo");
    printf("Error!\n");
    exit(3);
  }

/*Passagem dos argumentos para variaveis globais*/
  n_total_lugares = atoi(argv[1]);
  t_abertura = atoi(argv[2]);
  tempoInicial = clock();

/*Criacao das 4 threads controladores relativas as portas do parque*/
  printf("Writing threads\n");
  pthread_t tN, tS, tE, tO;
  if (pthread_create(&tN, NULL, controlador_thread, "/tmp/fifoN")){//Norte
    printf("Error Creating Thread!\n");
    exit(4);
  }
  if (pthread_create(&tS, NULL, controlador_thread, "/tmp/fifoS")){//Sul
    printf("Error Creating Thread!\n");
    exit(4);
  }
  if (pthread_create(&tE, NULL, controlador_thread, "/tmp/fifoE")){//Este
    printf("Error Creating Thread!\n");
    exit(4);
  }
  if (pthread_create(&tO, NULL, controlador_thread, "/tmp/fifoO")){//Oeste
    printf("Error Creating Thread!\n");
    exit(4);
  }

/*Passagem do tempo de abertura do parque seguido do encerramento do mesmo*/
  unsigned int timeToSleep = t_abertura;
  while( ( timeToSleep = sleep(timeToSleep) ) > 0);
  pthread_mutex_lock(&mutex);//Secção Critica
  encerrou = 1;
  pthread_mutex_unlock(&mutex);

/*Envio de uma viatura indicativa de encerramento para cada um dos FIFOS
  relativos aos controladores*/
  int i;
  for( i = 0; i < 4; i++){
    char* msg = "/tmp/fifoO";

    switch (i) {
      case 0:
      msg = "/tmp/fifoN";
      break;
      case 1:
      msg = "/tmp/fifoS";
      break;
      case 2:
      msg = "/tmp/fifoE";
      break;
    }
    int fd;
    if((fd = open(msg,O_WRONLY)) == -1){//Opening FIFO with write only
      perror(msg);
      close(fd);
      continue;
    }
    Viatura temp;
    temp.portaEntrada = 'X';//Viatura que indica fecho do parque
    write(fd,&temp, sizeof(Viatura));
    close(fd);
  }

/*Terminio do programa  (esperar que as threads terminem)*/
  pthread_join(tN,NULL);
  pthread_join(tS,NULL);
  pthread_join(tE,NULL);
  pthread_join(tO,NULL);
  pthread_exit(NULL);
}
