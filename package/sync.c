/*
 *	package by Russell J. Yount
 *	(C) Copyright 10/12/86 by Carnegie Mellon University
 */

#include "node.h"

int xsync()
{
  if (!silent)
    message("sync");
  if (!lazy && sync() < 0)
    {
      message("sync; %m");
      return -1;
    }
  return 0;
}
