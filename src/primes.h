#ifndef PRIMES_H
#define PRIMES_H

/**
 * Estrutura para coleção de alguns valores usados para estatísticas
 * irrelevantes.
 */
typedef struct {
	unsigned int nthreads;
	unsigned int nfilters;
	unsigned int nops;
} s_metrics;


/**
 * `thread` principal. Trata do primeiro número primo (2), prepara uma fila
 * circular e preenche-a com os valores pares para proceder à extração dos
 * restantes primos, procurados numa `thread` filtro. Assinala o semáforo global
 * que define o final da execução.
 * @param	arg Valor auxiliar, valor limite para procurar números primos.
 * @return		 Deve devolver um apontador nulo em caso de sucesso. O resultado
 *						 não é necessário.
 */
void *bootstrap_worker(void *);

/**
 * `thread` filtro que processa a fila circular recebida do `thread` que o criou
 * e procede à extração do valor primo ou à construção de uma nova fila circular
 * que é passada a um novo `thread` filtro numa tentativa recursiva assincrona
 * de aplicação da Peneira de Eratóstenes®
 * @param	arg Fila circular com os múltiplos de n encontrados pelo `thread`
 *						 criador.
 * @return		 NULL
 */
void *filter_worker(void *);

/**
 * Calcula o valor necessário de blocos na memória partilhada.
 * P.S.: a fórmula é simples (olhar para `return`).
 * @param	n Valor limite para procurar números primos.
 * @return	 A memória necessária a alocar.
 */
size_t ceiling(unsigned int);

/**
 * Comparador necessário para o uso do método `qsort`. Subtrai o valor `a` pelo
 * `b` e devolve o resultado, possibilitando uma ordenação crescente.
 * @param	a Primeiro valor.
 * @param	b Segundo valor.
 * @return	 Resultado a operação aritmética.
 */
int compare_handler(const void *, const void *);

/**
 * Imprime a ajuda para execução da aplicação.
 */
void help();

#endif