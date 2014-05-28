#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "shared_memory.h"


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
int p_shm_init(p_shm_t **shm, char *name, size_t size) {
	*shm = (p_shm_t *) malloc(sizeof(p_shm_t));

	(*shm)->name = (char *) malloc(sizeof(char) * strlen(name));
	memcpy((*shm)->name, name, strlen(name));

	(*shm)->id = shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
	if ((*shm)->id == -1) {
		perror("p_shm_init::shmget");
		return -1;
	}

	if (sem_init(&((*shm)->block), 0, 1) == -1) {
		perror("Trying to init `shm::block` semaphore.");
		return -1;
	}

	if (pthread_mutex_init(&((*shm)->mutex), NULL) != 0) {
		perror("Trying to init `shm::mutex`.");
		return -1;
	}

	(*shm)->total = 0;

	return 0;
}

/**
 * Método auxiliar para obtenção da chave de acesso à memória partilhada pela
 * forma canónica (nome).
 * !!!NÃO IMPLEMENTADO!!!
 * @param	shm Apontador para o controlo de memória.
 * @return		 0 em caso de sucesso, -1 em caso de erro.
 *
int p_shm_get_key(p_shm_t *shm) {
	shm->key = ftok((*shm)->name, 0);
	if ((*shm)->key == -1) {
		perror("p_shm_init::ftok");
		return -1;
	}
	return 0;
}
*/

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
int p_shm_push(p_shm_t *shm, unsigned int n) {
	unsigned int index = 0, *md;

	pthread_mutex_lock(&shm->mutex);

	md = (unsigned int *) shmat(shm->id, 0, 0);
	if (md == (unsigned int *)(-1)) {
		perror("p_shm_init::shmat");
		return -1;
	}

	sem_wait(&shm->block);

	index = md[0];
	md[++index] = n;
	md[0] = index;

	if (shmdt(md) == -1) {
		perror("Detaching shared memory");
		return -1;
	}

	shm->total++;

	sem_post(&shm->block);
	pthread_mutex_unlock(&shm->mutex);

	return 0;
}

/**
 * Devolve o último valor escrito na memória partilhada apontada pelo índice
 * guardado na primeira posição do mesmo espaço. Diminui o índice guardando-o no
 * primeiro espaço da memória partilhada. Controlado por semáforos e `mutex` tal
 * como o processo de escrita. Destroi o acesso à memória no final da operação.
 * @param	shm Estrutura de controlo.
 * @return		 O valor ocupado na última posição da memória partilhada ou -1 em
 *						 caso de erro.
 */
int p_shm_pop(p_shm_t *shm) {
	unsigned int index = 0, value, *md;

	pthread_mutex_lock(&shm->mutex);
	
	md = (unsigned int *) shmat(shm->id, 0, 0);
	if (md == (unsigned int *)(-1)) {
		perror("p_shm_init::shmat");
		return -1;
	}

	sem_wait(&shm->block);
	
	index = md[0];	
	value = md[index--];
	md[0] = index;
	
	if (shmdt(md) == -1) {
		perror("Detaching shared memory");
		return -1;
	}
	
	sem_post(&shm->block);
	pthread_mutex_unlock(&shm->mutex);

	return value;
}

/**
 * Destroi o conteúdo da memória partilhada e liberta o espaço para o sistema
 * operativo.
 * @param	shm Estrutura de controlo.
 * @return		 0 em caso de sucesso, -1 em caso de erro.
 */
int p_shm_destroy(p_shm_t *shm) {
	if (shmctl(shm->id, IPC_RMID, NULL) == -1) {
		perror("Releasing shared memory");
		return -1;
	}

	return 0;
}
