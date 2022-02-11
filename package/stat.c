#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef __STDC__
int my_stat(char *path, struct stat *buf) 
#else
int my_stat(path, buf) 
char *path;
struct stat *buf;
#endif
{
  if (stat(path, buf) < 0)
    return stat(path, buf);
  return 0;
}

#ifdef __STDC__
int my_fstat(int fildes, struct stat *buf) 
#else
int my_fstat(fildes, buf) 
int fildes;
struct stat *buf;
#endif
{
  if (fstat(fildes, buf) < 0)
    return fstat(fildes, buf);
  return 0;
}

#ifdef __STDC__
int my_lstat(char *path, struct stat *buf) 
#else
int my_lstat(path, buf) 
char *path;
struct stat *buf;
#endif
{
  if (lstat(path, buf) < 0)
    return lstat(path, buf);
  return 0;
}
