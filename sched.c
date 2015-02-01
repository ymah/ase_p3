/*
  TP Semaphore, réalisé par Yaker Mahieddine

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "sched.h"
#include "../ordonnanceur/hw.h"



int init_ctx(struct ctx_s *ctx, int stack_size, func_t f, void *args){
  ctx->ctx_stack = (char*) malloc(stack_size);
  if ( ctx->ctx_stack == NULL) return 1;
  ctx->ctx_state = CTX_RDY;
  ctx->ctx_size = stack_size;
  ctx->ctx_f = f;
  ctx->ctx_arg = args;
  ctx->ctx_rsp = &(ctx->ctx_stack[stack_size-16]);
  ctx->ctx_rbp = ctx->ctx_rsp;
  ctx->ctx_magic = CTX_MAGIC;
  ctx->ctx_next = ctx;
  return 0;
}



int create_ctx(int size, func_t f, void * args){
  struct ctx_s* new_ctx = (struct ctx_s*) malloc(sizeof(struct ctx_s));
  assert(new_ctx);
  irq_disable();
  if(init_ctx(new_ctx, size, f, args)){ /* error */ return 1; }

  if(!ring_head){
    ring_head = new_ctx;
    new_ctx->ctx_next = new_ctx;
  }
  else {
    new_ctx->ctx_next = ring_head->ctx_next;
    ring_head->ctx_next = new_ctx;
  }
  irq_enable();
  return 0;
}



void start_current_ctx(){
  current_ctx->ctx_state = CTX_EXQ;
  (current_ctx->ctx_f)(current_ctx->ctx_arg);
  current_ctx->ctx_state = CTX_END;
  yield();
}



void start(){
  printf("Entering in start for schedule");
  setup_irq(TIMER_IRQ, &yield);
  start_hw();
  yield();
}



void switch_to_ctx(struct ctx_s *new_ctx){
  struct ctx_s *ctx = new_ctx;
  assert(ctx->ctx_magic == CTX_MAGIC);
  irq_disable();
  while(ctx->ctx_state == CTX_END || ctx->ctx_state == CTX_STP){
    if(ctx->ctx_state == CTX_END) printf("Finished context encountered\n");
    if(ctx->ctx_state == CTX_STP) printf("Frozen context encountered\n");
    if(ctx == ctx->ctx_next){
      /* return to main */
      free(ctx->ctx_stack);
      free(ctx);
      printf("Return to main\n");
      __asm__ ("mov %0, %%rbp\n" ::"r"(return_ctx->ctx_rsp));
      __asm__ ("mov %0, %%rbp\n" ::"r"(return_ctx->ctx_rbp));
      return;
    }
    else {
      struct ctx_s *next = ctx->ctx_next;
      if (ctx->ctx_state == CTX_END){
	current_ctx->ctx_next = next;
	free(ctx->ctx_stack);
	free(ctx);
      }
      ctx = next;
    }
  }
  if(!current_ctx){
    return_ctx = (struct ctx_s*)malloc(sizeof(struct ctx_s));
    printf("Save return context\n");
    __asm__ ("mov %%rsp, %0\n" :"=r"(return_ctx->ctx_rsp));
    __asm__ ("mov %%rbp, %0\n" :"=r"(return_ctx->ctx_rbp));
  }
  else{
    __asm__ ("mov %%rsp, %0\n" :"=r"(current_ctx->ctx_rsp));
    __asm__ ("mov %%rbp, %0\n" :"=r"(current_ctx->ctx_rbp)); 
  }
  current_ctx = ctx;
  __asm__ ("mov %0, %%rsp\n" ::"r"(current_ctx->ctx_rsp));
  __asm__ ("mov %0, %%rbp\n" ::"r"(current_ctx->ctx_rbp));
  irq_enable();
  if(current_ctx->ctx_state == CTX_RDY){
    start_current_ctx();
  }
}



void yield(){
  if(!current_ctx){
    assert(ring_head);
    switch_to_ctx(ring_head);
  }
  else{
    switch_to_ctx(current_ctx->ctx_next);
  }
}





void bloquer(struct sem_s *sem){
  irq_disable();
  current_ctx->ctx_state = CTX_STP;
  current_ctx->ctx_sem_next = sem->sem_head;
  sem->sem_head = current_ctx;
  irq_enable();
  yield();
}


void debloquer(struct sem_s *sem){
  struct ctx_s* ctx;
  irq_disable();
  ctx = sem->sem_head;
  ctx->ctx_state = CTX_EXQ;
  sem->sem_head=ctx->ctx_sem_next;
  irq_enable();
  /*	switch_to_ctx(ctx); */
}


void sem_init(struct sem_s *sem, unsigned int val, char* name){
  sem->sem_cpt = val;
  sem->sem_head = NULL;
  sem->sem_name = name;
}


void sem_up(struct sem_s *sem){
  irq_disable();
  sem->sem_cpt++;
  if(sem->sem_cpt <= 0){
    printf("UNFREEZE : %s\n", sem->sem_name);
    debloquer(sem);
  }
  irq_enable();
}


void sem_down(struct sem_s *sem){
  irq_disable();
  sem->sem_cpt--;
  if(sem->sem_cpt < 0){
    printf("FREEZE : %s\n", sem->sem_name);
    bloquer(sem);
  }
  irq_enable();
}


/*
  Ancienne fonction utilisée pour tester le systeme de contexte.

  void f_ping(void *args) {
  int i;
  printf("A\n") ;
  for(i=0; i < LOOP ; i++);
  printf("B\n") ; 
  for(i=0; i < LOOP ; i++);
  printf("C\n") ; 
  }


  void f_pong(void *args) {
  int i=0;
  printf("1\n") ;
  for(i=0; i < LOOP ; i++);
  printf("2\n") ; 
  for(i=0; i < LOOP ; i++);
  printf("3\n") ; 
  for(i=0; i < LOOP ; i++);
  printf("4\n") ; 
  for(i=0; i < LOOP ; i++);
  printf("5\n") ; 
  for(i=0; i < LOOP ; i++);
  printf("6\n") ; 
  }


  int main(int argc, char*argv[]){
  create_ctx(16384, f_ping, NULL);
  create_ctx(16384, f_pong, NULL);
  start();
  printf("done\n");
  exit(EXIT_SUCCESS);
  }
*/


void produce_object(object_t* object){
  object->value=1;
}


void use_object(object_t object){
  object.value = 0;
}


void push_object(object_t object){
  next_index++;
  if(next_index>N){
    printf("FAIL!\n");
  }
}


void pull_object(object_t* object){
  next_index--;
  if(next_index<0){
    printf("FAIL!\n");
  }
}



void producer(){
  object_t object;
  while(1){
    printf("produce: %d/%d\n", next_index, vide.sem_cpt);
    produce_object(&object);
    sem_down(&vide);
    sem_down(&mutex);
    push_object(object);
    sem_up(&mutex);
    sem_up(&plein);
  }
}


void consumer(){
  object_t object;
  while(1){
    printf("consume: %d/%d\n", next_index, plein.sem_cpt);
    sem_down(&plein);
    sem_down(&mutex);
    pull_object(&object);
    sem_up(&mutex);
    sem_up(&vide);
    use_object(object);
  }
}


/* int main(int argc, char*argv[]){ */
/*   sem_init(&mutex, 1, "mutex");  */
/*   sem_init(&vide, N, "vide");  */
/*   sem_init(&plein, 0, "plein"); */

/*   create_ctx(16384, &producer, NULL); */
/*   create_ctx(16384, &consumer, NULL); */
/*   start(); */
/*   printf("done\n"); */
/*   exit(EXIT_SUCCESS); */
/* } */