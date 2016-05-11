/** Version: 1.0
  * Author: Grupo T1G09
  *
  */
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int N_TOTAL_LUGARES;
int LUGARES_OCUPADOS = 0;
int T_ABERTURA;

void * controlador_thread(void * args){
  return NULL;
}

void * arrumador_thread(void * args){
  return NULL;
}

int main(int argc, char *argv[]){

  if(argc != 3){
    printf("Error <Usage> : %s <N_LUGARES> <T_ABERTURA>\n",argv[0]);
    return 1;
  }

  N_TOTAL_LUGARES = atoi(argv[1]);
  T_ABERTURA = atoi(argv[2]);

  return 0;
}
