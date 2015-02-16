#include "sched.h"

void print_ctx(struct ctx_s *ctx){
  printf("|> ");
  printf(BOLDWHITE"%s with state %d\n"RESET,ctx->ctx_name,ctx->ctx_state);
}

void print_pile_ctx(){
  irq_disable();
  unsigned int i;
  struct ctx_s *ctx;
  assert(ring_head);
  ctx = ring_head;

  printf(BOLDGREEN"\n=============================\n"RESET);
  printf(BOLDGREEN"%d ctx\n"RESET,nb_ctx);
  printf("\n>><<\n");
  for(i=0;i<nb_ctx;i++){
    print_ctx(ctx);
    ctx = ctx->ctx_next;
  }
  printf(BLUE"\n=============================\n"RESET);
  irq_enable();
}


int init_ctx(struct ctx_s *ctx, int stack_size, func_t f,struct parameters * args,char *name){

  ctx->ctx_stack = (char*) calloc(stack_size,sizeof(char));
  if ( ctx->ctx_stack == NULL) return 1;

  ctx->ctx_name = name;
  ctx->ctx_state = CTX_RDY;
  ctx->ctx_size = stack_size;
  ctx->ctx_f = f;
  ctx->ctx_arg = args;
  ctx->ctx_rsp = &(ctx->ctx_stack[stack_size-16]);
  ctx->ctx_rbp = ctx->ctx_rsp;
  ctx->ctx_magic = CTX_MAGIC;
  ctx->ctx_next = ctx;
  nb_ctx++;

  if(DEBUG)
    printf(BOLDBLUE"\n%d ) creating ctx %s\n"RESET,nb_ctx,name);

  return 0;
}
int create_ctx(int size, func_t f, struct parameters * args,char *name){


  irq_disable();

  struct ctx_s* new_ctx = (struct ctx_s*) calloc(1,sizeof(struct ctx_s));


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
  yield();
  return 0;
}






void del_ctx(struct ctx_s *ctx){
  irq_disable();
  assert(ctx != NULL);
  if(DEBUG)
    printf(RED"Deleting : "RESET);

  if(DEBUG)
    print_ctx(ctx);

  if(DEBUG)
    printf(RED"ADDRESS : %p \n"RESET,ctx);


  if(ctx == ctx->ctx_next){
    free(ctx->ctx_stack);
    free(ctx);
  }else{
    struct ctx_s *suivant = ctx->ctx_next;
    struct ctx_s *precedent = ctx;
    while(precedent->ctx_next != ctx )
      precedent = precedent->ctx_next;
    precedent->ctx_next = suivant;
    ctx = NULL;
  }


}

void start(){
  printf("Entering in start for schedule");
  yield();
}



void switch_to_ctx(struct ctx_s *new_ctx){
  assert(new_ctx != NULL);
  assert(new_ctx->ctx_magic == CTX_MAGIC);
  if(!current_ctx){
    return_ctx = (struct ctx_s*)calloc(1,sizeof(struct ctx_s));
    return_ctx->ctx_magic = CTX_MAGIC;
    __asm__ ("mov %%rsp, %0\n" :"=r"(return_ctx->ctx_rsp));
    __asm__ ("mov %%rbp, %0\n" :"=r"(return_ctx->ctx_rbp));
  }
  __asm__ ("mov %%rsp, %0\n" :"=r"(current_ctx->ctx_rsp));
  __asm__ ("mov %%rbp, %0\n" :"=r"(current_ctx->ctx_rbp));


  current_ctx = new_ctx;

  __asm__ ("mov %0, %%rsp\n" ::"r"(current_ctx->ctx_rsp));
  __asm__ ("mov %0, %%rbp\n" ::"r"(current_ctx->ctx_rbp));


  if(current_ctx->ctx_state == CTX_RDY){
    current_ctx->ctx_state = CTX_EXQ;
    irq_enable();
    (*current_ctx->ctx_f)(current_ctx->ctx_arg);
    irq_disable();
    current_ctx->ctx_state = CTX_END;
    yield();

  }

}



void yield(){


  irq_disable();
  _out(TIMER_ALARM,TIMER);  /* alarm at next tick (at 0xFFFFFFFF) */
  if(DEBUG){
    printf("\n !!!!!!!! CPT : %d!!!!!!!\n",cpt++);
    printf(GREEN"\n======================\n"RESET);
    printf(GREEN"\nENTERING YIELD\n"RESET);
    printf(GREEN"\n======================\n"RESET);
    if(current_ctx)
      print_ctx(current_ctx);
    else
      print_ctx(ring_head);
    printf(GREEN"\n======================\n"RESET);
    print_pile_ctx();
  }
  if(!current_ctx){
    current_ctx = ring_head;
    current_ctx->ctx_next = ring_head;
    switch_to_ctx(current_ctx);
  }else{
    struct ctx_s * ctx = current_ctx->ctx_next;


    while(1){

      if(ctx->ctx_state == CTX_RDY){
        break;
      }
      if(ctx->ctx_state == CTX_EXQ){
        break;
      }
      if(ctx->ctx_state == CTX_DISQUE || ctx->ctx_state == CTX_STP){
        continue;
      }
      if(ctx->ctx_state == CTX_END){
        struct ctx_s *tmp;
        tmp = ctx->ctx_next;
        del_ctx(ctx);
        ctx = tmp;
        continue;
      }
    }

    switch_to_ctx(ctx);
  }
}



void reset_ctx_disque(){
  irq_disable();
  if(DEBUG){
    printf(BOLDMAGENTA"\nENTERING reset_ctx_disque()\n"RESET);  
    printf("\tReset_ctx_disque : Le ctx qui a reçu l'IRQ :  ");
    print_ctx(ctx_disque);
  }
  assert(ctx_disque != NULL);
  ctx_disque->ctx_next = current_ctx;
  current_ctx = ctx_disque;
  current_ctx->ctx_state = CTX_EXQ;
  irq_enable();
}


void wait_disque(){

  irq_disable();
  if(DEBUG)
    printf(BOLDYELLOW"\nENTERING wait_disque()\n"RESET);
  if(DEBUG)
    printf("\nWait_disque(): le ctx demandeur :  ");
  if(DEBUG)
    print_ctx(current_ctx);
  ctx_disque = current_ctx;
  ctx_disque->ctx_state = CTX_DISQUE;
  yield();
}



void sem_init(struct sem_s *sem, unsigned int val, char* name){
  printf(BOLDGREEN"[init semaphore disque]"RESET GREEN"\n"RESET);
  assert(sem);
  sem->init_cpt = sem->sem_cpt = val;
  sem->sem_head = NULL;
  sem->sem_name = name;
  assert(sem);
}

void sem_up(struct sem_s *sem){

  irq_disable();
  if(DEBUG){
    printf(BOLDMAGENTA"\n[Tentative de rendu de semaphre] %s avec comme etat : %d\n"RESET,ctx_disque->ctx_name,sem->sem_cpt);
  }
  sem->sem_cpt++;
  if(sem->sem_cpt > 0){
    struct ctx_s *ctx_tmp = sem -> sem_head->ctx_next;
    sem->sem_head->ctx_state = CTX_EXQ;
    sem->sem_head->ctx_next = ring_head;
    current_ctx = sem->sem_head;
    sem->sem_head = ctx_tmp;
  }
  if(DEBUG){
    printf(BOLDMAGENTA"\n[SEMAPHORE DISQUE RENDU PAR] %s avec comme nouvel etat : %d\n"RESET,ctx_disque->ctx_name,sem->sem_cpt);
  }

  irq_enable();

}


void sem_down(struct sem_s *sem){
  irq_disable();
  if(DEBUG)
    printf(BOLDMAGENTA"\n[Tentative de prise de semaphre] %s avec comme etat : %d\n"RESET,current_ctx->ctx_name,sem->sem_cpt);

  if(sem->sem_cpt == 0){
    printf("FREEZE");
    current_ctx->ctx_state = CTX_STP;
    if(!sem->sem_head){
      current_ctx->ctx_sem_next = sem->sem_last;
      sem->sem_head = current_ctx;
    }else{
      current_ctx->ctx_sem_next = sem->sem_head;
      sem->sem_last->ctx_next = current_ctx;
      sem->sem_last = current_ctx;
    }
    yield();
  }
  sem->sem_cpt--;
  if(DEBUG)
    printf(BOLDMAGENTA"\n[SEMAPHORE DISQUE PRIS PAR] %s avec comme nouvel etat : %d\n"RESET,current_ctx->ctx_name,sem->sem_cpt);

  irq_enable();
  yield();
}

