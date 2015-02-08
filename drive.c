#include "drive.h"


static void go_to_sector(int cylindre, int sector);

/* utilise pour initialiser le hardware */
static void
empty_it()
{
  return;
}

void init_master() {
  unsigned int i;

  /* init hardware */
  if(init_hardware("hardware.ini") == 0) {
    fprintf(stderr, "Error in hardware initialization\n");
    exit(EXIT_FAILURE);
  }

  /* Interreupt handlers */
  for(i=0; i<16; i++)
    IRQVECTOR[i] = empty_it;

  /* Allows all IT */
  _mask(1);


  /*init semaphore du disque*/
}

void format_sector(unsigned int cylinder, unsigned int sector, unsigned int nsector, unsigned int value) {

  go_to_sector(cylinder, sector);
  _out(HDA_DATAREGS, (nsector >> 8) & 0xFF);
  _out(HDA_DATAREGS + 1, nsector & 0xFF);
  _out(HDA_DATAREGS + 2, (value >> 24) & 0xFF);
  _out(HDA_DATAREGS + 3, (value >> 16) & 0xFF);
  _out(HDA_DATAREGS + 4, (value >> 8) & 0xFF);
  _out(HDA_DATAREGS + 5, value & 0xFF);
  _out(HDA_CMDREG, CMD_FORMAT);
  yield();
}

/* lit n nombre de secteur */
void read_sector_n(unsigned int cylinder, unsigned int sector, unsigned char *buffer, int n) {
  if(n > SECTOR_SIZE) {
    printf("Vous essayez de lire un nombre de secteur plus grand que le nombre de secteur dispo\n");
    exit(EXIT_FAILURE);
  }

  go_to_sector(cylinder, sector);

  _out(HDA_DATAREGS, 1 & 0xFF);
  _out(HDA_CMDREG, CMD_READ);
  yield();
  memcpy(buffer,MASTERBUFFER,n);


}


void read_sector(unsigned int cylinder, unsigned int sector, unsigned char *buffer) {


  struct parameters_d *str=calloc(1,sizeof(struct parameters_d));
  str->cylinder = cylinder;
  str->sector = sector;
  str->buffer = buffer;
  str->n = SECTOR_SIZE;

  create_ctx(16384,&read_sector_n,(void *)str,"contexte lecture disque");
  /* read_sector_n(cylinder, sector, buffer, SECTOR_SIZE); */
}

void write_sector_n(unsigned int cylinder, unsigned int sector, const unsigned char *buffer, int n) {

  int i;
  struct parameters_m *str;
  str = calloc(1,sizeof(struct parameters_m));
  str->cylinder = cylinder;
  str->sector = sector;

  if(n > SECTOR_SIZE) {
    printf("Vous essayez de lire un nombre de secteur plus grand que le nombre de secteur dispo\n");
    exit(EXIT_FAILURE);
  }


  for(i = 0; i < SECTOR_SIZE; i++)
    MASTERBUFFER[i] = 0;

  create_ctx(16384,&go_to_sector,str,"go to sector context");
  sem_down(semaphore_disque);
  _out(HDA_DATAREGS, 0);
  _out(HDA_DATAREGS+1, 1 & 0xFF);
  memcpy(MASTERBUFFER,buffer,n);
  _out(HDA_CMDREG, CMD_WRITE);

  yield();
  sem_up(semaphore_disque);

}


void write_sector(unsigned int cylinder, unsigned int sector, const unsigned char *buffer) {

  struct parameters_d *str=calloc(1,sizeof(struct parameters_d));
  str->cylinder = cylinder;
  str->sector = sector;
  str->buffer = buffer;
  str->n = SECTOR_SIZE;

  create_ctx(16384,&write_sector_n,str,"contexte ecriture disque");

  /* write_sector_n(cylinder, sector, buffer, SECTOR_SIZE); */
}



static void go_to_sector(int cylinder, int sector) {
  /* on verifie que les valeures passees en parametres sont les bonnes */
  if(cylinder > MAX_CYLINDER) {
    printf("Appel de la fonction go_to_sector avec un cylinder superieur a MAX_CYLINDER\n");
    exit(EXIT_FAILURE);
  }
  if(sector > MAX_SECTOR) {
    printf("Appel de la fonction go_to_sector avec un cylinder superieur a MAX_CYLINDER\n");
    exit(EXIT_FAILURE);
  }

  sem_down(semaphore_disque);
  printf("go to : %d %d %d %d\n",(cylinder >> 8) & 0xFF,cylinder & 0xFF, (sector >> 8) & 0xFF,sector & 0xFF);
  _out(HDA_DATAREGS, (cylinder >> 8) & 0xFF);
  _out(HDA_DATAREGS + 1, cylinder & 0xFF);
  _out(HDA_DATAREGS + 2, (sector >> 8) & 0xFF);
  _out(HDA_DATAREGS + 3, sector & 0xFF);
  _out(HDA_CMDREG, CMD_SEEK);
  yield();
  sem_up(semaphore_disque);
}

void check_hda() {
  int sector_size, max_sector, max_cylinder;

  _out(HDA_CMDREG, CMD_DSKINFO);

  max_cylinder = _in(HDA_DATAREGS) << 8;
  max_cylinder += _in(HDA_DATAREGS + 1);

  if(max_cylinder != MAX_CYLINDER) {
    printf("Max Cylinder is %d. Please RECOMPILE with correct Max cylinder\n", max_cylinder);
    exit(EXIT_FAILURE);
  }

  max_sector = _in(HDA_DATAREGS + 2) << 8;
  max_sector += _in(HDA_DATAREGS + 3);
  if(max_sector != MAX_SECTOR) {
    printf("Max sector is %d. Please RECOMPILE with correct Max sector\n", max_sector);
    exit(EXIT_FAILURE);
  }

  sector_size = _in(HDA_DATAREGS+4) << 8;
  sector_size += _in(HDA_DATAREGS+5);
  if(sector_size != SECTOR_SIZE) {
    printf("Sector size is %d. Please RECOMPILE with correct sector size\n", sector_size);
    exit(EXIT_FAILURE);
  }



}
