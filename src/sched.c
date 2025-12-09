/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif
static void print_running_list(const char *action, int pid_affected) {//@nguyên
    printf("\t[DEBUG Scheduler] %s PID %2d | Running List Now: [", action, pid_affected);
    for (int i = 0; i < running_list.size; i++) {
        if (running_list.proc[i]) {
            printf("%d", running_list.proc[i]->pid);
            if (i < running_list.size - 1) printf(", ");
        }
    }
    printf("]\n");
}
int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return 0;//@nguyên
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(struct krnl_t *krnl) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i; 
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	running_list.size = 0;

	if (krnl != NULL) {
        krnl->running_list = &running_list;
        krnl->ready_queue = &ready_queue;
        #ifdef MLQ_SCHED
        krnl->mlq_ready_queue = mlq_ready_queue;
        #endif
    }

	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
void finish_proc(struct pcb_t * proc) {//@nguyên
    pthread_mutex_lock(&queue_lock);
    purgequeue(&running_list, proc);
	//print_running_list("Yielded (Removed)", proc->pid);
    pthread_mutex_unlock(&queue_lock);
}

struct pcb_t * get_mlq_proc(void) {//@nguyên
	struct pcb_t * proc = NULL;
	pthread_mutex_lock(&queue_lock);
	/* TODO: get a process from PRIORITY [ready_queue].
	 * It worth to protect by a mechanism.
	 * */
    /*for (int i = 0; i < MAX_PRIO; i++) {
        if (!empty(&mlq_ready_queue[i]) && slot[i] > 0) {
            
            proc = dequeue(&mlq_ready_queue[i]);
            slot[i]--;
            break; 
        }

        else if (!empty(&mlq_ready_queue[i]) && slot[i] == 0){
            slot[i] = MAX_PRIO - i;            
            continue;
        }
    }*/
	int found_process = 0;

    for (int i = 0; i < MAX_PRIO; i++) {
        // 1. Check if queue has work
        if (!empty(&mlq_ready_queue[i])) {
            
            // 2. Check if it has slot budget left
            if (slot[i] > 0) {
                proc = dequeue(&mlq_ready_queue[i]);
                slot[i]--;
                found_process = 1;
                break; // Found our process, exit loop
            }
            // If slots == 0, we simply continue to i+1. 
            // We DO NOT reset slots here. This forces the scheduler 
            // to move to lower priorities in the next function call.
        }
    }

    if (!found_process) {
        // Check if we need to reset slots because everyone is exhausted
        int work_exists = 0;
        for (int i = 0; i < MAX_PRIO; i++) {
            if (!empty(&mlq_ready_queue[i])) {
                work_exists = 1;
                slot[i] = MAX_PRIO - i; // Reset usage
            }
        }

        // If we reset slots, we should try to get a process again immediately
        if (work_exists) {
            // Simple recursion or goto to try again now that slots are full
            // For simplicity, let's just retry the loop once:
            for (int i = 0; i < MAX_PRIO; i++) {
                if (!empty(&mlq_ready_queue[i]) && slot[i] > 0) {
                    proc = dequeue(&mlq_ready_queue[i]);
                    slot[i]--;
                    break;
                }
            }
        }
    }
	if (proc != NULL) {
        enqueue(&running_list, proc);
		//print_running_list("Dispatched (Added)", proc->pid);
    }

	pthread_mutex_unlock(&queue_lock);

	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 *       It worth to protect by a mechanism.
	 * 
	 */
	//print_running_list("Yielded (Removed)", proc->pid);
	pthread_mutex_lock(&queue_lock);
	purgequeue(&running_list, proc);//@nguyên
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->mlq_ready_queue = mlq_ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_lock(&queue_lock);
	if (proc->prio >= MAX_PRIO) proc->prio = MAX_PRIO - 1;
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}

struct pcb_t *find_proc(struct krnl_t *krnl, uint32_t pid) {
   printf("DEBUG: find_proc pid=%d, list=%p\n", pid, krnl->running_list);
	if (krnl == NULL || krnl->running_list == NULL) return NULL;

    struct pcb_t *found = NULL;
    struct queue_t *list = krnl->running_list;

    pthread_mutex_lock(&queue_lock);
	if (list == NULL || list->proc == NULL) {
         pthread_mutex_unlock(&queue_lock);
         return NULL;
    }
    if (list->size < 0 || list->size > MAX_QUEUE_SIZE) {
        pthread_mutex_unlock(&queue_lock);
        return NULL;
    }

    for (int i = 0; i < list->size; i++) {
        struct pcb_t *proc = list->proc[i];

        if (proc == NULL) continue; 
        
        if (proc->pid == pid) {
            found = proc;
            break;
        }
    }
    pthread_mutex_unlock(&queue_lock);
    return found;
}

#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;

	pthread_mutex_lock(&queue_lock);
	/*TODO: get a process from [ready_queue].
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->krnl->ready_queue = &ready_queue;
	proc->krnl->running_list = &running_list;

	/* TODO: put running proc to running_list 
	 *       It worth to protect by a mechanism.
	 * 
	 */

	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif