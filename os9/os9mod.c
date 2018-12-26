#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "os9.h"


u_char *os9_string(u_char *string);
void ident(OS9_MODULE_t *mod);
void usage(void);
long pos;

static char *types[16] = {
    "???", "Prog", "Subr", "Multi", "Data", "USR 5", "USR 6", "USR 7", 
    "USR 8", "USR 9", "USR A", "USR B", "System", "File Manager",
    "Device Driver", "Device Descriptor"
};
  
static char *langs[16] = {
  "Data", "6809 Obj", "Basic09 I-Code", "Pascal P-Code", "C I-Code",
  "Cobol I-Code", "Fortran I-Code", "6309 Obj", "???", "???", "???",
  "???", "???", "???", "???", "???"
};

int offset = 0;

int main(int argc, char **argv)
{
  char *filename = NULL;
  FILE *fp;
  u_char buffer[65536];		/* OS9 Module can't be larger than this */
  OS9_MODULE_t *mod = (OS9_MODULE_t *) buffer;
  int i=0, j;
  int flag = 0;

  argv++;			/* skip my name */

  if (argc == 1)
    usage();

  while ((argc >= 2) && (*argv[0] == '-')) {
    if (*(argv[0] + 1) == 's') {
      argc--;
      flag = 1;
    } else if (*(argv[0] + 1) == 'o') {
      argc--; argc--;
      argv++;
      offset = strtol(argv[0],(char**)0,0);
    } else
      usage();
    argv++;
  }
    

  while (argc-- > 1) {
    if (*argv==0) return 0;
    filename = *(argv++);
    
    if ((fp = fopen(filename,"rb")) == NULL) {
      fprintf(stderr, "Error opening file %s: %s\n",
	      filename, strerror(errno));
      return 1;
    }
    
    while (!feof(fp)) {

      if (flag) {
          int c;
          while( !feof(fp) && (c = fgetc(fp)) != 0x87 );
          ungetc(c, fp);
      }

      pos = ftell(fp);
      if (fread(buffer, OS9_HEADER_SIZE, 1, fp) != 1) {
	if (feof(fp))
	  break;
	else {
	  fprintf(stderr, "Error reading file %s: %s\n",
		  filename, strerror(errno));
	  return 1;
	}
      }
      
      if ((mod->id[0] != OS9_ID0) && (mod->id[1] != OS9_ID1)) {
	fprintf(stderr,"Not OS9 module, skipping.\n");
	return 1;
      }
      
      if ((i = os9_header(mod))!=0xff) {
	fprintf(stderr, "Bad header parity.  Expected 0xFF, got 0x%02X\n", i);
	return 1;
      }
      
      i = INT(mod->size) - OS9_HEADER_SIZE;
      if ((j = fread(buffer + OS9_HEADER_SIZE, 1, i, fp)) != i) {
	fprintf(stderr,"Module short.  Expected 0x%04X, got 0x%04X\n",
		i + OS9_HEADER_SIZE, j + OS9_HEADER_SIZE);
	return 1;
      }
      ident(mod);
    }
    fclose(fp);
  }
  return 0;
    
}

void ident(OS9_MODULE_t *mod)
{
  int i, j;
  u_char *name, *ptr, tmp, *buffer = (u_char *) mod;

  i = INT(mod->name);
  j = INT(mod->size);
  name = os9_string(&buffer[i]);
  printf("Offset : 0x%04lx\n", pos + offset);
  printf("Header for : %s\n", name);
  printf("Module size: $%X  #%d\n", j, j);
  ptr = &buffer[j - 3];
  printf("Module CRC : $%02X%02X%02X (%s)\n", ptr[0], ptr[1], ptr[2],
	   os9_crc(mod) ? "Good" : "Bad" );
  printf("Hdr parity : $%02X\n", mod->parity);

  switch ((mod->tyla & TYPE_MASK) >> 4)
    {

    case Drivr:
    case Prgrm:
      i = INT(mod->data.program.exec);
      printf("Exec. off  : $%04X  #%d\n", i, i);
      i = INT(mod->data.program.mem);
      printf("Data size  : $%04X  #%d\n", i, i);
      break;
      
    case Devic:
      printf("File Mgr   : %s\n",
	     os9_string(&buffer[INT(mod->data.descriptor.fmgr)]));
      printf("Driver     : %s\n",
	     os9_string(&buffer[INT(mod->data.descriptor.driver)]));
      break;
      
    case NULL_TYPE:
    case TYPE_6:
    case TYPE_7:
    case TYPE_8:
    case TYPE_9:
    case TYPE_A:
    case TYPE_B:
    case Systm:
      break;
    }
  



  tmp = buffer[i + strlen((const char *)name)];
  printf("Edition    : $%02X  #%d\n", tmp, tmp);
  printf("Ty/La At/Rv: $%02X $%02x\n", mod->tyla, mod->atrv);
  printf("%s mod, ", types[(mod->tyla & TYPE_MASK) >> 4]);
  printf("%s, ", langs[mod->tyla & LANG_MASK]);
  printf("%s, %s\n", (mod->atrv & ReEnt) ? "re-ent" : "non-share",
	 (mod->atrv & Modprot) ? "R/W" : "R/O" );
  printf("\n");
}

u_char *os9_string(u_char *string)
{
  static u_char cleaned[80];	/* strings shouldn't be longer than this */
  u_char *ptr = cleaned;
  int i = 0;

  while (((*(ptr++) = *(string++)) < 0x7f) &&
	 (i++ < sizeof(cleaned) - 1))
    ;

  *(ptr - 1) &= 0x7f;
  *ptr = '\0';
  return cleaned;
}
void usage(void)
{
  printf("Usage: os9mod [-s] [-o offset] file [ file ... ]\n");
  printf("Performs an OS-9: 6809 'ident' on the specified files.\n");
  printf("  -s : skip to valid module\n");
  printf("  -o : offset \n\n");
  exit(0);
}
