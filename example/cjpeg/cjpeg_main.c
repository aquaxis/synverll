/*
 * Copyright (C)2005-2015 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * For further information please contact.
 *  URI:    http://www.aquaxis.com/
 *  E-Mail: info(at)aquaxis.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char *buffer;		//image to be encoded
extern unsigned char *fp_jpeg_stream;
extern int jpeg_ptr;
unsigned short int Ximage,Yimage;	// 1/8サイズ

void load_bitmap(char *bitmap_name, unsigned short int *Ximage_original, unsigned short int *Yimage_original){
  unsigned short int Xdiv8,Ydiv8;
  unsigned char nr_fillingbytes;
  unsigned int offset;
  unsigned char lastcolor[3];
  unsigned short int column;
  unsigned char TMPBUF[256];
  unsigned short int nrline_up,nrline_dn,nrline;
  unsigned short int dimline;
  unsigned char *tmpline;
  FILE *fp_bitmap=fopen(bitmap_name,"rb");
  fread(TMPBUF,1,54,fp_bitmap);
  Ximage=(unsigned short int)TMPBUF[19]*256+TMPBUF[18];
  Yimage=(unsigned short int)TMPBUF[23]*256+TMPBUF[22];
  *Ximage_original=Ximage;
  *Yimage_original=Yimage;
  if (Ximage%16!=0) Xdiv8=(Ximage/16)*16+16;
  else Xdiv8=Ximage;
  if (Yimage%16!=0) Ydiv8=(Yimage/16)*16+16;
  else Ydiv8=Yimage;

  offset = TMPBUF[10];
  if(offset > 54){
    offset -= 51;
    fread(TMPBUF,1,offset,fp_bitmap);
  }

  buffer=(unsigned char *)(malloc(3*Xdiv8*Ydiv8));
  if (Ximage%4!=0) nr_fillingbytes=4-(Ximage%4);
  else nr_fillingbytes=0;
  for (nrline=0;nrline<Yimage;nrline++){
    fread(buffer+nrline*Xdiv8*3,1,Ximage*3,fp_bitmap);
    fread(TMPBUF,1,nr_fillingbytes,fp_bitmap);
    memcpy(&lastcolor,buffer+nrline*Xdiv8*3+Ximage*3-1,3);
    for (column=Ximage;column<Xdiv8;column++){
      memcpy(buffer+nrline*Xdiv8*3+column,&lastcolor,3);
    }
  }
  Ximage=Xdiv8;
  dimline=Ximage*3;
  tmpline=(unsigned char *)malloc(dimline);

  for (nrline_up=Yimage-1,nrline_dn=0;nrline_up>nrline_dn;nrline_up--,nrline_dn++){
    memcpy(tmpline,buffer+nrline_up*Ximage*3, dimline);
    memcpy(buffer+nrline_up*Ximage*3,buffer+nrline_dn*Ximage*3,dimline);
    memcpy(buffer+nrline_dn*Ximage*3,tmpline,dimline);
  }

  memcpy(tmpline,buffer+(Yimage-1)*Ximage*3,dimline);
  for (nrline=Yimage;nrline<Ydiv8;nrline++){
    memcpy(buffer+nrline*Ximage*3,tmpline,dimline);
  }
  Yimage=Ydiv8;
  free(tmpline);
  fclose(fp_bitmap);
}

int main(int argc,char *argv[]){
  char BMP_filename[64];
  char JPG_filename[64];
  unsigned short int Ximage_original,Yimage_original;
  unsigned char len_filename;
  FILE *fp_jpeg;
  unsigned char *jpeg_stream;

  fp_jpeg_stream = (unsigned char *)malloc(1*1024*1024);

  // ファイル名の指定
  if (argc>1) { strcpy(BMP_filename,argv[1]);
    if (argc>2) strcpy(JPG_filename,argv[2]);
    else{
      strcpy(JPG_filename,BMP_filename);
      len_filename=strlen(BMP_filename);
      strcpy(JPG_filename+(len_filename-3),"jpg");
    }
  }
  // ビットマップのロード
  load_bitmap(BMP_filename, &Ximage_original, &Yimage_original);

  create_jpeg(Ximage_original, Yimage_original);

  free(buffer);

  fp_jpeg=fopen(JPG_filename,"wb");
  fwrite(fp_jpeg_stream, 1, jpeg_ptr, fp_jpeg);
  fclose(fp_jpeg);

  free(fp_jpeg_stream);

  return 0;
}
