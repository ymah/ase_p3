/*
TP Semaphore, réalisé par Yaker Mahieddine
 */



#ifndef _SCHED_H
#define _SCHED_H
#define CTX_MAGIC 0xBABE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "hw.h"

#define LOOP 1000000000
#define N 100

int next_index = 0;


typedef void (func_t) (void *);
/* typedef void (irq_handler_func_t)(void); */
typedef enum {CTX_RDY, CTX_EXQ, CTX_STP, CTX_END} state_e;




struct ctx_s {
  void* ctx_rsp;
  void* ctx_rbp;
  unsigned ctx_magic;
  func_t* ctx_f;
  void* ctx_arg;
  state_e ctx_state;
  char* ctx_stack; /* adresse la plus basse de la pile */
  unsigned ctx_size;
  struct ctx_s* ctx_next;
  struct ctx_s* ctx_sem_next;
};




struct sem_s {
  char* sem_name;
  int sem_cpt;
  struct ctx_s* sem_head;
};

struct ctx_s* current_ctx = (struct ctx_s *) 0;
struct ctx_s* ring_head = (struct ctx_s *) 0;
struct ctx_s* return_ctx;
struct sem_s mutex, vide, plein;


typedef struct object_s{
	int value;
} object_t;






object_t stack[N];

int init_ctx(struct ctx_s *ctx, int stack_size, func_t f, void *args);
int create_ctx(int size, func_t f, void * args);
void start_current_ctx();
void start();
void switch_to_ctx(struct ctx_s *new_ctx);
void yield();
void sem_init(struct sem_s *sem, unsigned int val, char* name);
void sem_up(struct sem_s *sem);
void sem_down(struct sem_s *sem);


#endif
