#include "hw.h"




void irq_enable(){
  _mask(0);
}
void irq_disable(){
  _mask(15);
}
