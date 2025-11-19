/*

Nome: Ramon Pelle

GRR: 20244408

*/
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "ppos.h"
#include "../queue_0/queue.h"

// #define DEBUG

// ----------------
#define START_ID 0
#define STACKSIZE 64*1024	/* tamanho de pilha das threads (recuperado do contexts.c)*/
#define PRIO 0
#define MINPRIO 20
#define MAXPRIO -20
#define QUANTUM 10  // quantum padrão em ticks
// ----------------

// estados das tarefas
#define CRIADA 0
#define RODANDO 1
#define SUSPENSA 2
#define TERMINADA 3
#define PRONTA 4
#define ESPERA 5
// ----------------

void dispatcher();
task_t *scheduler();
void timer_handler(int signum);
void configura_timer();
unsigned int systime () ;

task_t *current_task, main_task, dispatcher_task;
task_t *ready_q = NULL; // Fila de tarefas prontas
task_t *waiting_q = NULL; // Fila de tarefas em espera

static int id_counter = START_ID;
static int user_tasks = 0; // Contador de tarefas do usuário

// Variáveis para controle de preempção
static struct itimerval timer;
struct sigaction action;
static int is_preempcao = 1;

// Variável global de tempo
unsigned int sys_clock = 0;
unsigned int systime (){
	return sys_clock;
}



void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);
    
    main_task.id = id_counter++;
    main_task.prev = NULL;
    main_task.next = NULL;
    main_task.stack = NULL; 
    main_task.status = CRIADA;
    main_task.prio = PRIO;
    main_task.aging = PRIO;
    main_task.systask = 0;
    main_task.quantum = QUANTUM;
    main_task.exec_time = systime();
    main_task.cpu_time = 0;
    main_task.activations = 1; // main já inicia ativa
    main_task.exit_code = 0;
    main_task.waiting_queue = NULL;

    getcontext(&main_task.context);
    current_task = &main_task;

    // Cria a tarefa dispatcher
    task_init(&dispatcher_task, (void *)dispatcher, NULL);
    dispatcher_task.systask = 1; // dispatcher é tarefa de sistema
    dispatcher_task.exec_time = systime();
    dispatcher_task.activations = 0;

    configura_timer();
    sys_clock = 0;

    #ifdef DEBUG
    printf("ppos_init: main task inicializada. ID: %d\n", main_task.id);
    #endif
}

int task_init(task_t *task, void (*start_func)(void *), void *arg) {
    if (!task || !start_func) return -1;

    task->id = id_counter++;
    task->prev = NULL;
    task->next = NULL;
    task->status = CRIADA;
    task->stack = malloc(STACKSIZE);
    task->prio = PRIO;
    task->aging = PRIO;
    task->systask = 0; // por padrão, tarefas são de usuário
    task->quantum = QUANTUM;
    task->exec_time = 0; // será definido na primeira ativação
    task->cpu_time = 0;
    task->activations = 0;
    task->exit_code = 0;
    task->waiting_queue = NULL;

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

int task_id() {
    return current_task->id;
}

void task_exit(int exit_code) {
    current_task->exec_time = systime() - current_task->exec_time;
    printf("Task %d exit: execution time %5u ms, processor time %5u ms, %d activations\n", 
           current_task->id, current_task->exec_time, current_task->cpu_time, current_task->activations);

    // Define o código de saída da tarefa
    current_task->exit_code = exit_code;
    current_task->status = TERMINADA;
    
    // Acorda todas as tarefas que estão esperando por esta tarefa
    while (current_task->waiting_queue) {
        task_t *waiting_task = (task_t *)current_task->waiting_queue;
        task_awake(waiting_task, (task_t **)&(current_task->waiting_queue));
    }

    // Se a tarefa que está sendo encerrada não for o dispatcher, transfere o controle para ele
    if (current_task != &dispatcher_task) {
        dispatcher_task.activations++; // Conta ativação do dispatcher
        task_switch(&dispatcher_task);
    } else {
        task_switch(&main_task);
    }
}

int task_switch(task_t *task) {
    if (!task) return -1;

    is_preempcao = 0; // Desabilita preempções durante a troca

    task_t *prev_task = current_task;
    
    #ifdef DEBUG
    printf("task_switch: trocando da tarefa ID: %d para a tarefa ID: %d\n", current_task->id, task->id);
    #endif

    current_task = task;

    is_preempcao = 1; // Reabilita preempções
    
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
void dispatcher() {
    #ifdef DEBUG
    printf("dispatcher: iniciando, user_tasks: %d\n", user_tasks);
    #endif

    while (user_tasks > 0) {
        task_t *next_task = scheduler();
        if (next_task) {
            
            // 1. REMOVE a tarefa da fila de prontos ANTES de executar
            queue_remove((queue_t **)&ready_q, (queue_t *)next_task);

            // Reseta o quantum da tarefa
            next_task->quantum = QUANTUM;
            
            // Incrementa contador de ativações da tarefa
            next_task->activations++;
            
            // Se é a primeira ativação, marca o tempo de início
            if (next_task->activations == 1) {
                next_task->exec_time = systime();
            }

            unsigned int ticks1 = systime();
            task_switch(next_task); // Executa a tarefa
            unsigned int ticks2 = systime();
            next_task->cpu_time += ticks2 - ticks1;

            // O código abaixo executa QUANDO a tarefa retorna ao dispatcher

            if (next_task->status == TERMINADA) {
                free(next_task->stack); // Libera a pilha
                user_tasks--;
                #ifdef DEBUG
                printf("dispatcher: tarefa ID %d terminada e removida\n", next_task->id);
                #endif
            } else {
                // 2. Se a tarefa não terminou, ADICIONA de volta à fila de prontos
                queue_append((queue_t **)&ready_q, (queue_t *)next_task);
            }
        }
    }
    task_exit(0);
}

void task_yield() {
    if (current_task == &dispatcher_task) return;

    #ifdef DEBUG
    printf("task_yield: tarefa ID %d cedendo o processador\n", current_task->id);
    #endif
    
    // Conta ativação do dispatcher
    dispatcher_task.activations++;
    
    // Apenas devolve o controle para o dispatcher
    task_switch(&dispatcher_task);
}

void task_setprio(task_t *task, int prio) {
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

int task_getprio(task_t *task) {
    if (!task) return current_task->prio;
    return task->prio;
}

void timer_handler(int signum) {
    if (!current_task) return;
    
    sys_clock++; // Incrementa o relógio do sistema
    current_task->quantum--;
    
    // Verifica se o quantum acabou e se a tarefa atual pode ser preemptada
    if (current_task->quantum <= 0) {
        // Só preempta se preempções estiverem habilitadas E for tarefa de usuário
        if (is_preempcao && current_task->systask == 0) {
            task_yield();
        }
    }
}
void configura_timer() {
    action.sa_handler = timer_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        perror("Erro em sigaction: ");
        exit(1);
    }

    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000; // 1ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000; // 1ms

    if (setitimer(ITIMER_REAL, &timer, 0) < 0) {
        perror("Erro em setitimer: ");
        exit(1);
    }
}

void task_awake(task_t *task, task_t **queue) {
    if (!task) return;
    
    is_preempcao = 0;
    // Se a fila queue não for nula, retira a tarefa apontada por task dessa fila
    if (queue) {
        queue_remove((queue_t **)queue, (queue_t *)task);
    }
    
    // Ajusta o status dessa tarefa para "pronta"
    task->status = PRONTA;
    
    // Insere a tarefa na fila de tarefas prontas
    queue_append((queue_t **)&ready_q, (queue_t *)task);
    is_preempcao = 1;
    // Continua a tarefa atual (não retorna ao dispatcher)
}

void task_suspend(task_t **queue) {
    // Retira a tarefa atual da fila de tarefas prontas (se estiver nela)
    queue_remove((queue_t **)&ready_q, (queue_t *)current_task);
    
    // Ajusta o status da tarefa atual para "suspensa"
    current_task->status = SUSPENSA;
    
    // Insere a tarefa atual na fila apontada por queue (se essa fila não for nula)
    if (queue) {
        queue_append((queue_t **)queue, (queue_t *)current_task);
    }
    
    // Retorna ao dispatcher
    task_switch(&dispatcher_task);
}

int task_wait(task_t *task) {
    if (!task) return -1;
    
    is_preempcao = 0;
    // Se a tarefa já terminou, retorna imediatamente com o exit_code
    if (task->status == TERMINADA) {
        return task->exit_code;
    }
    
    // Suspende a tarefa atual na fila de espera da tarefa especificada
    task_suspend(&task->waiting_queue);
    is_preempcao = 1;
    // Quando a tarefa acordar, retorna o exit_code da tarefa esperada
    return task->exit_code;
}