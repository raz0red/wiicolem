#include "dirent.h"

DIR *opendir(const char *DirName)
{
  DIR *D;
  int J;

  // Must have non-empty name
  J=strlen(DirName);
  if(!J) return(0);
  // Allocate memory
  D=(DIR *)malloc(sizeof(DIR)+J+5);
  if(!D) return(0);
  D->Path=(char *)(D+1);
  // Add mask
  strcpy(D->Path,DirName);
  if(!strchr(D->Path,'*'))
  {
    if(D->Path[J-1]!='/') D->Path[J++]='/';
    strcpy(D->Path+J,"*.*");
  }
  // Do FindFirst()
  D->Handle=FindFirstFile(D->Path,&D->FD);
  if(D->Handle==INVALID_HANDLE_VALUE) { free(D);return(0); }
  // Done
  return(D);
}

struct dirent *readdir(DIR *D)
{
  // See if directory ended
  if(D->Handle==INVALID_HANDLE_VALUE) return(0);
  // Copy name
  strcpy(D->DE.d_name,D->FD.cFileName);
  // Get next name
  if(!FindNextFile(D->Handle,&D->FD))
  {
    FindClose(D->Handle);
    D->Handle=INVALID_HANDLE_VALUE;
  }
  // Done
  return(&D->DE);
}

int closedir(DIR *D)
{
  if(D)
  {
    if(D->Handle!=INVALID_HANDLE_VALUE) FindClose(D->Handle);
    free(D);
  }
  return(0);
}

void rewinddir(DIR *D)
{
  // Close current scan
  if(D->Handle!=INVALID_HANDLE_VALUE) FindClose(D->Handle);
  // Do FindFirst()
  D->Handle=FindFirstFile(D->Path,&D->FD);
}
