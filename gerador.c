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
#include <sys/times.h>
#include <fcntl.h>
#include <semaphore.h>

#include "viatura.h"

#define DIRECTORY_LENGTH 4096
#define FILE_LENGTH 255
#define BILLION 1000000000

int viatura_ID = 1;
sem_t * semN = SEM_FAILED;
sem_t * semS = SEM_FAILED;
sem_t * semO = SEM_FAILED;
sem_t * semE = SEM_FAILED;

int fileLog = 0;
clock_t clockInicial;


void mySleep(int ticks){//Recebe os ticks para dormir
  double myS = ticks / (double)sysconf(_SC_CLK_TCK);
  struct timespec * req, * rem;
  req = malloc(sizeof(struct timespec));
  rem = malloc(sizeof(struct timespec));
  req->tv_sec = myS / 1;
  req->tv_nsec = (long)((myS - req->tv_sec) * BILLION);

  int nr;
  while((nr = nanosleep(req,rem)) != 0){
    nr = nanosleep(rem,req);
    if(nr == 0)
      break;
  }

  free(req);
  free(rem);
}

void sigPipe(int id){
  printf("SIGPIPE!");
}

void closeFifo(char * dir, int fd2){
  close(fd2);
  unlink(dir);
}

void debug(unsigned int tempo, int numViatura, char entrada, unsigned int tempoEstacionamento, unsigned int tempoVida, char* tag){
  char text[DIRECTORY_LENGTH + FILE_LENGTH];

  if(tempoVida != -1)
  sprintf(text, "%10d ; %7d ;   %c    ; %10d ; %6d ; %s\n" , tempo , numViatura , entrada, tempoEstacionamento,tempoVida, tag);
  else
  sprintf(text, "%10d ; %7d ;   %c    ; %10d ;    ?   ; %s\n" , tempo , numViatura , entrada, tempoEstacionamento, tag);

  write(fileLog, text , strlen(text) );

}

sem_t* abrirSemaforo(char c){

  sem_t ** tmp;
  char * path;

  switch (c) {
    case 'N':
      path = "/semaforoN";
      tmp = &semN;
      break;
    case 'S':
      path = "/semaforoS";
      tmp = &semS;
      break;
    case 'O':
      path = "/semaforoO";
      tmp = &semO;
      break;
    case 'E':
      path = "/semaforoE";
      tmp = &semE;
      break;
      default:
        printf("Error opening semaphore: %c\n",c);
        return NULL;
  }

  if((*tmp) == SEM_FAILED)
    if( ((*tmp) = sem_open(path, 0, S_IRWXU, 1)) == SEM_FAILED){
      return SEM_FAILED;
    }

    return (*tmp);
}

void * viatura_thread(void * arg){

  clock_t tInicial = times(NULL);
  sem_t * sem;

  Viatura* viatura = (Viatura*)arg;

  char fifoName[DIRECTORY_LENGTH + FILE_LENGTH] ;
  char fifoViatura[DIRECTORY_LENGTH + FILE_LENGTH] ;

  sprintf(fifoViatura, "/tmp/viatura%d", viatura->numeroID);

  if( mkfifo(fifoViatura, S_IRWXU) != 0){
    perror(fifoViatura);
    free(viatura);
    exit(5);
  }



  sprintf(fifoName, "/tmp/fifo%c", viatura->portaEntrada);

  sem = abrirSemaforo(viatura->portaEntrada);

  if(sem == SEM_FAILED){
    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "");
    unlink(fifoViatura);
    free(viatura);
    return NULL;
  }

  //printf("A espera de Entrar\n");


  sem_wait(sem);
  //printf("Dentro da seccao critica\n");

  int fifoDestino = 0;
  if( (fifoDestino = open(fifoName, O_WRONLY | O_NONBLOCK)) == -1){ //Abrir FifoControlador
    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "");
    //printf("SeccaoCritica, nao consigo abrir!\n");

    unlink(fifoViatura);
    close(fifoDestino);
    sem_post(sem);
    free(viatura);
    //perror(fifoName);
    return NULL;
  }
  //printf("Escrever Viatura: %d\n", viatura->numeroID);
  if( write( fifoDestino, viatura, sizeof(Viatura) ) == -1 ){ //Escrever para o Fifo Controlador
    printf("Error Writing to FIFO Dest\n");
    free(viatura);
    unlink(fifoViatura);
    close(fifoDestino);
    sem_post(sem);
    exit(6);
  }
  close(fifoDestino);

  sem_post(sem);

  //printf("Entrou!\n");

  sprintf(fifoName, "/tmp/viatura%d", viatura->numeroID);

  int fifoOrigem = 0;
  if( (fifoOrigem = open(fifoName, O_RDONLY)) == -1 ){ //Abrir Fifo leitura
    perror(fifoName);
    free(viatura);
    closeFifo(fifoViatura,fifoOrigem);
    exit(7);
  }


  //printf("Esperando para leitura!\n");

  char info;
  int res = 0;
  if((res = read(fifoOrigem, &info, sizeof(char) )) == -1){
    printf("Error Reading fifo!");
    free(viatura);
    closeFifo(fifoViatura,fifoOrigem);
    exit(8);
  }


  if(info == RES_ENTRADA){
    //printf("Entrou!\n");

    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "entrada");

    if( (res = read(fifoOrigem, &info, sizeof(char) )) == -1){
      printf("Error Reading fifo!");
      free(viatura);
      closeFifo(fifoViatura,fifoOrigem);
      exit(9);
    }

  }else if(info == RES_CHEIO){
    //printf("Estava Cheio!\n");

    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "cheio!");


  }else if(info == RES_ENCERRADO){
    //printf("Estava Encerrado!\n");

    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento, -1 , "");
  }

  if(info == RES_SAIDA){
    //printf("Acabei de SAIR!!\n");

    debug((int)(times(NULL) - clockInicial) , viatura->numeroID , viatura->portaEntrada, viatura->tempoEstacionamento,(int)(times(NULL) - tInicial), "saida");
  }

  free(viatura);
  closeFifo(fifoViatura,fifoOrigem);
  return NULL;
}



int main(int argn, char *argv[]){



  printf("%lu\n", sysconf(_SC_CLK_TCK));
  if(argn != 3){
    printf("Error <Usage>: %s <T_GERACAO> <U_RELOGIO>\n",  argv[0]);
    return 1;
  }

  int u_relogio = 10;

  int t_geracao = atoi(argv[1]);
  u_relogio = atoi(argv[2]);

  if(t_geracao <= 0){
    printf("Error <T_GERACAO> must be greater than 0\n");
    return 1;
  }
  if(u_relogio <= 0){
    printf("Error <U_RELOGIO> must be greater than 0\n");
    return 1;
  }

  char path[DIRECTORY_LENGTH + FILE_LENGTH];
  realpath(".", path);
  sprintf(path, "%s/%s", path, "gerador.log");

  if( (fileLog = open(path, O_CREAT | O_WRONLY | O_TRUNC , S_IRWXU)) == -1){
    perror(path);
    exit(3);
  }

  clockInicial = times(NULL);

  char string[DIRECTORY_LENGTH];
  sprintf(string, "Tick inicial : %lu\n", clockInicial);
  write(fileLog, string, strlen(string));
  write(fileLog, " t(ticks)  ; id_viat ; destin ; t_estacion ; t_vida ; observ\n" ,62);



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

    local = rand() % 10;

    if(local < 5){
      local = 0;
    } else if(local < 8){
      local = 1;
    } else {
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
    mySleep(wastingTime);
    /*CODIGO ANTIGO
    clock_t init = clock();
    while( 0 <  wastingTime ){
      wastingTime -= clock() - init;
      init = clock();
    }
    */
    totalTime = (time(NULL) - segundosIniciais);
    printf("%d\n",(int)totalTime);
  }while( totalTime < t_geracao);

  pthread_exit(NULL);
}
