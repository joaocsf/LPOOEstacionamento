/** Version: 1.0
  * Author: Grupo T1G09
  *
  */

#define RES_ENTRADA 0
#define RES_SAIDA 1
#define RES_CHEIO 2
#define RES_ENCERRADO 3

typedef struct{
  char portaEntrada;
  int tempoEstacionamento;
  int numeroID;
  int fifoID;
} Viatura;
