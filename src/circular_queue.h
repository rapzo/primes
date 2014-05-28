#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_h

#include <semaphore.h>
#include <pthread.h>

#define CAPACITY 10

// Type of the circular queue elements
typedef unsigned long QueueElem;

// Struct for representing a "circular queue"
// Space for the queue elements will be allocated dinamically by queue_init()
typedef struct {
	QueueElem *v; // pointer to the queue buffer
	
	unsigned int capacity; // queue capacity
	unsigned int first; // head of the queue
	unsigned int last; // tail of the queue
	
	sem_t empty; // semaphores and mutex for implementing the
	sem_t full; // producer-consumer paradigm
	pthread_mutex_t mutex;
} CircularQueue;


/**
 * Inicia a fila circular, alocando espaço para o apontador fornecido como
 * primeiro argumento, limitando a fila à capacidade oferecida como segundo
 * argumento.
 * @param	q				Apontador que identifica a fila circular a criar.
 * @param	capacity Capacidade da fila circular.
 * @return					0 em caso de sucesso, -1 em caso de erro.
 */
int queue_init(CircularQueue **q, unsigned int capacity);

/**
 * Coloca um valor na fila circular, atualizando as posições dos seus índices.
 * @param	q		 Apontador para a fila circular.
 * @param	value Valor a colocar na fila circular.
 * @return			 0 em caso de sucesso, -1 em caso de erro.
 */
int queue_put(CircularQueue *q, QueueElem value);

/**
 * Devolve o valor apontado como primeiro valor da fila circular `q`.
 * @param	q Apontador para a fila circular.
 * @return	 O valor reservado no ínicio da fila ou -1 em caso de erro.
 */
QueueElem queue_get(CircularQueue *q);

/**
 * Destroi a fila circular, libertando o espaço em memória reservado para o seu
 * uso.
 * @param	q Apontador para a fila circular.
 * @return	 0 em caso de sucesso, -1 em caso de erro.
 */
int queue_destroy(CircularQueue *q);

/**
 * Método auxiliar para verificar se a fila circular está cheia sem bloquear a
 * execução da aplicação (ou `thread`).
 * !!!NÃO FOI TESTADO!!! este método não é usado na aplicação e pode funcionar
 * de forma inesperada.
 * @param	q Apontador para a fila circular.
 * @return	 1 caso a fila esteja vazia, 0 caso não esteja vazia, -1 em caso de
 *					 erro.
 */
int queue_is_empty(CircularQueue *q);

#endif
