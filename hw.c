#include "hw.h"




void irq_enable(){

  _mask(1);
  enable_irq = 1;
}
void irq_disable(){

  _mask(16);
  enable_irq = 0;
}
