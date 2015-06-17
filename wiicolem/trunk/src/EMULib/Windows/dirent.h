#ifndef DIRENT_H
#define DIRENT_H

#include <Windows.h>

struct dirent
{
  char d_name[MAX_PATH];
};

typedef struct {
  struct dirent DE; 
  WIN32_FIND_DATA FD;
  HANDLE Handle;
  char *Path;
} DIR;


DIR *opendir(const char *DirName);
struct dirent *readdir(DIR *D);
int closedir(DIR *D);
void rewinddir(DIR *D);

#endif /* DIRENT_H */
