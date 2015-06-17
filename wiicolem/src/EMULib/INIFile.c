/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       INIFile.c                         **/
/**                                                         **/
/** This file contains the .INI file parser.                **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "INIFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __inline char *SkipWS(char *P)
{
  while(*P&&(*P<=' ')) ++P;
  return(P);
}

/** InitINI() ************************************************/
/** Initialize INIFile structure clearing all fields.       **/
/*************************************************************/
void InitINI(INIFile *File)
{
  // Reset all fields
  File->Secs    = 0;
  File->CurSecs = 0;
  File->MaxSecs = 0;
}

/** TrashINI() ***********************************************/
/** Free all resources allocated in an INIFile structure.   **/
/*************************************************************/
void TrashINI(INIFile *File)
{
  int J;

  // Free all resources
  if(File->Secs)
  {
    for(J=0;J<File->CurSecs;++J)
      if(File->Secs[J].Vars) free(File->Secs[J].Vars);
    free(File->Secs);
  }

  // Reset all fields
  File->Secs    = 0;
  File->CurSecs = 0;
  File->MaxSecs = 0;
}

/** LoadINI() ************************************************/
/** Load .INI file into a given INIFile structure. Returns  **/
/** number of variables loaded or 0 on failure.             **/
/*************************************************************/
int LoadINI(INIFile *File,const char *FileName)
{
  char S[256],SName[256],VName[256];
  int J,Count;
  char *P;
  FILE *F;

  // Open file
  F=fopen(FileName,"rb");
  if(!F) return(0);

  // Read file line by line
  for(Count=0;fgets(S,sizeof(S),F);)
  {
    // Skip white space
    P=SkipWS(S);

    // If a new section starts...
    if(*P=='[')
    {
      // Skip white space
      P=SkipWS(P+1);
      // Copy section name
      for(J=0;*P&&(*P!=']');SName[J++]=*P++);
      // Trim white space
      while(J&&(SName[J-1]<=' ')) --J;
      // Terminate section name
      SName[J]='\0'; 
    }
    else
      if(SName[0])
      {
        //
        // Variable (but only if we have a valid section name)
        //

        // Copy variable name
        for(J=0;*P&&(*P!='=');VName[J++]=*P++);
        // Trim white space
        while(J&&(VName[J-1]<=' ')) --J;
        // Terminate variable name
        VName[J]='\0'; 

        // Only if we have got a variable name and an '=' sign...
        if(VName[0]&&(*P=='='))
        {
          // Skip white space
          P=SkipWS(P+1);
          // Copy variable value
          for(J=0;*P;S[J++]=*P++);
          // Trim whitespace
          while(J&&(S[J-1]<=' ')) --J;
          // Terminate variable value
          S[J]='\0';
          // If we have got a value, set it!
          INISetString(File,SName,VName,S);
          ++Count;
        }
      }
  }

  // Done reading file 
  fclose(F); 
  return(Count);
}

/** SaveINI() ************************************************/
/** Save given INIFile structure into an .INI file. Returns **/
/** number of variables saved or 0 on failure.              **/
/*************************************************************/
int SaveINI(INIFile *File,const char *FileName)
{
  int Count,J,I;
  FILE *F;

  // No sections -> nothing to write
  if(!File->Secs) return(0);

  // Open file for writing 
  F=fopen(FileName,"wb");
  if(!F) return(0);

  // For each section...
  for(J=Count=0;J<File->CurSecs;++J)
  {
    // Write section name
    fprintf(F,"[%s]\n",File->Secs[J].Name);
    // For each variable, write its name and value
    // No variables -> nothing to write
    if(File->Secs[J].Vars)
      for(I=0;I<File->Secs[J].CurVars;++I,++Count)
        fprintf(F,"%s = %s\n",File->Secs[J].Vars[I].Name,File->Secs[J].Vars[I].Value);
  }

  // Done writing file
  fclose(F);
  return(Count);  
}

/** INIGetInteger() ******************************************/
/** Get integer setting by name or return Default if not    **/
/** found.                                                  **/
/*************************************************************/
unsigned int INIGetInteger(INIFile *File,const char *Section,const char *Name,unsigned int Default)
{
  char S[MAX_VALUE];
  return(INIGetString(File,Section,Name,S,sizeof(S))? atol(S):Default);
}

/** INISetInteger() ******************************************/
/** Set integer setting to a given value.                   **/
/*************************************************************/
void INISetInteger(INIFile *File,const char *Section,const char *Name,unsigned int Value)
{
  char S[MAX_VALUE];
  sprintf(S,"%lu",Value);
  INISetString(File,Section,Name,S);
}

/** INIGetFloat() ********************************************/
/** Get floating point setting by name or return Default if **/
/** not found.                                              **/
/*************************************************************/
double INIGetFloat(INIFile *File,const char *Section,const char *Name,double Default)
{
  char S[MAX_VALUE];
  return(INIGetString(File,Section,Name,S,sizeof(S))? atof(S):Default);
}

/** INISetFloat() ********************************************/
/** Set floating point setting to a given value.            **/
/*************************************************************/
void INISetFloat(INIFile *File,const char *Section,const char *Name,double Value)
{
  char S[MAX_VALUE];
  sprintf(S,"%lf",Value);
  INISetString(File,Section,Name,S);
}

/** INIGetString() *******************************************/
/** Get string setting by name.                             **/
/*************************************************************/
char *INIGetString(INIFile *File,const char *Section,const char *Name,char *Value,int MaxChars)
{
  int J,I;

  // Need to have sections
  if(!File->Secs) return(0);
  // Scan through sections
  for(J=0;(J<File->CurSecs)&&strcmp(Section,File->Secs[J].Name);++J);
  if(J>=File->CurSecs) return(0);
  // Need to have variables
  if(!File->Secs[J].Vars) return(0);
  // Scan through variables
  for(I=0;(I<File->Secs[J].CurVars)&&strcmp(Name,File->Secs[J].Vars[I].Name);++I);
  if(I>=File->Secs[J].CurVars) return(0);
  // Copy value truncating as needed
  strncpy(Value,File->Secs[J].Vars[I].Value,MaxChars);
  Value[MaxChars-1]='\0';
  // Done
  return(Value);
}

/** INISetString() *******************************************/
/** Set string setting to a given value.                    **/
/*************************************************************/
void INISetString(INIFile *File,const char *Section,const char *Name,const char *Value)
{
  INISection *NewSecs;
  INIVar *NewVars;
  int I,J;

  // Look for section
  if(!File->Secs) J=0;
  else for(J=0;(J<File->CurSecs)&&strcmp(File->Secs[J].Name,Section);++J);

  // If section not found...
  if(J>=File->CurSecs)
  {
    // If need to expand section array...
    if(J>=File->MaxSecs)
    {
      J       = File->Secs? File->MaxSecs*2:4;
      NewSecs = (INISection *)malloc(J*sizeof(INISection));
      if(!NewSecs) return;
      if(File->Secs)
      {
        memcpy(NewSecs,File->Secs,File->CurSecs*sizeof(INISection));
        free(File->Secs);
      }
      File->Secs    = NewSecs;
      File->MaxSecs = J;
    }

    // Create a new section
    J = File->CurSecs++;
    strncpy(File->Secs[J].Name,Section,MAX_SECNAME);
    File->Secs[J].Name[MAX_SECNAME-1]='\0';
    File->Secs[J].Vars    = 0;
    File->Secs[J].CurVars = 0;
    File->Secs[J].MaxVars = 0;
  }

  // Look for variable
  if(!File->Secs[J].Vars) I=0;
  else for(I=0;(I<File->Secs[J].CurVars)&&strcmp(File->Secs[J].Vars[I].Name,Name);++I);

  // If variable not found..
  if(I>=File->Secs[J].CurVars)
  {
    // If need to expand variable array...
    if(I>=File->Secs[J].MaxVars)
    {
      I       = File->Secs[J].Vars? File->Secs[J].MaxVars*2:4;
      NewVars = (INIVar *)malloc(I*sizeof(INIVar));
      if(!NewVars) return;
      if(File->Secs[J].Vars)
      {
        memcpy(NewVars,File->Secs[J].Vars,File->Secs[J].CurVars*sizeof(INIVar));
        free(File->Secs[J].Vars);
      }
      File->Secs[J].Vars    = NewVars;
      File->Secs[J].MaxVars = I;
    }

    // Create a new variable
    I = File->Secs[J].CurVars++;
    strncpy(File->Secs[J].Vars[I].Name,Name,MAX_VARNAME);
    File->Secs[J].Vars[I].Name[MAX_VARNAME-1]='\0';
  }

  // Set variable to a new value
  strncpy(File->Secs[J].Vars[I].Value,Value,MAX_VALUE);
  File->Secs[J].Vars[I].Value[MAX_VALUE-1]='\0';
}

/** INIHasVar() **********************************************/
/** Return 1 if File has a given variable, 0 otherwise. Set **/
/** Name=0 when looking for a section rather than variable. **/
/*************************************************************/
int INIHasVar(INIFile *File,const char *Section,const char *Name)
{
  int I,J;

  // Need to have sections
  if(!File->Secs) return(0);
  // Scan through sections
  for(J=0;(J<File->CurSecs)&&strcmp(Section,File->Secs[J].Name);++J);
  if(J>=File->CurSecs) return(0);

  // If no variable name given, check section only
  if(!Name) return(1);

  // Need to have variables
  if(!File->Secs[J].Vars) return(0);
  // Scan through variables
  for(I=0;(I<File->Secs[J].CurVars)&&strcmp(Name,File->Secs[J].Vars[I].Name);++I);
  // Return 1 if variable found, 0 otherwise
  return(I<File->Secs[J].CurVars);
}

/** INIDeleteVar() *******************************************/
/** Delete a variable or a section (when Name=0). Returns 1 **/
/** on successful deletion, 0 otherwise.                    **/
/*************************************************************/
int INIDeleteVar(INIFile *File,const char *Section,const char *Name)
{
  int I,J;

  // Need to have sections
  if(!File->Secs) return(0);
  // Scan through sections
  for(J=0;(J<File->CurSecs)&&strcmp(Section,File->Secs[J].Name);++J);
  if(J>=File->CurSecs) return(0);

  // If no variable name given...
  if(!Name)
  {
    // Delete whole section
    if(File->Secs[J].Vars) free(File->Secs[J].Vars);
    if(J<File->CurSecs-1)
      memcpy(
        &File->Secs[J],
        &File->Secs[J+1],
        (File->CurSecs-J-1)*sizeof(INISection)
      );
    // One section less
    --File->CurSecs;
    // Done
    return(1);
  }

  // Need to have variables
  if(!File->Secs[J].Vars) return(0);
  // Scan through variables
  for(I=0;(I<File->Secs[J].CurVars)&&strcmp(Name,File->Secs[J].Vars[I].Name);++I);
  if(I>=File->Secs[J].CurVars) return(0);

  // Deleting found variable
  if(I<File->Secs[J].CurVars-1)
    memcpy(
      &File->Secs[J].Vars[I],
      &File->Secs[J].Vars[I+1],
      (File->Secs[J].CurVars-I-1)*sizeof(INIVar)
    );
  // One variable less
  --File->Secs[J].CurVars;
  // Done
  return(1);
}

