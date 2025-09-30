/*

Nome: Ramon Pelle

GRR: 20244408

*/
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "../queue_0/queue.h"

// #define DEBUG

// ----------------
#define START_ID 0
#define STACKSIZE 64*1024	/* tamanho de pilha das threads (recuperado do contexts.c)*/
#define PRIO 0
#define MINPRIO 20
#define MAXPRIO -20
// ----------------

// estados das tarefas
#define CRIADA 0
#define RODANDO 1
#define SUSPENSA 2
#define TERMINADA 3
// ----------------

void dispatcher();
task_t *scheduler();

task_t *current_task, main_task, dispatcher_task;
task_t *ready_q = NULL; // Fila de tarefas prontas 

static int id_counter = START_ID;
static int user_tasks = 0; // Contador de tarefas do usuário

void ppos_init () {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf (stdout, 0, _IONBF, 0);
    
    main_task.id = id_counter++;
    main_task.prev = NULL;
    main_task.next = NULL;
    main_task.stack = NULL; 
    main_task.status = CRIADA;
    main_task.prio = PRIO;
    main_task.aging = PRIO;

    getcontext(&main_task.context);
    current_task = &main_task;

    user_tasks++; // main conta como uma tarefa de usuário

    // Cria a tarefa dispatcher
    task_init(&dispatcher_task, (void *)dispatcher, NULL);

    #ifdef DEBUG
    printf("ppos_init: main task inicializada. ID: %d\n", main_task.id);
    #endif
}

int task_init (task_t *task, void  (*start_func)(void *), void *arg) {
    if (!task || !start_func) return -1;

    task->id = id_counter++;
    task->prev = NULL;
    task->next = NULL;
    task->status = CRIADA;
    task->stack = malloc(STACKSIZE);
    task->prio = PRIO;
    task->aging = PRIO;

    // Inicializa contexto
    getcontext(&task->context);
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;

    makecontext(&task->context, (void (*)(void))start_func, 1, arg);

    // Adiciona tarefa na fila
    if (task != &dispatcher_task) {
        queue_append((queue_t **)&ready_q, (queue_t *)task);
        user_tasks++;
    }

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
    current_task->status = TERMINADA;
    
    // Se a tarefa que está sendo encerrada não for o dispatcher, transfere o controle para ele
    if (current_task != &dispatcher_task) {
        task_switch(&dispatcher_task);
    } else {
        task_switch(&main_task);
    }
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
task_t *scheduler() {
    if (!ready_q) {
        return NULL;
    }

    task_t *current = ready_q;
    task_t *next_task = ready_q;

    do {
        // Envelhece a tarefa atual
        current->aging--;

        // Verifica se a tarefa atual se tornou a de maior prioridade
        if (current->aging < next_task->aging) {
            next_task = current;
        }
        
        current = current->next;
    } while (current != ready_q);

    // Reseta a prioridade da tarefa que foi escolhida para execução
    next_task->aging = task_getprio(next_task);

    return next_task;
}

void dispatcher () {
    #ifdef DEBUG
    printf("dispatcher: iniciando, user_tasks: %d\n", user_tasks);
    #endif

    while (user_tasks > 0) {
        task_t *next_task = scheduler();
        if (!next_task) break; // Nenhuma tarefa pronta

        task_switch(next_task);

        #ifdef DEBUG
        printf("dispatcher: tarefa atual ID %d\n", current_task->id);
        #endif

        if (next_task->status == TERMINADA) {
            queue_remove((queue_t **)&ready_q, (queue_t *)next_task);
            free(next_task->stack);
            user_tasks--;
            #ifdef DEBUG
            printf("dispatcher: tarefa ID %d terminada e removida da fila\n", next_task->id);
            #endif
        } else {
            next_task->status = RODANDO;
        }

    }
    task_exit(0);
}

void task_yield () {
    if (current_task == &dispatcher_task) return; // Dispatcher não deve ceder
    
    #ifdef DEBUG
    printf("task_yield: tarefa ID %d cedendo o processador\n", current_task->id);
    #endif

    queue_remove((queue_t **)&ready_q, (queue_t *)current_task);
    queue_append((queue_t **)&ready_q, (queue_t *)current_task);
    task_switch(&dispatcher_task);
}

void task_setprio (task_t *task, int prio) {
    if (prio < -20 || prio > 20) return; // Prioridade inválida

    if (!task) {
        task = current_task;
    }
    if (prio > MINPRIO) {
        task->prio = MINPRIO;
    } else if (prio < MAXPRIO) {
        task->prio = MAXPRIO;
    } else {
        task->prio = prio;
    }

    task->aging = task->prio;

}

int task_getprio (task_t *task) {
    if (!task) return current_task->prio;
    return task->prio;
}

