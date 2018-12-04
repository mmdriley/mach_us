/*
 * util.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.5 $
 * $Date: 1993/02/01 23:59:57 $
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

/* 
 * System prototypes
 */
void *	malloc( unsigned );


char *
xerox(char *str)
{
  char *temp;
  int len;

  assert(str);
  len = strlen(str) + 2;
  temp = (char *) malloc(len);
  strcpy(temp, str);
  return (temp);
}


char *
join(char *str1, char *str2)
{
  char *temp;
  int len;

  if (!str1)
    return (str2);
  if (!str2)
    return (str1);

  len = strlen(str1) + strlen(str2) + 2;
  temp = (char *) malloc(len);
  strcpy(temp, str1);
  strcat(temp, str2);
  return (temp);
}
