/********************************************************************************************************************************************

Note
-------------
The version of lp_solve included in this repository has been modified as follows:
1. The original .c files have been modified to .cpp files to facilitate the use of c++ std library functions for abs, fabs, sqrt, etc.
2. The lp_solve specific file modifications can be found at https://github.com/NatLabRockies/ssc/commits/patch/lpsolve

The original version of lp_solve can be found at https://sourceforge.net/projects/lpsolve/

********************************************************************************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "lp_lib.h"

#include "ini.h"

FILE *ini_create(char *filename)
{
  FILE *fp;

  fp = fopen(filename, "w");

  return(fp);
}

FILE *ini_open(char *filename)
{
  FILE *fp;

  fp = fopen(filename, "r");

  return(fp);
}

void ini_writecomment(FILE *fp, char *comment)
{
  fprintf(fp, "; %s\n", comment);
}

void ini_writeheader(FILE *fp, char *header, int addnewline)
{
  if((addnewline) && (ftell(fp) > 0))
    fputs("\n", fp);
  fprintf(fp, "[%s]\n", header);
}

void ini_writedata(FILE *fp, char *name, char *data)
{
  if(name != NULL)
    fprintf(fp, "%s=%s\n", name, data);
  else
    fprintf(fp, "%s\n", data);
}

int ini_readdata(FILE *fp, char *data, int szdata, int withcomment)
{
  int l;
  char *ptr;

  if(fgets(data, szdata, fp) == NULL)
    return(0);

  if(!withcomment) {
    ptr = strchr(data, ';');
    if(ptr != NULL)
      *ptr = 0;
  }

  l = (int) strlen(data);
  while((l > 0) && (isspace(data[l - 1])))
    l--;
  data[l] = 0;
  if((l >= 2) && (data[0] == '[') && (data[l - 1] == ']')) {
    memcpy(data, data + 1, l - 2);
    data[l - 2] = 0;
    return(1);
  }
  return(2);
}

void ini_close(FILE *fp)
{
  fclose(fp);
}
