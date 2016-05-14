/** Version: 1.0
  * Author: Grupo T1G09
  *
  */

/*
 COMENTARIO DO JOAO:
 -um problema que temos e que como estas em read() mesmo que executes o parque sem receber um unico vehiculo
 o parque nao chega a fechar fica sempre aberto sem veiculos la dentro, uma ideia que tive seria usar
 NON_BLOCK para ler os vehiculos que chegassem mas isso deve ser dispendioso porque vai fazer busy waiting...
 David - O stores disseram para evitarmos ao maximo os casos de busy waiting :/

 -Outra Cena: acho que nao podemos fechar o FIFO como tinha dito quando o tempo passa porque se nao rip
 aos logs do tipo: encerrado.

 -ainda falta fazer testes ao resto mas ate agora parece fazer sentido.


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
clock_t tempoInicial;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
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

  if((nr = mkfifo((char *)args, 0644)) == -1){//Creating FIFO
    perror((char *)args);
    return NULL;
  }
  printf("A\n");
  if((fd = open((char *)args,O_RDONLY)) == -1){//Opening FIFO with read only
    perror((char *)args);
    return NULL;
  }
  printf("B\n");
  if((fd_dummy = open((char *)args,O_WRONLY)) == -1){//Opening FIFO with write only
    perror((char *)args);
    close(fd);
    unlink((char *)args);
    return NULL;
  }
  printf("C\n");

  Viatura* viaturaTemp;
  while( (viaturaTemp = lerViatura(fd)) !=NULL){
    if( viaturaTemp->portaEntrada == 'X'){ //Se Terminou
      printf("D-Closing\n");
      close(fd_dummy);
      continue;
    }
    printf("Looping\n");
    printf("Entrou!: %c\n", viaturaTemp->portaEntrada);
      //Viatura* viaturaTemp = lerViatura(fd);
      //Ler viaturas
    continue;
    if(pthread_create(&tid, NULL , arrumador_thread , viaturaTemp)){
      printf("Error Creating Thread!\n");
      close(fd);
      close(fd_dummy);
      unlink((char *)args);
      return NULL;
    }
    pthread_detach(tid);
  }
    //Ler continuamente as viaturas que vao chegando
    printf("CLOSED\n");
  //Fechou
  //Ler as restantes viaturas;

  close(fd);
  //close(fd_dummy);

  if((nr = unlink((char *)args)) == -1){//Deleting FIFO
    perror((char *)args);
    unlink((char *)args);
    return NULL;
  }

  unlink((char *)args);
  return NULL;
}

Viatura* lerViatura(int fd){
  Viatura* v = (Viatura *)malloc(sizeof(Viatura *));
  int returnValue = read(fd,v,sizeof(Viatura));
  if( returnValue == -1 || returnValue == 0) //Caso nao consiga ler viaturas
    return NULL;
  else
    return v;
}

void * arrumador_thread(void * args){

  char resposta = 0;
  Viatura * v = (Viatura *)args;

//Criar FIFO
  char fifoViatura[DIRECTORY_LENGTH + FILE_LENGTH] ;
  sprintf(fifoViatura, "/tmp/viatura%d", v->numeroID);
  int fdViatura = 0;
  if( (fdViatura = open(fifoViatura, O_RDONLY) ) == -1){
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
  clock_t tickInicial = clock();
  while(clock() - tickInicial < v->tempoEstacionamento);

  //Saida
  resposta = RES_SAIDA;
  write(fdViatura, &resposta, sizeof(char) );

  pthread_mutex_lock(&mutex);//Seccção Critica

  debugLog(clock() - tempoInicial , n_total_lugares -  lugares_ocupados , v->numeroID, "saida");
  lugares_ocupados--;


  pthread_mutex_unlock(&mutex);

  close(fdViatura);
  free(v);
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

  tempoInicial = clock();

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

  unsigned int timeToSleep = t_abertura;
  while( ( timeToSleep = sleep(timeToSleep) ) > 0);
  pthread_mutex_lock(&mutex);//Secção Critica
  encerrou = 1;
  pthread_mutex_unlock(&mutex);
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
      unlink(msg);
      continue;
    }

    Viatura temp;
    temp.portaEntrada = 'X';
    write(fd,&temp, sizeof(Viatura));
    close(fd);
  }

  pthread_join(tN,NULL);
  pthread_join(tS,NULL);
  pthread_join(tE,NULL);
  pthread_join(tO,NULL);

  close(fileLog);
  return 0;
}
