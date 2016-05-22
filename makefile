Projeto2: gerador parque

gerador: gerador.c viatura.h
	gcc gerador.c -Wall -lpthread -o gerador

parque: parque.c viatura.h
	gcc parque.c -Wall -lpthread -o parque
