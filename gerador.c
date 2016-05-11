/** Version: 1.0
  * Author: Grupo T1G09
  *
*/

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "viatura.h"

unsigned int u_relogio = 10;
int viatura_ID = 1;

void * viatura_thread(void * arg){
  Viatura* viatura = (Viatura*)arg;
  return viatura ;
}


int main(int argn, char *argv[]){

  if(argn != 3){
    printf("Error <Usage>: %s <T_GERACAO> <U_RELOGIO>\n",  argv[0]);
    return 1;
  }

  //clock_t c_inicio = clock();

  //int t_geracao = atoi(argv[1]);

  //u_relogio = atoi(argv[2]);

  int local = rand()%4;

  Viatura* v = (Viatura*)malloc(sizeof(Viatura));

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

  v->tempoEstacionamento;



  return 0;
}
