/*

Nome: Ramon Pelle

GRR: 20244408

*/
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

// #define DEBUG

// ----------------
#define START_ID 0
#define STACKSIZE 64*1024	/* tamanho de pilha das threads (recuperado do contexts.c)*/

task_t *current_task, main_task;
static int id_counter = START_ID;

void ppos_init () {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    
    main_task.id = id_counter++;
    main_task.prev = NULL;
    main_task.next = NULL;
    main_task.stack = NULL; // main não precisa de pilha separada
    getcontext(&main_task.context); // Salva o contexto da main
    current_task = &main_task;

    #ifdef DEBUG
    printf("ppos_init: main task inicializada. ID: %d\n", main_task.id);
    #endif
}
// fazer depois
int task_init (task_t *task, void  (*start_func)(void *), void *arg) {
    if (!task || !start_func) return -1;

    task->id = id_counter++;
    task->prev = NULL;
    task->next = NULL;
    
    // Aloca pilha para a tarefa
    task->stack = malloc(STACKSIZE);
    if (!task->stack) return -1;

    // Inicializa contexto
    getcontext(&task->context);
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;

    makecontext(&task->context, (void (*)(void))start_func, 1, arg);

    #ifdef DEBUG
    printf("task_init: tarefa com ID %d criada\n", task->id);
    #endif

    return task->id;
}

int task_id () {
    return current_task->id;
}

void task_exit (int exit_code) {
    #ifdef DEBUG
    printf("task_exit: encerrando tarefa ID %d com código %d\n", current_task->id, exit_code);
    #endif

    task_switch(&main_task);
}

int task_switch (task_t *task) {
    if (!task) return -1;
    
    task_t *prev_task = current_task;
    
    #ifdef DEBUG
    printf("task_switch: trocando da tarefa ID: %d para a tarefa ID: %d\n", current_task->id, task->id);
    #endif

    current_task = task;
    return swapcontext(&prev_task->context, &task->context);
}
