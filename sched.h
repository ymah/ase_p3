/*
TP Semaphore, réalisé par Yaker Mahieddine
 */
#ifndef _SCHED_H
#define _SCHED_H


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

#include "hardware.h"
#include "hw.h"
#include "colors.h"

#define CTX_MAGIC 0xBABE

#define LOOP 1000000000
#define N 100

int next_index;


typedef void (func_t) (void *);
/* typedef void (irq_handler_func_t)(void); */
typedef enum {CTX_RDY, CTX_EXQ, CTX_STP, CTX_END} state_e;




struct ctx_s {
  char *ctx_name;
  void* ctx_rsp;
  void* ctx_rbp;
  unsigned ctx_magic;
  func_t* ctx_f;
  struct parameters* ctx_arg;
  state_e ctx_state;
  char* ctx_stack; /* adresse la plus basse de la pile */
  unsigned ctx_size;
  struct ctx_s* ctx_next;
  struct ctx_s* ctx_sem_next;
};

struct parameters{
  unsigned int cylinder;
  unsigned int sector;
  const unsigned char *buffer;
  int n;
};

struct parameters_m{
  unsigned int cylinder;
  unsigned int sector;

};


struct sem_s {
  char* sem_name;
  int sem_cpt;
  struct ctx_s* sem_head;
};

struct ctx_s* current_ctx;
struct ctx_s* ring_head;
struct ctx_s* return_ctx;
struct sem_s mutex, vide, plein;
struct ctx_s* ctx_disque;

typedef struct object_s{
	int value;
} object_t;






object_t stack[N];


int init_ctx(struct ctx_s *ctx, int stack_size, func_t f, struct parameters *args,char * name);
int create_ctx(int size, func_t f, struct parameters * args,char * name);
void start_current_ctx();
void print_ctx(struct ctx_s *ctx);
void start();
void switch_to_ctx(struct ctx_s *new_ctx);
void yield();
void yield_disque();
void my_sleep();
void sem_init(struct sem_s *sem, unsigned int val, char* name);
void sem_up(struct sem_s *sem);
void sem_down(struct sem_s *sem);



#endif
