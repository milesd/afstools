/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include "node.h"

int ln(register char *from, register char *to)
{
  if (!silent)
    message("ln %s %s",from,to);
  if (!lazy && link(from,to) < 0)
    {
      message("ln %s %s; %m",from,to);
      return -1;
    }
  return 0;
}
