/*

Nome: Ramon Pelle

GRR: 20244408

*/

#include "queue.h"
#include <stdio.h>

/**
* @params: queue - ponteiro para a fila circular duplamente ligada
* Conta o número de elementos na fila percorrendo todos os nós até retornar ao início
* @return: número de elementos na fila, ou 0 se a fila for NULL
*/
int queue_size(queue_t *queue){
    if (queue == NULL)
        return 0;
    
    int count = 0;
    queue_t *atual = queue;

    do {
        count++;
        atual = atual->next;
    } while (atual != queue); 

    return count;
}

/**
* @params: name - string com o nome da fila para impressão
*          queue - ponteiro para a fila circular duplamente ligada
*          print_elem - função callback para imprimir cada elemento da fila
* Imprime todos os elementos da fila no formato [elem1 elem2 elem3]
* @return: void
*/
void queue_print(char *name, queue_t *queue, void print_elem (void*)){
    if (print_elem == NULL) {
        fprintf(stderr, "ERRO NO PRINT: Função de impressão inválida.\n");
        return;
    }

    printf("%s", name);

    if (queue == NULL) {
        printf("[]\n");
        return;
    }

    queue_t *atual = queue;
    printf("[");
    
    print_elem(atual);
    atual = atual->next;
    
    while (atual != queue) {
        printf(" ");
        print_elem(atual);
        atual = atual->next;
    }
    
    printf("]\n");

    return;
}

/**
* @params: queue - ponteiro duplo para a fila (permite modificar o ponteiro da fila)
*          elem - ponteiro para o elemento a ser inserido no final da fila
* Insere um elemento no final da fila.
* @return: 0 se sucesso, -1 se fila inválida, -10 se elemento inválido, -11 se elemento já está em outra fila
*/
int queue_append(queue_t **queue, queue_t *elem){
    if (queue == NULL) {
        fprintf(stderr, "ERRO NO APPEND: Fila inválida.\n");
        return -1;
    }

    if (elem == NULL) {
        fprintf(stderr, "ERRO NO APPEND: Elemento inválido.\n");
        return -10;
    }

    if (elem->prev != NULL || elem->next != NULL) {
        fprintf(stderr, "ERRO NO APPEND: Elemento já está em outra fila.\n");
        return -11;
    }

    // Pode ser o primeiro elemento da fila...
    if (*queue == NULL) {
        elem->prev = elem;
        elem->next = elem;
        *queue = elem;
        return 0;

    } 

    queue_t *penultimo = (*queue)->prev;

    elem->next = *queue;
    elem->prev = penultimo;
    penultimo->next = elem;
    (*queue)->prev = elem;

    return 0;
}

/**
* @params: queue - ponteiro duplo para a fila (permite modificar o ponteiro da fila)
*          elem - ponteiro para o elemento a ser removido da fila
* Remove um elemento específico da fila.
* @return: 0 se sucesso, -1 se fila nula, -10 se fila vazia, -11 se elemento inválido, 
*          -100 se fila inconsistente, -101 se elemento não está na fila
*/
int queue_remove(queue_t **queue, queue_t *elem){
    if (queue == NULL) {
        fprintf(stderr, "ERRO NO REMOVE: Fila nula.\n");
        return -1;
    }

    if (*queue == NULL) {
        fprintf(stderr, "ERRO NO REMOVE: Fila vazia.\n");
        return -10;
    }

    if (elem == NULL) {
        fprintf(stderr, "ERRO NO REMOVE: Elemento inválido.\n");
        return -11;
    }

    queue_t *aux = *queue;

    while (aux != elem){
        if(aux->next == NULL){
            fprintf(stderr, "ERRO NO REMOVE: Fila inconsistente - Elemento aponta para NULL.\n");
            return -100;
        }

        aux = aux->next;

        if(aux == *queue){
            fprintf(stderr, "ERRO NO REMOVE: Elemento não está na fila. \n");
            return -101;
        }
    }

    /**
     * Cenários possíveis de remoção:
     * 1. O elemento é o único na fila.
     * 2. O elemento é o primeiro da fila.
     * 3. O elemento está na fila ("caso base").
     */
    
    if (elem->next == elem && elem->prev == elem) {
        *queue = NULL;
    }

    if (elem == *queue) {
        *queue = elem->next;
    }

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    elem->prev = NULL;
    elem->next = NULL;

    return 0;
}
