#include "drive.h"

#include "colors.h"



static void
empty_it()
{
    return;
}


int
main() {
  int i, y;

  /* init hardware */
  if(init_hardware("hardware.ini") == 0) {
    fprintf(stderr, "Error in hardware initialization\n");
    exit(EXIT_FAILURE);
  }
  
  /* Interreupt handlers */
  for(i=0; i<16; i++)
    IRQVECTOR[i] = empty_it;

  /* Allows all IT */
  _mask(1);  init_hardware("hardware.ini");
  printf(BOLDGREEN"#frmt] start formating\n"RESET);
  for(i = 0; i < MAX_CYLINDER; i++) {
    printf(GREEN"formating %d, %d\r"RESET, i, 0);fflush(stdout); 
    for(y = 0; y < MAX_SECTOR; y++)
      format_sector(i, y, SECTOR_SIZE, 0);
  }
  return 0;
}
