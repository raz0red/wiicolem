/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         NetUnix.c                       **/
/**                                                         **/
/** This file contains standard communication routines for  **/
/** Unix using TCP/IP via sockets.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1997-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"
#include "NetPlay.h"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

static int IsServer = 0;
static int Socket   = -1;
static int Blocking = 1;

/** NETConnected() *******************************************/
/** Returns NET_SERVER, NET_CLIENT, or NET_OFF.             **/
/*************************************************************/
int NETConnected(void)
{ return(Socket<0? NET_OFF:IsServer? NET_SERVER:NET_CLIENT); }

/** NETMyName() **********************************************/
/** Returns local hostname/address or 0 on failure.         **/
/*************************************************************/
const char *NETMyName(char *Buffer,int MaxChars)
{
  struct hostent *Host;

  /* Have to have enough characters */
  if(MaxChars<16) return(0);
  /* Get local hostname */
  gethostname(Buffer,MaxChars);
  /* Look up address */
  if(!(Host=gethostbyname(Buffer))) return(0);
  /* Must have an address */
  if(!Host->h_addr_list||!Host->h_addr_list[0]) return(0);
  /* Copy address */
  sprintf(Buffer,"%d.%d.%d.%d",
    Host->h_addr_list[0][0],
    Host->h_addr_list[0][1],
    Host->h_addr_list[0][2],
    Host->h_addr_list[0][3]
  );
  return(Buffer);
}

/** NETConnect() *********************************************/
/** Connects to Server:Port. When Host=0, becomes a server, **/
/** waits at for an incoming request, and connects. Returns **/
/** the NetPlay connection status, like NETConnected().     **/
/*************************************************************/
int NETConnect(const char *Server,unsigned short Port)
{
  struct sockaddr_in Addr;
  struct hostent *Host;
  int AddrLength,LSocket;
  unsigned long J;

  /* Close existing network connection */
  NETClose();

  /* Clear the address structure */
  memset(&Addr,0,sizeof(Addr));

  /* If the server name is given, we first try to */
  /* connect as a client.                         */
  if(Server)
  {
    /* Look up server address */
    if(!(Host=gethostbyname(Server))) return(NET_OFF);

    /* Set fields of the address structure */
    memcpy(&Addr.sin_addr,Host->h_addr,Host->h_length);
    Addr.sin_family = AF_INET;
    Addr.sin_port   = htons(Port);

    /* Create a socket */
    if((Socket=socket(AF_INET,SOCK_STREAM,0))<0) return(NET_OFF);

    /* Connecting... */
    if(connect(Socket,(struct sockaddr *)&Addr,sizeof(Addr))>=0)
    {
      /* Make communication socket blocking/non-blocking */
      J=!Blocking;
      if(ioctl(Socket,FIONBIO,&J)<0)
      { close(Socket);Socket=-1;return(NET_OFF); }
      /* Succesfully connected as client */
      IsServer=0;
      return(NET_CLIENT);
    }

    /* Failed to connect as a client */
    close(Socket);
  }

  /* Connection as client either failed or hasn't */
  /* been attempted at all. Becoming a server and */
  /* waiting for connection request.              */

  /* Set fields of the address structure */
  Addr.sin_addr.s_addr = htonl(INADDR_ANY);
  Addr.sin_family      = AF_INET;
  Addr.sin_port        = htons(Port);

  /* Create a listening socket */
  if((LSocket=socket(AF_INET,SOCK_STREAM,0))<0) return(NET_OFF);

  /* Bind listening socket */
  if(bind(LSocket,(struct sockaddr *)&Addr,sizeof(Addr))<0)
  { close(LSocket);return(NET_OFF); }

  /* Make listening socket non-blocking */
  J=1;
  if(ioctl(LSocket,FIONBIO,&J)<0)
  { close(LSocket);return(NET_OFF); }

  /* Listen for one client */
  if(listen(LSocket,1)<0)
  { close(LSocket);return(NET_OFF); }

  /* Accepting calls... */
  AddrLength=sizeof(Addr);
  GetKey();
  do
  {
#ifdef MAEMO
    GTKProcessEvents();
#else
    X11ProcessEvents();
#endif
    Socket=accept(LSocket,(struct sockaddr *)&Addr,&AddrLength);
    if(Socket==EWOULDBLOCK) break;
  }
  while((Socket<0)&&VideoImg&&!GetKey());
  close(LSocket);

  /* Client failed to connect */
  if(Socket<0) return(NET_OFF);

  /* Make communication socket blocking/non-blocking */
  J=!Blocking;
  if(ioctl(Socket,FIONBIO,&J)<0)
  { close(Socket);Socket=-1;return(NET_OFF); }

  /* Client connected succesfully */
  IsServer=1;
  return(NET_SERVER);
}

/** NETClose() ***********************************************/
/** Closes connection open with NETConnect().               **/
/*************************************************************/
void NETClose(void)
{
  if(Socket>=0) close(Socket);
  Socket   = -1;
  IsServer = 0;
}

/** NETBlock() ***********************************************/
/** Makes NETSend()/NETRecv() blocking or not blocking.     **/
/*************************************************************/
int NETBlock(int Switch)
{
  unsigned long J;

  /* Toggle blocking if requested */
  if(Switch==NET_TOGGLE) Switch=!Blocking;

  /* If switching blocking... */
  if((Switch==NET_OFF)||(Switch==NET_ON))
  {
    J=!Switch;
    if((Socket<0)||(ioctl(Socket,FIONBIO,&J)>=0)) Blocking=Switch;
  }

  /* Return blocking state */
  return(Blocking);
}

/** NETSend() ************************************************/
/** Send N bytes. Returns number of bytes sent or 0.        **/
/*************************************************************/
int NETSend(const char *Out,int N)
{
  int J,I;

  /* Have to have a socket */
  if(Socket<0) return(0);

  /* Send data */
  for(I=J=N;(J>=0)&&I;)
  {
    J=send(Socket,Out,I,0);
    if(J>0) { Out+=J;I-=J; }
  }

  /* Return number of bytes sent */
  return(N-I);
}

/** NETRecv() ************************************************/
/** Receive N bytes. Returns number of bytes received or 0. **/
/*************************************************************/
int NETRecv(char *In,int N)
{
  int J,I;

  /* Have to have a socket */
  if(Socket<0) return(0);

  /* Receive data */
  for(I=J=N;(J>=0)&&I;)
  {
    J=recv(Socket,In,I,0);
    if(J>0) { In+=J;I-=J; }
  }

  /* Return number of bytes received */
  return(N-I);
}
