#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "circular_queue.h"

/**
 * Inicia a fila circular, alocando espaço para o apontador fornecido como
 * primeiro argumento, limitando a fila à capacidade oferecida como segundo
 * argumento.
 * @param	q				Apontador que identifica a fila circular a criar.
 * @param	capacity Capacidade da fila circular.
 * @return					0 em caso de sucesso, -1 em caso de erro.
 */
int queue_init(CircularQueue **q, unsigned int capacity) {
	*q = (CircularQueue *) malloc(sizeof(CircularQueue));
	
	if (sem_init(&((*q)->empty), 0, capacity) == -1) {
		perror("Trying to init `empty queue` semaphore.");
		return -1;
	}

	if (sem_init(&((*q)->full), 0, 0) == -1) {
		perror("Trying to init `full queue` semaphore.");
		return -1;
	}
	
	errno = pthread_mutex_init(&((*q)->mutex), NULL);
	if (errno != 0) {
		perror("Trying to init `queue mutex`.");
		return -1;
	}

	(*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem));
	(*q)->capacity = capacity + 1; // If a zero needs to be added
	(*q)->first = 0;
	(*q)->last = 0;

	return 0;
}

/**
 * Coloca um valor na fila circular, atualizando as posições dos seus índices.
 * @param	q		 Apontador para a fila circular.
 * @param	value Valor a colocar na fila circular.
 * @return			 0 em caso de sucesso, -1 em caso de erro.
 */
int queue_put(CircularQueue *q, QueueElem value) {

	sem_wait(&q->empty);
	pthread_mutex_lock(&q->mutex);
	
	q->v[q->last] = value;

	// it must check for overbound problems (thanks wikipedia)
	q->last = (q->last + 1) % q->capacity;

	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->full);

	return 0;
}

/**
 * Devolve o valor apontado como primeiro valor da fila circular `q`.
 * @param	q Apontador para a fila circular.
 * @return	 O valor reservado no ínicio da fila ou -1 em caso de erro.
 */
QueueElem queue_get(CircularQueue *q) {
	QueueElem value;

	sem_wait(&q->full);
	pthread_mutex_lock(&q->mutex);
	
	value = q->v[q->first];
	q->v[q->first] = 0;

	// it must check for overbound problems (thanks wikipedia)
	q->first = (q->first + 1) % q->capacity;

	pthread_mutex_unlock(&q->mutex);
	sem_post(&q->empty);

	return value;
}

/**
 * Destroi a fila circular, libertando o espaço em memória reservado para o seu
 * uso.
 * @param	q Apontador para a fila circular.
 * @return	 0 em caso de sucesso, -1 em caso de erro.
 */
int queue_destroy(CircularQueue *q) {
	if (pthread_mutex_destroy(&q->mutex) == -1) {
		perror("Destroying queue's mutex.");
		return -1;
	}
	
	if (sem_destroy(&q->empty) == -1) {
		perror("Destroying queue's empty semaphore.");
		return -1;
	}

	if (sem_destroy(&q->full) == -1) {
		perror("Destroying queue's full semaphore.");
		return -1;
	}

	free(q->v);

	return 0;
}

/**
 * Método auxiliar para verificar se a fila circular está cheia sem bloquear a
 * execução da aplicação (ou `thread`).
 * !!!NÃO FOI TESTADO!!! este método não é usado na aplicação e pode funcionar
 * de forma inesperada.
 * @param	q Apontador para a fila circular.
 * @return	 1 caso a fila esteja vazia, 0 caso não esteja vazia, -1 em caso de
 *					 erro.
 */
int queue_is_empty(CircularQueue *q) {
	int empty = 0, full = 0, status = 0;

	pthread_mutex_lock(&q->mutex);
	if (sem_getvalue(&(q->empty), &empty) == -1) {
		perror("queue_is_empty: checking queue's empty semaphore.");
		pthread_mutex_unlock(&q->mutex);		
		return -1;
	}
	if (sem_getvalue(&(q->full), &full) == -1) {
		perror("queue_is_empty: checking queue's full semaphore.");
		pthread_mutex_unlock(&q->mutex);		
		return -1;
	}
	
	status = (empty == q->capacity && full == 0);
	pthread_mutex_unlock(&q->mutex);

	return status;
}
