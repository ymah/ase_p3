/* ------------------------------
   $Id: mount.c,v 1.2 2009/11/17 16:20:54 marquet Exp $
   ------------------------------------------------------------

   Initialization and finalization
   Philippe Marquet, Nov 2009
   
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include "hardware.h"
#include "config.h"
#include "tools.h"
#include "mount.h"
#include "mbr.h"
#include "super.h"
#include "colors.h"

/* load super bloc of the $CURRENT_VOLUME
   set current_volume accordingly */

void
load_current_volume ()
{
    char* current_volume_str;
    int status;
    current_volume_str = getenv("CURRENT_VOLUME");
    ffatal(current_volume_str != NULL, "no definition of $CURRENT_VOLUME"); 

    errno = 0;
    current_volume = strtol(current_volume_str, NULL, 10);
    ffatal(!errno, "bad value of $CURRENT_VOLUME %s", current_volume_str);
    printf(BOLDGREEN"[load current volume]"RESET GREEN" load super from vol %d\n"RESET, current_volume);
    status =load_super(current_volume);
    printf(BOLDGREEN"[load current volume]"RESET GREEN" status = %d\n"RESET, status);
    ffatal(status, BOLDRED"unable to load super of vol %d"RESET, current_volume);
}

/* return hw_config filename */
static char *
get_hw_config ()
{
    char* hw_config;

    hw_config = getenv("HW_CONFIG");
    return hw_config ? hw_config : DEFAULT_HW_CONFIG;
}

static void
emptyIT()
{
    return;
}

/* ------------------------------
   Initialization and finalization fucntions
   ------------------------------------------------------------*/
void
mount()
{
    char *hw_config;
    int status, i; 

    /* Hardware initialization */
    printf(BOLDGREEN"[mount sequence]"RESET GREEN" get hw config\n");
    hw_config = get_hw_config();
    printf(BOLDGREEN"[mount sequence]"RESET GREEN" init hardware\n");
    status = init_hardware(hw_config);
    ffatal(status, "error in hardware initialization with %s\n", hw_config);

    /* Interrupt handlers */
    for(i=0; i<16; i++)
	IRQVECTOR[i] = emptyIT;
    
    /* Allows all IT */
    _mask(1);

    /* Load MBR and current volume */
    printf(BOLDGREEN"[mount sequence]"RESET GREEN" load mbr\n");
    load_mbr();
    printf(BOLDGREEN"[mount sequence]"RESET GREEN" load current volume\n");
    load_current_volume();
}

void
umount()
{
    /* save current volume super bloc */
    save_super();

    /* bye */
}
 

