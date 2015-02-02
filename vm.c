/* ------------------------------
   $Id: vm-skel.c,v 1.1 2002/10/21 07:16:29 marquet Exp $
   ------------------------------------------------------------

   Volume manager skeleton.
   Philippe Marquet, october 2002

   1- you must complete the NYI (not yet implemented) functions
   2- you may add commands (format, etc.)
   
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drive.h"
#include "mbr.h"
#include "colors.h"
#include "mount.h"
#include "tools.h"
/* ------------------------------
   command list
   ------------------------------------------------------------*/
struct _cmd {
    char *name;
    void (*fun) (struct _cmd *c);
    char *comment;
};

static void list(struct _cmd *c);
static void new(struct _cmd *c);
static void del(struct _cmd *c);
static void help(struct _cmd *c) ;
static void save(struct _cmd *c);
static void quit(struct _cmd *c);
static void xit(struct _cmd *c);
static void none(struct _cmd *c) ;

static struct _cmd commands [] = {
    {"list", list, 	"display the partition table"},
    {"new",  new,	"create a new partition"},
    {"del",  del,	"delete a partition"},
    {"save", save,	"save the MBR"},
    {"quit", quit,	"save the MBR and quit"},
    {"exit", xit,	"exit (without saving)"},
    {"help", help,	"display this help"},
    {0, none, 		"unknown command, try help"}
} ;

/* ------------------------------
   dialog and execute 
   ------------------------------------------------------------*/

static void
execute(const char *name)
{
    struct _cmd *c = commands; 
  
    while (c->name && strcmp (name, c->name))
	c++;
    (*c->fun)(c);
}

static void
loop(void)
{
    char name[64];
    
    while (printf("> "), scanf("%62s", name) == 1)
	execute(name) ;
}

/* ------------------------------
   command execution 
   ------------------------------------------------------------*/
static void
list(struct _cmd *c)
{
  int i, cylindreTmp, secteurTmp;
  if(mbr.mbr_n_vol == 0)
    printf("Ce volume n'a pas encore de partitions.\n");
  else{
    printf(MAGENTA "%2s - %8s - %8s - %7s - %7s - %12s - %4s\n" RESET, "id",  "first c.", "first s.", "last c.", "last s.", "nbr secteurs", "type");

    for(i = 0; i < mbr.mbr_n_vol; i++) {
      cylindreTmp = mbr.mbr_vol[i].vol_first_cylinder + (mbr.mbr_vol[i].vol_n_sector / MAX_SECTOR);
      secteurTmp = (mbr.mbr_vol[i].vol_first_sector + mbr.mbr_vol[i].vol_n_sector) % MAX_SECTOR;
      printf("%2d   %8d   %8d   %7d   %7d   %12d   ", i, mbr.mbr_vol[i].vol_first_cylinder, mbr.mbr_vol[i].vol_first_sector, cylindreTmp, secteurTmp, mbr.mbr_vol[i].vol_n_sector);
      switch(mbr.mbr_vol[i].vol_type) {
      case VOLT_PR:
	printf("%4s", "primaire");
	break;
      case VOLT_SND:
	printf("%4s", "secondaire");
	break;
      case VOLT_OTHER:
	printf("%4s", "autre");
	break;
      default:
	printf("%4s", "inconnu");
	break;
      }
      printf("\n");
    }
  }
}

static void
new(struct _cmd *c)
{
  int cylindre, sector, n_sector, vol_type;
  int i, debutNvllePartition, finNvllePartition, debutTmp, finTmp;
  int partitionValide = 1; /* 1 = valide. 0 = pas valide */
  if(mbr.mbr_n_vol >= MAX_VOL)
    printf(BOLDRED "Ce disque contient le maximum de partitions autorisé (%d). Vous ne pouvez donc pas en créer une autre\n" RESET, MAX_VOL);
  else {
    /* recupération des données de l'utilisateur */
    printf("Veuillez donner le premier " BOLDWHITE "cylindre " RESET "de la nouvelle partition\n");
    scanf("%d", &cylindre);
    printf("Veuillez donner le premier " BOLDWHITE "secteur " RESET "de la nouvelle partition\n");
    scanf("%d", &sector);
    
    printf("Veuillez donner la " BOLDWHITE "taille " RESET "de la partition (en nbr de secteur)\n");
    scanf("%d", &n_sector);
    printf("Veuillez donner le " BOLDWHITE "type " RESET "de la partition\n0 pour primaire; 1 pour secondaire; 2 pour autre\n");
    scanf("%d", &vol_type);
    /* FIN recupération des données de l'utilisateur */

    /* verification des données */
    /* verifions que l'endroit ou on veut ecrire n'est pas deja dans une partition */
    /* pour se faire, on va tranformer les coordonnées de cylindre/sector/nb sector de la partition à crééer en un nombre (secteur) et le comparer à ceux du debut et de la fin de chaque partitions. Si la nouvelle partition n'est pas créée dans une partition dejà existante, tout va bien. Sinon, on empeche l'utilisateur de la créer. */
    debutNvllePartition = (cylindre * MAX_SECTOR) + sector;
    finNvllePartition = debutNvllePartition + n_sector;
    for(i = 0; i < mbr.mbr_n_vol; i++) {
      /* on va recuperer les coordonnee de chaque debut / fin de chaque partitions */
      debutTmp = (mbr.mbr_vol[i].vol_first_cylinder * MAX_SECTOR) + mbr.mbr_vol[i].vol_first_sector;
      finTmp = debutTmp + mbr.mbr_vol[i].vol_n_sector;
      /* si on essait d'ecrire dans les données déjà reservé à une autre partition, on ne laisse pas faire */
      /* if(debutNvllePartition >= debutTmp || finNvllePartition <= finTmp) { */
      if(debutNvllePartition <= finTmp && finNvllePartition >= finTmp) {
	printf(BOLDRED "La partition que vous voulez ecrire overlap une partition deja existante. Veuillez changer vos paramètres\n" RESET);
	partitionValide = 0;
	break;
      }
      else if(finNvllePartition >= debutTmp && finNvllePartition <= finTmp/* || finNvllePartition >= debutTmp */) {
	printf(BOLDRED "La partition que vous voulez ecrire overlap une partition deja existante. Veuillez changer vos paramètres\n" RESET);
	partitionValide = 0;
	break;
      }
    }

    /* verifions que le CYLINDRE dans lequel l'utilisateur veut ecrire existe */
    if(cylindre < 0 || cylindre > MAX_CYLINDER)
      printf(BOLDRED "Le disque ne contient pas ce cylindre. \nCe disque contient des cilyndres allant de 0 à %d\n" RESET, MAX_CYLINDER);
    /* verifions que le SECTEUR dans lequel l'utilisateur veut ecrire existe */
    else if(sector < 0 || sector > MAX_SECTOR)
      printf(BOLDRED "Ce disque utilise un maximum de %d secteurs par cylindre. Vous lui en demandez trop !\n" RESET, MAX_SECTOR);
    /* verifions que l'utilisateur ne veut pas creer une PARTITION TROP GRANDE */
    else if(cylindre + ((sector + n_sector) / MAX_SECTOR) > MAX_CYLINDER)
      printf(BOLDRED "Le disque n'est pas assez grand pour acceuillir votre partition. Veuillez revoir les données entrées\n" RESET);
    /* verifions que l'utilisateur n'essait pas d'ECRIRE DANS LE MBR*/
    else if(cylindre == 0 && sector == 0) {
      printf(BOLDRED "Le cylindre 0, secteur 0 est un emplacement reserve. Vous ne pouvez ecrire dedant.\n" RESET);
    }
    /* FIN verification des données... enfin presque, j'en fait une derniere en dessous ! */    
      
    else if(partitionValide == 1) { /* si on peut ecrire la partition */
      mbr.mbr_vol[mbr.mbr_n_vol].vol_first_cylinder = cylindre;
      mbr.mbr_vol[mbr.mbr_n_vol].vol_first_sector = sector;
      mbr.mbr_vol[mbr.mbr_n_vol].vol_n_sector = n_sector;
	
      switch(vol_type) {
      case 0:
	mbr.mbr_vol[mbr.mbr_n_vol].vol_type = VOLT_PR;
	break;
      case 1:
	mbr.mbr_vol[mbr.mbr_n_vol].vol_type = VOLT_SND;
	break;
      case 2:
	mbr.mbr_vol[mbr.mbr_n_vol].vol_type = VOLT_OTHER;
	break;
      default:
	break;
      }
      mbr.mbr_n_vol++;
      if(DEBUG) {
	printDebug(FNNAME, "");
	printf("mbr.mbr_vol[mbr.mbr_n_vol].vol_first_cylinder = %d\n", mbr.mbr_vol[mbr.mbr_n_vol - 1].vol_first_cylinder);
	printf("mbr.mbr_vol[mbr.mbr_n_vol].vol_first_sector = %d\n", mbr.mbr_vol[mbr.mbr_n_vol - 1].vol_first_sector);
	printf("mbr.mbr_vol[mbr.mbr_n_vol].vol_n_sector = %d\n", mbr.mbr_vol[mbr.mbr_n_vol - 1].vol_n_sector);
      }
    }
  }
}

static void
del(struct _cmd *c)
{
  int partition, i;
  printf("Numero de la partition à supprimer\n");
  scanf("%d", &partition);
  if(mbr.mbr_n_vol == 0) {
        printf("Aucune partition n'a encore été créée.\n");
  }
    
  else if(partition > mbr.mbr_n_vol - 1)
    printf("Cette partition n'existe pas\n");
  /* ici, on s'assure que l'ordre des partitions est toujours bon... c.a.d. que les partitions présente après celle que nous avons supprimé sont replacé dans mbr_vol les une à la suite des autres */ 
  else {
    for(i = partition; i < mbr.mbr_n_vol; i++) {
      memcpy(&mbr.mbr_vol[i], &mbr.mbr_vol[i + 1], sizeof(struct vol_s));
    }
    mbr.mbr_n_vol--;
  }
}

static void
save(struct _cmd *c)
{
  save_mbr();
  printf(GREEN "mbr saved (if needed :)\n" RESET);
}

static void
quit(struct _cmd *c)
{
  save_mbr();
  printf(GREEN "mbr saved (if needed :). Exiting program.\n" RESET);
  exit(EXIT_SUCCESS);
}

static void
do_xit()
{
    exit(EXIT_SUCCESS);
}

static void
xit(struct _cmd *dummy)
{
    do_xit(); 
}

static void
help(struct _cmd *dummy)
{
    struct _cmd *c = commands;
  
    for (; c->name; c++) 
	printf ("%s\t-- %s\n", c->name, c->comment);
}

static void
none(struct _cmd *c)
{
    printf ("%s\n", c->comment) ;
}

int
main(int argc, char **argv)
{
    /* dialog with user */ 
  if(DEBUG)
    printf(BOLDGREEN"[mount sequence]"RESET GREEN" init master\n");


  init_master();
  printf(BOLDGREEN"[mount sequence]"RESET GREEN" load mbr\n");
  load_mbr();
  /* mount(); */
  loop();
  /* abnormal end of dialog (cause EOF for xample) */
  do_xit();
  /* make gcc -W happy */
  exit(EXIT_SUCCESS);
}
