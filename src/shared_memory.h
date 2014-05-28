#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct {
	key_t key;
	char *name;
	int id;
	unsigned int total;
	sem_t block;
	pthread_mutex_t mutex;
} p_shm_t;


/**
 * Prepara o apontador fornecido como primeiro argumento para alocação de espaço
 * em memória para partilhar entre `threads`.
 * @param	shm	Apontador a usar para o acesso à memória partilhada.
 * @param	name Nome do espaço reservado para acesso sobre a forma canónica
 *							(normalmente usado entre processos, o que não é o caso logo não
 *							está devidamente implementado).
 * @param	size Espaço total da memória partilhada
 * @return			0 em caso de sucesso, -1 na ocurrência de erro.
 */
int p_shm_init(p_shm_t **, char *, size_t);

/**
 * Escreve um valor no local apontado pelo primeiro espaço da memória partilhada
 * que funciona como índice de escrita. Controlado pelo semáforo `block` da
 * estrutura de controlo e pelo `mutex`, que permite acesso apenas a um `thread`
 * por cada vez. Incrementa o índice sempre que atualiza um valor e destroi o
 * acesso à memória por questões de segurança.
 * @param	shm Estrutura de controlo.
 * @param	n	 Valor a guardar na memória partilhada.
 * @return		 0 em caso de sucesso, -1 em caso de erro.
 */
int p_shm_push(p_shm_t *, unsigned int);

/**
 * Devolve o último valor escrito na memória partilhada apontada pelo índice
 * guardado na primeira posição do mesmo espaço. Diminui o índice guardando-o no
 * primeiro espaço da memória partilhada. Controlado por semáforos e `mutex` tal
 * como o processo de escrita. Destroi o acesso à memória no final da operação.
 * @param	shm Estrutura de controlo.
 * @return		 O valor ocupado na última posição da memória partilhada ou -1 em
 *						 caso de erro.
 */
int p_shm_pop(p_shm_t *);

/**
 * Método auxiliar para obtenção de um valor num dado índice da memória
 * partilhada.
 * !!!NÃO IMPLEMENTADO!!!
 * @param	shm Apontador para o controlo de memória.
 * @param	 n	 Valor do índice.
 * @return		 0 em caso de sucesso, -1 em caso de erro.
 */
int p_shm_get(p_shm_t *, unsigned int);

/**
 * Destroi o conteúdo da memória partilhada e liberta o espaço para o sistema
 * operativo.
 * @param	shm Estrutura de controlo.
 * @return		 0 em caso de sucesso, -1 em caso de erro.
 */
int p_shm_destroy(p_shm_t *);

#endif
