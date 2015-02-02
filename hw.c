#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>
#include "hardware.h"
#include "hw.h"




void irq_enable(){
  _mask(0);
}
void irq_disable(){
  _mask(15);
}
