#include <stdio.h>

#include "os9.h"

int os9_crc(OS9_MODULE_t *mod)
{
  int i;
  u_char crc[3] = {0xff, 0xff, 0xff};
  u_char *ptr = (u_char *) mod;
  u_char a;

  for (i = 0; i < INT(mod->size); i++)
    {
      a = *(ptr++);

      a ^= crc[0];
      crc[0] = crc[1];
      crc[1] = crc[2];
      crc[1] ^= (a >> 7);
      crc[2] = (a << 1);
      crc[1] ^= (a >> 2);
      crc[2] ^= (a << 6);
      a ^= (a << 1);
      a ^= (a << 2);
      a ^= (a << 4);
      if (a & 0x80) {
	crc[0] ^= 0x80;
	crc[2] ^= 0x21;
      }
    }
  if ((crc[0] == OS9_CRC0) &&
      (crc[1] == OS9_CRC1) &&
      (crc[2] == OS9_CRC2))
    return 1;

  return 0;
}

int os9_header(OS9_MODULE_t *mod)
{
  u_char tmp = 0x00;
  u_char *ptr = (u_char *) mod;
  int i;
  
  for (i = 0; i < OS9_HEADER_SIZE; i++)
    tmp ^= *(ptr++);
  
  return tmp;
}
