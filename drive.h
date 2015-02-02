#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "hardware.h"
#include "hw.h"
/* #include "sched.h" */

#ifndef _DRIVE
#define _DRIVE


struct sem_s *semaphore_disque;


void check_hda();
void read_sector(unsigned int cylinder, unsigned int sector, unsigned char *buffer);
void read_sector_n(unsigned int cylinder, unsigned int sector, unsigned char *buffer, int n);
void write_sector(unsigned int cylinder, unsigned int sector, const unsigned char *buffer);
void write_sector_n(unsigned int cylinder, unsigned int sector, const unsigned char *buffer, int n);
void format_sector(unsigned int cylinder, unsigned int sector, unsigned int nsector,unsigned int value);
void init_master();


#endif
