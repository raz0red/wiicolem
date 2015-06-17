/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       INIFile.h                         **/
/**                                                         **/
/** This file contains declarations for .INI file parser.   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef INIFILE_H
#define INIFILE_H
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILENAME 256
#define MAX_SECNAME  64
#define MAX_VARNAME  64
#define MAX_VALUE    256

typedef struct
{
  char Name[MAX_VARNAME];
  char Value[MAX_VALUE];
} INIVar;

typedef struct
{
  char Name[MAX_SECNAME];
  INIVar *Vars;
  int MaxVars;
  int CurVars;
} INISection;

typedef struct
{
  char Name[MAX_FILENAME];
  INISection *Secs;
  int MaxSecs;
  int CurSecs;  
} INIFile;

/** InitINI() ************************************************/
/** Initialize INIFile structure clearing all fields.       **/
/*************************************************************/
void InitINI(INIFile *File);

/** TrashINI() ***********************************************/
/** Free all resources allocated in an INIFile structure.   **/
/*************************************************************/
void TrashINI(INIFile *File);

/** LoadINI() ************************************************/
/** Load .INI file into a given INIFile structure. Returns  **/
/** number of variables loaded or 0 on failure.             **/
/*************************************************************/
int LoadINI(INIFile *File,const char *FileName);

/** SaveINI() ************************************************/
/** Save given INIFile structure into an .INI file. Returns **/
/** number of variables saved or 0 on failure.              **/
/*************************************************************/
int SaveINI(INIFile *File,const char *FileName);

/** INIGetInteger() ******************************************/
/** Get integer setting by name or return Default if not    **/
/** found.                                                  **/
/*************************************************************/
unsigned int INIGetInteger(INIFile *File,const char *Section,const char *Name,unsigned int Default);

/** INISetInteger() ******************************************/
/** Set integer setting to a given value.                   **/
/*************************************************************/
void INISetInteger(INIFile *File,const char *Section,const char *Name,unsigned int Value);

/** INIGetFloat() ********************************************/
/** Get floating point setting by name or return Default if **/
/** not found.                                              **/
/*************************************************************/
double INIGetFloat(INIFile *File,const char *Section,const char *Name,double Default);

/** INISetFloat() ********************************************/
/** Set floating point setting to a given value.            **/
/*************************************************************/
void INISetFloat(INIFile *File,const char *Section,const char *Name,double Value);

/** INIGetString() *******************************************/
/** Get string setting by name.                             **/
/*************************************************************/
char *INIGetString(INIFile *File,const char *Section,const char *Name,char *Value,int MaxChars);

/** INISetString() *******************************************/
/** Set string setting to a given value.                    **/
/*************************************************************/
void INISetString(INIFile *File,const char *Section,const char *Name,const char *Value);

/** INIHasVar() **********************************************/
/** Return 1 if File has a given variable, 0 otherwise. Set **/
/** Name=0 when looking for a section rather than variable. **/
/*************************************************************/
int INIHasVar(INIFile *File,const char *Section,const char *Name);

/** INIDeleteVar() *******************************************/
/** Delete a variable or a section (when Name=0). Returns 1 **/
/** on successful deletion, 0 otherwise.                    **/
/*************************************************************/
int INIDeleteVar(INIFile *File,const char *Section,const char *Name);

#ifdef __cplusplus
}
#endif
#endif /* INIFILE_H */
