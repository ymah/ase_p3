#include "sched.h"

int init_ctx(struct ctx_s *ctx, int stack_size, func_t f,struct parameters * args,char *name){


  ctx->ctx_stack = (char*) malloc(stack_size);
  if ( ctx->ctx_stack == NULL) return 1;
  ctx->ctx_name = name;
  ctx->ctx_state = CTX_RDY;
  ctx->ctx_size = stack_size;
  ctx->ctx_f = f;
  ctx->ctx_arg = args;
  ctx->ctx_rsp = &(ctx->ctx_stack[stack_size-8]);
  ctx->ctx_rbp = ctx->ctx_rsp;
  ctx->ctx_magic = CTX_MAGIC;
  ctx->ctx_next = ctx;
  nb_ctx++;
  printf(BOLDBLUE"\n%d ) creating ctx %s\n"RESET,nb_ctx,name);
  return 0;
}


void print_ctx(struct ctx_s *ctx){
  printf("> ");
  printf("%s with state %d\n",ctx->ctx_name,ctx->ctx_state);
}

void print_pile_ctx(){
  irq_disable();
  int i;
  struct ctx_s *ctx;
  ctx = ring_head;
  printf(BLUE"\n=============================\n"RESET);
  printf(BOLDBLUE"%d ctx\n"RESET,nb_ctx);
  printf("\n>><<\n");
  for(i=0;i<nb_ctx;i++){
    print_ctx(ctx);
    ctx = ctx->ctx_next;
  }
  printf(BLUE"\n=============================\n"RESET);
  irq_enable();
}
int create_ctx(int size, func_t f, struct parameters * args,char *name){

  struct ctx_s* new_ctx = (struct ctx_s*) malloc(sizeof(struct ctx_s));

  irq_disable();


  assert(new_ctx);

  if(init_ctx(new_ctx, size, f, args ,name)){ /* error */ return 1; }
  if(!ring_head){

    ring_head = new_ctx;
    ring_head->ctx_next = ring_head;
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
  (*current_ctx->ctx_f)(current_ctx->ctx_arg);
  current_ctx->ctx_state = CTX_END;
  yield();
}

void del_ctx(struct ctx_s *ctx){
  irq_disable();
  printf(RED"Deleting : "RESET);
  print_ctx(ctx);
  if(ctx == NULL)
    return;
  if(ctx->ctx_next == ctx){
    free(ctx);
    ctx = NULL;
  }else{
    struct ctx_s *suivant = ctx->ctx_next;
    struct ctx_s *precedent = ctx;
    while(precedent->ctx_next != ctx )
      precedent = precedent->ctx_next;
    precedent->ctx_next = suivant;
    ctx = suivant;
  }
  irq_enable();
}

void start(){
  printf("Entering in start for schedule");
  yield();
}



void switch_to_ctx(struct ctx_s *new_ctx){
  assert(new_ctx->ctx_magic == CTX_MAGIC);
  if(!current_ctx){
    return_ctx = (struct ctx_s*)malloc(sizeof(struct ctx_s));
    return_ctx->ctx_magic = CTX_MAGIC;
    __asm__ ("mov %%rsp, %0\n" :"=r"(return_ctx->ctx_rsp));
    __asm__ ("mov %%rbp, %0\n" :"=r"(return_ctx->ctx_rbp));
  }
  else{
    __asm__ ("mov %%rsp, %0\n" :"=r"(current_ctx->ctx_rsp));
    __asm__ ("mov %%rbp, %0\n" :"=r"(current_ctx->ctx_rbp));
  }

  current_ctx = new_ctx;

  __asm__ ("mov %0, %%rsp\n" ::"r"(current_ctx->ctx_rsp));
  __asm__ ("mov %0, %%rbp\n" ::"r"(current_ctx->ctx_rbp));

  irq_enable();
  if(current_ctx->ctx_state == CTX_RDY){
    start_current_ctx();
  }

}



void yield(){


  irq_disable();
  printf("\n !!!!!!!! CPT : %d!!!!!!!\n",cpt++);

  printf(GREEN"\n======================\n"RESET);
  printf(GREEN"\nENTERING YIELD\n"RESET);
  _out(TIMER_ALARM,TIMER);  /* alarm at next tick (at 0xFFFFFFFF) */
  print_pile_ctx();
  if(!current_ctx){
    current_ctx = ring_head;
  }
  switch_to_ctx(current_ctx);
}



void reset_ctx_disque(){
  irq_disable();
  printf(BOLDMAGENTA"\nENTERING reset_ctx_disque()\n"RESET);  
  printf("\tReset_ctx_disque : Le ctx qui a reçu l'IRQ :  ");
  print_ctx(ctx_disque);
  assert(ctx_disque->ctx_next);
  ctx_disque->ctx_next = current_ctx;
  current_ctx = ctx_disque;
  current_ctx->ctx_state = CTX_EXQ;
  yield();
}


void wait_disque(){
  /* assert(ctx_disque == (struct ctx_s *) 0); */
  irq_disable();
  printf(BOLDYELLOW"\nENTERING my_sleep()\n"RESET);
  printf("\nMy_sleep : le ctx demandeur :  ");
  print_ctx(current_ctx);
  ctx_disque = current_ctx;
  current_ctx->ctx_state = CTX_DISQUE;
  current_ctx = current_ctx->ctx_next;

  yield();
}



void bloquer(struct sem_s *sem){

  current_ctx->ctx_state = CTX_STP;
  current_ctx->ctx_sem_next = sem->sem_head;
  sem->sem_head = current_ctx;
  yield();
}


void debloquer(struct sem_s *sem){
  struct ctx_s* ctx;
  ctx = sem->sem_head;
  ctx->ctx_state = CTX_EXQ;
  sem->sem_head=ctx->ctx_sem_next;
  irq_enable();
}


void sem_init(struct sem_s *sem, unsigned int val, char* name){
  printf(BOLDGREEN"[init semaphore disque]"RESET GREEN"\n"RESET);
  assert(sem);
  sem->sem_cpt = val;
  sem->sem_head = NULL;
  sem->sem_name = name;
  assert(sem);
}


void sem_up(struct sem_s *sem){
  irq_disable();
  assert(sem);
  sem->sem_cpt++;
  if(sem->sem_cpt <= 0){
    printf("UNFREEZE : %s\n", sem->sem_name);
    debloquer(sem);
  }
  irq_enable();
}


void sem_down(struct sem_s *sem){
  irq_disable();
  assert(sem);
  sem->sem_cpt--;
  if(sem->sem_cpt < 0){
    printf("FREEZE : %s\n", sem->sem_name);
    bloquer(sem);
  }
  irq_enable();
}

