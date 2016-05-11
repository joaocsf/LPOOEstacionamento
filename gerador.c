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

int viatura_ID = 1;

int fileLog = 0;
clock_t clockInicial;
/**
*
*
*/
void debug(unsigned int tempo, int numViatura, char entrada, unsigned int tempoEstacionamento, unsigned int tempoVida, char* tag){
  char text[DIRECTORY_LENGTH + FILE_LENGTH];

  if(tempoVida != -1)
  sprintf(text, "%10d ; %7d ;   %c    ; %10d ; %6d ; %s\n" , tempo , numViatura , entrada, tempoEstacionamento,tempoVida, tag);
  else
  sprintf(text, "%10d ; %7d ;   %c    ; %10d ;    ?   ; %s\n" , tempo , numViatura , entrada, tempoEstacionamento, tag);

  write(fileLog, text , strlen(text) );

}

void * viatura_thread(void * arg){

  clock_t tInicial = clock();

  Viatura* viatura = (Viatura*)arg;

  char fifoName[DIRECTORY_LENGTH + FILE_LENGTH] ;
  char fifoViatura[DIRECTORY_LENGTH + FILE_LENGTH] ;

  sprintf(fifoViatura, "/tmp/viatura%d", viatura->numeroID);

  if( mkfifo(fifoViatura, S_IRWXU) != 0){
    perror(fifoViatura);
    exit(5);
  }

  sprintf(fifoName, "/tmp/fifo%c", viatura->portaEntrada);

  int fifoDestino = 0;
  if( (fifoDestino = open(fifoName, O_WRONLY | O_NONBLOCK)) == -1){

    debug((int)(clock() - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "cheio!");
    closeFifo(fifoViatura);
    perror(fifoName);
    return NULL;
  }

  if( write( fifoDestino, viatura, sizeof(Viatura) ) == -1 ){
    printf("Error Writing to FIFO Dest\n");

    closeFifo(fifoViatura);
    exit(6);
  }

  sprintf(fifoName, "/tmp/viatura%d", viatura->numeroID);


  int fifoOrigem = 0;
  if( (fifoOrigem = open(fifoName, O_RDONLY | O_CREAT)) == -1 ){
    perror(fifoName);

    closeFifo(fifoViatura);
    exit(7);
  }

  char info;
  int res = 0;
  while ( (res = read(fifoOrigem, &info, sizeof(char) )) == 0);
  if(res == -1){
    printf("Error Reading fifo!");

    closeFifo(fifoViatura);
    exit(8);
  }


  if(info == RES_ENTRADA){


    debug((int)(clock() - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "entrada");

    while ( (res = read(fifoOrigem, &info, sizeof(char) )) == 0);
    if(res == -1){
      printf("Error Reading fifo!");

      closeFifo(fifoViatura);
      exit(9);
    }

  }else if(info == RES_CHEIO){
    debug((int)(clock() - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "cheio!");


  }else if(info == RES_ENCERRADO){
    debug((int)(clock() - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "");
  }

  if(info == RES_SAIDA){
    debug((int)(clock() - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento,(int)(clock() - tInicial), "saida");
  }

  closeFifo(fifoViatura);
  return NULL;
}


void closeFifo(char * dir){
  close(dir);
  unlink(dir);
}

int main(int argn, char *argv[]){

  if(argn != 3){
    printf("Error <Usage>: %s <T_GERACAO> <U_RELOGIO>\n",  argv[0]);
    return 1;
  }
  char path[DIRECTORY_LENGTH + FILE_LENGTH];
  realpath(".", path);
  sprintf(path, "%s/%s", path, "gerador.log");

  if( (fileLog = open(path, O_CREAT | O_WRONLY | O_TRUNC , S_IRWXU)) == -1){
    perror(path);
    exit(3);
  }

  write(fileLog, " t(ticks)  ; id_viat ; destin ; t_estacion ; t_vida ; observ\n" ,62);



  unsigned int u_relogio = 10;

  int t_geracao = atoi(argv[1]);
  u_relogio = atoi(argv[2]);

  clockInicial = clock();

  time_t segundosIniciais = time(NULL);
  time_t totalTime = 0;



  do{

    Viatura* v = (Viatura*)malloc(sizeof(Viatura));

    int local = rand()%4;

    switch (local) {
      case 0:
        v->portaEntrada = 'N';
      break;
      case 1:
        v->portaEntrada = 'S';
      break;
      case 2:
        v->portaEntrada = 'E';
      break;
      case 3:
        v->portaEntrada = 'O';
      break;
    }
    v->numeroID = viatura_ID++;
    v->fifoID = v->numeroID;

    local = rand() % 100;

    if(local < 50){
      local = 0;
    }else if(local < 80){
      local = 1;
    }else if(local < 100){
      local = 2;
    }

    v->tempoEstacionamento = (rand()%10 + 1 )* u_relogio;
    pthread_t tid;
    printf("Viatura Criada: %10d ;   %c   ; %10d\n", v->numeroID , v->portaEntrada , (int)v->tempoEstacionamento);

    if(pthread_create(&tid, NULL , viatura_thread , v)){
      printf("Error Creating Thread!\n");
      exit(2);
    }

    pthread_detach(tid);
    clock_t wastingTime = local * u_relogio;
    clock_t init = clock();
    while( 0 <  wastingTime ){
      wastingTime -= clock() - init;
      init = clock();
    }

    totalTime = (time(NULL) - segundosIniciais);
    printf("%d\n",totalTime);
  }while( totalTime < t_geracao);

  close(fileLog);

  return 0;
}
