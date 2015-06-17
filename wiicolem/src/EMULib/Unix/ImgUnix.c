/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        ImgUnix.c                        **/
/**                                                         **/
/** This file wraps Image.c image routines for multiple     **/
/** display depths (BPP8,BPP16,BPP32). Use it instead of    **/
/** Image.c wherever you have to support multiple depths.   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2008                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "EMULib.h"

#undef BPP8
#undef BPP16
#undef BPP24
#undef BPP32
#undef RMASK
#undef GMASK
#undef BMASK
#undef PIXEL

#define BPP8
#define pixel           unsigned char
#define GetColor        GetColor_8
#define IMGCopy         IMGCopy_8
#define IMGDrawRect     IMGDrawRect_8
#define IMGFillRect     IMGFillRect_8
#define CropImage       CropImage_8
#define ClearImage      ClearImage_8
#define ScaleImage      ScaleImage_8
#define TelevizeImage   TelevizeImage_8
#define SoftenImage     SoftenImage_8
#define Merge2          Merge2_8
#define Merge4          Merge4_8
#define PIXEL(R,G,B)    (pixel)(((7*(R)/255)<<5)|((7*(G)/255)<<2)|(3*(B)/255))
#define RMASK           0xE0
#define GMASK           0x1C
#define BMASK           0x03
#include "Image.c"
#undef pixel
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef TelevizeImage
#undef SoftenImage
#undef Merge2
#undef Merge4
#undef RMASK
#undef GMASK
#undef BMASK
#undef PIXEL
#undef BPP8

#define BPP16
#define pixel           unsigned short
#define GetColor        GetColor_16
#define IMGCopy         IMGCopy_16
#define IMGDrawRect     IMGDrawRect_16
#define IMGFillRect     IMGFillRect_16
#define CropImage       CropImage_16
#define ClearImage      ClearImage_16
#define ScaleImage      ScaleImage_16
#define TelevizeImage   TelevizeImage_16
#define SoftenImage     SoftenImage_16
#define Merge2          Merge2_16
#define Merge4          Merge4_16
#if defined(UNIX) || defined(S60) || defined(UIQ)
/* Symbian and Unix use true 16BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<11)|((63*(G)/255)<<5)|(31*(B)/255))
#define RMASK           0xF800
#define GMASK           0x07E0
#define BMASK           0x001F
#else
/* Other platforms use 15BPP color */
#define PIXEL(R,G,B)    (pixel)(((31*(R)/255)<<10)|((31*(G)/255)<<5)|(31*(B)/255))
#define RMASK           0x7C00
#define GMASK           0x03E0
#define BMASK           0x001F
#endif	 
#include "Image.c"
#undef pixel
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef TelevizeImage
#undef SoftenImage
#undef Merge2
#undef Merge4
#undef RMASK
#undef GMASK
#undef BMASK
#undef PIXEL
#undef BPP16

#define BPP32
#define pixel           unsigned int
#define GetColor        GetColor_32
#define IMGCopy         IMGCopy_32
#define IMGDrawRect     IMGDrawRect_32
#define IMGFillRect     IMGFillRect_32
#define CropImage       CropImage_32
#define ClearImage      ClearImage_32
#define ScaleImage      ScaleImage_32
#define TelevizeImage   TelevizeImage_32
#define SoftenImage     SoftenImage_32
#define Merge2          Merge2_32
#define Merge4          Merge4_32
#define PIXEL(R,G,B)    (pixel)(((int)R<<16)|((int)G<<8)|B)
#define RMASK           0xFF0000
#define GMASK           0x00FF00
#define BMASK           0x0000FF
#include "Image.c"
#undef pixel
#undef GetColor
#undef IMGCopy
#undef IMGDrawRect
#undef IMGFillRect
#undef CropImage
#undef ClearImage
#undef ScaleImage
#undef TelevizeImage
#undef SoftenImage
#undef Merge2
#undef Merge4
#undef RMASK
#undef GMASK
#undef BMASK
#undef PIXEL
#undef BPP32

/** GetColor() ***********************************************/
/** Return pixel corresponding to the given <R,G,B> value.  **/
/** This only works for non-palletized modes.               **/
/*************************************************************/
pixel GetColor(unsigned char R,unsigned char G,unsigned char B)
{
  if(VideoImg)
    switch(VideoImg->D)
    {
      case 8:  GetColor_8(R,G,B);break;
      case 16: GetColor_16(R,G,B);break;
      case 24:
      case 32: GetColor_32(R,G,B);break;
    }

  return(0);
}

/** ClearImage() *********************************************/
/** Clear image with a given color.                         **/
/*************************************************************/
void ClearImage(Image *Img,int Color)
{
  switch(Img->D)
  {
    case 8:  ClearImage_8(Img,Color);break;
    case 16: ClearImage_16(Img,Color);break;
    case 24:
    case 32: ClearImage_32(Img,Color);break;
  }
}

/** IMGCopy() ************************************************/
/** Copy one image into another.                            **/
/*************************************************************/
void IMGCopy(Image *Dst,int DX,int DY,const Image *Src,int SX,int SY,int W,int H,int TColor)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  IMGCopy_8(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
    case 16: IMGCopy_16(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
    case 24:
    case 32: IMGCopy_32(Dst,DX,DY,Src,SX,SY,W,H,TColor);break;
  }
}

/** IMGDrawRect()/IMGFillRect() ******************************/
/** Draw filled/unfilled rectangle in a given image.        **/
/*************************************************************/
void IMGDrawRect(Image *Img,int X,int Y,int W,int H,int Color)
{
  switch(Img->D)
  {
    case 8:  IMGDrawRect_8(Img,X,Y,W,H,Color);break;
    case 16: IMGDrawRect_16(Img,X,Y,W,H,Color);break;
    case 24:
    case 32: IMGDrawRect_32(Img,X,Y,W,H,Color);break;
  }
}

void IMGFillRect(Image *Img,int X,int Y,int W,int H,int Color)
{
  switch(Img->D)
  {
    case 8:  IMGFillRect_8(Img,X,Y,W,H,Color);break;
    case 16: IMGFillRect_16(Img,X,Y,W,H,Color);break;
    case 24:
    case 32: IMGFillRect_32(Img,X,Y,W,H,Color);break;
  }
}

/** SoftenImage() ********************************************/
/** Uses softening algorithm to interpolate image Src into  **/
/** a bigger image Dst.                                     **/
/*************************************************************/
void SoftenImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  SoftenImage_8(Dst,Src,X,Y,W,H);break;
    case 16: SoftenImage_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: SoftenImage_32(Dst,Src,X,Y,W,H);break;
  }
}

/** ScaleImage() *********************************************/
/** Copy Src into Dst, scaling as needed                    **/
/*************************************************************/
void ScaleImage(Image *Dst,const Image *Src,int X,int Y,int W,int H)
{
  if(Src->D!=Dst->D) return;

  switch(Dst->D)
  {
    case 8:  ScaleImage_8(Dst,Src,X,Y,W,H);break;
    case 16: ScaleImage_16(Dst,Src,X,Y,W,H);break;
    case 24:
    case 32: ScaleImage_32(Dst,Src,X,Y,W,H);break;
  }
}

/** TelevizeImage() ******************************************/
/** Create televizion effect on the image.                  **/
/*************************************************************/
void TelevizeImage(Image *Img,int X,int Y,int W,int H)
{
  switch(Img->D)
  {
    case 8:  TelevizeImage_8(Img,X,Y,W,H);break;
    case 16: TelevizeImage_16(Img,X,Y,W,H);break;
    case 24:
    case 32: TelevizeImage_32(Img,X,Y,W,H);break;
  }
}

