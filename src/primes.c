#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include "shared_memory.h"
#include "circular_queue.h"
#include "primes.h"

/**
 * Estrutura auxiliar usada para algumas metricas nada importantes
 */
static s_metrics *metrics;

/**
 * Estrutura que controla o acesso à memória partilhada
 */
static p_shm_t *shmd;

/**
 * Limite para encontrar os números primos contidos no intervalo delimitado por
 * 0 e o próprio (é sempre posítivo e espera-se que seja superior ou igual a 2).
 */
static int nproblem;

/**
 * Semáforo que controla a duração da aplicação.
 */
sem_t work;

/**
 * Imprime a ajuda para execução da aplicação.
 */
void help() {
	printf(
		"\bPrimes\n" \
		"A multi-threaded implementation of the Sieve of Eratosthenes.\n" \
		"This implementation applies the producer-consumer pattern into a the\n" \
		"prime solving algorithm.\n\n" \
		"Usage:\n prime <n>\t\t" \
		"Where <n> is a natural number from which all\n" \
		"Returns:\n" \
		" All primes are extracted and sorted.\n"
	);
}

/**
 * Calcula o valor necessário de blocos na memória partilhada.
 * P.S.: a fórmula é simples (olhar para `return`).
 * @param	n Valor limite para procurar números primos.
 * @return	 A memória necessária a alocar.
 */
size_t ceiling(unsigned int n) {
	float x = 1.2;
	return round(x * (n * logl(n)));
}

/**
 * Comparador necessário para o uso do método `qsort`. Subtrai o valor `a` pelo
 * `b` e devolve o resultado, possibilitando uma ordenação crescente.
 * @param	a Primeiro valor.
 * @param	b Segundo valor.
 * @return	 Resultado a operação aritmética.
 */
int compare_handler(const void *a, const void *b) {
	return (*(int *) a - *(int *) b);
}

/**
 * `thread` principal. Trata do primeiro número primo (2), prepara uma fila
 * circular e preenche-a com os valores pares para proceder à extração dos
 * restantes primos, procurados numa `thread` filtro. Assinala o semáforo global
 * que define o final da execução.
 * @param	arg Valor auxiliar, valor limite para procurar números primos.
 * @return		 Deve devolver um apontador nulo em caso de sucesso. O resultado
 *						 não é necessário.
 */
void *bootstrap_worker(void *arg) {
	unsigned int i = 2;
	QueueElem x;
	CircularQueue *queue;
	x = *((QueueElem*) arg);
	pthread_t t_filter;

	if (x > 1) {
		p_shm_push(shmd, i++);
	}

	if (x == 2) {
		sem_post(&work);
		return NULL;
	}

	if (queue_init(&queue, CAPACITY) != 0) {
		perror("queue_init");
		sem_post(&work);
		return (int *) -1;
	}

	metrics->nthreads++;
	metrics->nfilters++;
	pthread_create(&t_filter, NULL, filter_worker, queue);

	for (; i <= x; i += 2) {
		if (queue_put(queue, i) == -1) {
			perror("push_worker: queue_put");
			queue_destroy(queue);
			sem_post(&work);
			return NULL;
		}
	}
	queue_put(queue, 0);

	pthread_join(t_filter, NULL);

	if (queue_destroy(queue) == -1) {
		sem_post(&work);
		return (int *) -1;
	}

	sem_post(&work);
	return NULL;
}

/**
 * `thread` filtro que processa a fila circular recebida do `thread` que o criou
 * e procede à extração do valor primo ou à construção de uma nova fila circular
 * que é passada a um novo `thread` filtro numa tentativa recursiva assincrona
 * de aplicação da Peneira de Eratóstenes®
 * @param	arg Fila circular com os múltiplos de n encontrados pelo `thread`
 *						 criador.
 * @return		 NULL
 */
void *filter_worker(void *arg) {
	QueueElem n = 0;
	CircularQueue *queue, *new_queue;
	queue = (CircularQueue *) arg;
	pthread_t filter;
	unsigned int i = 0;

	n = queue_get(queue);
	if (n > (int) sqrt(nproblem)) {
		do {
			p_shm_push(shmd, n);
			n = queue_get(queue);
		} while (n > 0);
	} else {
		if (queue_init(&new_queue, CAPACITY) != 0) {
			perror("queue_init");
			return (int *) -1;
		}

		metrics->nthreads++;
		metrics->nfilters++;

		pthread_create(&filter, NULL, filter_worker, (void *) new_queue);
		p_shm_push(shmd, n);

		for (i = 2 * n; i <= nproblem && i != 0; i = queue_get(queue)) {
			if (i % n != 0) {
				queue_put(new_queue, i);
			}
		}
		queue_put(new_queue, 0);

		pthread_join(filter, NULL);

		if (queue_destroy(new_queue) == -1) {
			return (int *) -1;
		}
	}

	return NULL;
}

/**
 * Thread principal. Arranca a aplicação.,
 * @param	argc Número de argumentos
 * @param	argv Conteúdo dos argumentos
 * @return			0
 */
int main(int argc, char *argv[]) {
	int i = 0;
	unsigned int value, *result;
	size_t limit;
	pthread_t bootstrap_w;

	if (argc != 2) {
		printf("Wrong number of arguments.\n");
		help();
		exit(0);
	}

	sscanf(argv[1], "%d", &nproblem);
	if (nproblem <= 1) {
		printf("C'mon man! Primes from %d???\n", nproblem);
		exit(0);
	}

	metrics = malloc(sizeof(s_metrics));
	metrics->nthreads = 0;
	metrics->nfilters = 0;
	metrics->nops = 0;

	limit = ceiling((unsigned int ) nproblem);
	printf("Prime find:\n N = %d\n Ceiling: %zu\n", nproblem, limit);

	if (p_shm_init(&shmd, "primes_block", limit) == -1) {
		perror("Too much memory needed. Bailing");
		exit(0);
	}

	if (sem_init(&work, 0, 0) == -1) {
		perror("Trying to init `full queue` semaphore.");
		return -1;
	}

	metrics->nthreads++;
	pthread_create(&bootstrap_w, NULL, bootstrap_worker, &nproblem);

	printf("Processing....\n");
	sem_wait(&work);
	printf("BAM!\n");

	pthread_join(bootstrap_w, NULL);

	result = malloc(sizeof(unsigned int) * shmd->total);
	for (value = p_shm_pop(shmd); value != 0; i++, value = p_shm_pop(shmd)) {
		result[i] = value;
	}
	qsort(result, shmd->total, sizeof(unsigned int), compare_handler);

	printf("\nResult:\n");
	for (value = i, i = 0; i < value; i++)
		printf("%d ", result[i]);
	printf("\n");

	if (shmd->total == 1) {
		printf("\n Found %d prime number.\n", shmd->total);
	} else {
		printf("\n Found %d prime numbers.\n", shmd->total);
	}

	printf(
		" Threads created: %d\tfilters: %d\n",
		metrics->nthreads, metrics->nfilters
	);

	if (p_shm_destroy(shmd) == -1) {
		exit(0);
	}

	free(metrics);
	free(result);
	
	return 0;
}
