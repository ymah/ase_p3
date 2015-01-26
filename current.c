#include "current.h"

/* load super bloc of the $CURRENT_VOLUME
   set current_volume accordingly */
int
load_current_volume ()
{
    char* current_volume_str;
    int status;
    
    current_volume_str = getenv("CURRENT_VOLUME");
    if (! current_volume_str) {
      printf("Current volume undefined.\n");
      exit(EXIT_FAILURE);
    }

    errno = 0;
    current_volume = strtol(current_volume_str, NULL, 10);

    status = load_super(current_volume);
    
    return status;
}

/* return hw_config filename */
char *
get_hw_config ()
{
    char* hw_config;

    hw_config = getenv("HW_CONFIG");
    return hw_config ? hw_config : DEFAULT_HW_CONFIG;
}
