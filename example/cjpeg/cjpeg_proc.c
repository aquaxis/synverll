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

static struct APP0infotype {
  unsigned short int marker;           // = 0xFFE0
  unsigned short int length;           // = 16 for usual JPEG, no thumbnail
  unsigned char      JFIFsignature[5]; // = "JFIF",'\0'
  unsigned char      versionhi;        // 1
  unsigned char      versionlo;        // 1
  unsigned char      xyunits;          // 0 = no units, normal density
  unsigned short int xdensity;         // 1
  unsigned short int ydensity;         // 1
  unsigned char      thumbnwidth;      // 0
  unsigned char      thumbnheight;     // 0
} APP0info={0xFFE0,16,'J','F','I','F',0,1,1,0,1,1,0,0};

static struct  SOF0infotype {
  unsigned short int marker;         // = 0xFFC0
  unsigned short int length;         // = 17 for a truecolor YCbCr JPG
  unsigned char      precision ;     // Should be 8: 8 bits/sample
  unsigned short int height ;
  unsigned short int width;
  unsigned char      nrofcomponents; //Should be 3: We encode a truecolor JPG
  unsigned char      IdY;            // = 1
  unsigned char      HVY;            // sampling factors for Y
                                     // (bit 0-3 vert., 4-7 hor.)
  unsigned char      QTY;            // Quantization Table number for Y = 0
  unsigned char      IdCb;           // = 2
  unsigned char      HVCb;
  unsigned char      QTCb;           // 1
  unsigned char      IdCr;           // = 3
  unsigned char      HVCr;
  unsigned char      QTCr;           // Normally equal to QTCb = 1
} SOF0info = { 0xFFC0,17,8,0,0,3,1,0x22,0,2,0x11,1,3,0x11,1};

static struct DQTinfotype {
  unsigned short int marker;      // = 0xFFDB
  unsigned short int length;      // = 132
  unsigned char      QTYinfo;     // = 0:  bit 0..3: number of QT = 0
                                  // (table for Y)
                                  //   bit 4..7: precision of QT, 0 = 8 bit
  unsigned char      Ytable[64];
  unsigned char      QTCbinfo;    // = 1 (quantization table for Cb,Cr}
  unsigned char      Cbtable[64];
} DQTinfo;

static struct DHTinfotype {
  unsigned short int marker;          // = 0xFFC4
  unsigned short int length;          //0x01A2
  unsigned char      HTYDCinfo;       // bit 0..3: number of HT (0..3), for Y =0
                                      //bit 4  :type of HT, 0 = DC table,1 = AC table
                                      //bit 5..7: not used, must be 0
  unsigned char      YDC_nrcodes[16]; //at index i = nr of codes with length i
  unsigned char      YDC_values[12];
  unsigned char      HTYACinfo;        // = 0x10
  unsigned char      YAC_nrcodes[16];
  unsigned char      YAC_values[162];  //we'll use the standard Huffman tables
  unsigned char      HTCbDCinfo;       // = 1
  unsigned char      CbDC_nrcodes[16];
  unsigned char      CbDC_values[12];
  unsigned char      HTCbACinfo;       //  = 0x11
  unsigned char      CbAC_nrcodes[16];
  unsigned char      CbAC_values[162];
} DHTinfo;

static struct SOSinfotype {
  unsigned short int marker;    // = 0xFFDA
  unsigned short int length;    // = 12
  unsigned char      nrofcomponents; // Should be 3: truecolor JPG
  unsigned char      IdY;            //1
  unsigned char      HTY;            //0 // bits 0..3: AC table (0..3)
                                     // bits 4..7: DC table (0..3)
  unsigned char      IdCb;           //2
  unsigned char      HTCb;           //0x11
  unsigned char      IdCr;           //3
  unsigned char      HTCr;           //0x11
  unsigned char      Ss,Se,Bf;       // not interesting, they should be 0,63,0
} SOSinfo={0xFFDA,12,3,1,0,2,0x11,3,0x11,0,0x3F,0};

typedef struct { unsigned char length;  unsigned short int value;} bitstring;

static unsigned char zigzag[64]={
  0, 1, 5, 6,14,15,27,28,
  2, 4, 7,13,16,26,29,42,
  3, 8,12,17,25,30,41,43,
  9,11,18,24,31,40,44,53,
  10,19,23,32,39,45,52,54,
  20,22,33,38,46,51,55,60,
  21,34,37,47,50,56,59,61,
  35,36,48,49,57,58,62,63
};

/* DQTテーブル
 *　JPEG標準の1/2しているのでクオリティが高くなっている */
static unsigned char DQT_Y[64] = {
  0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07,
  0x07, 0x07, 0x09, 0x09, 0x08, 0x0a, 0x0c, 0x14,
  0x0d, 0x0c, 0x0b, 0x0b, 0x0c, 0x19, 0x12, 0x13,
  0x0f, 0x14, 0x1d, 0x1a, 0x1f, 0x1e, 0x1d, 0x1a,
  0x1c, 0x1c, 0x20, 0x24, 0x2e, 0x27, 0x20, 0x22,
  0x2c, 0x23, 0x1c, 0x1c, 0x28, 0x37, 0x29, 0x2c,
  0x30, 0x31, 0x34, 0x34, 0x34, 0x1f, 0x27, 0x39,
  0x3d, 0x38, 0x32, 0x3c, 0x2e, 0x33, 0x34, 0x32
};

static unsigned char DQT_C[64] = {
  0x09, 0x09, 0x09, 0x0c, 0x0b, 0x0c, 0x18, 0x0d,
  0x0d, 0x18, 0x32, 0x21, 0x1c, 0x21, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
  0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32
};

static unsigned char std_dc_luminance_nrcodes[17]={0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
static unsigned char std_dc_luminance_values[12]={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static unsigned char std_dc_chrominance_nrcodes[17]={0,0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
static unsigned char std_dc_chrominance_values[12]={0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static unsigned char std_ac_luminance_nrcodes[17]={0,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
static unsigned char std_ac_luminance_values[162]= {
  0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa };

static unsigned char std_ac_chrominance_nrcodes[17]={0,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
static unsigned char std_ac_chrominance_values[162]={
  0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa };

static unsigned char bytenew=0;
static signed char bytepos=7;	// 0〜7の範囲
static unsigned short int mask[16]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};

// ハフマンテーブル
static bitstring YDC_HT[12];
static bitstring CbDC_HT[12];
static bitstring YAC_HT[256];
static bitstring CbAC_HT[256];

unsigned char *buffer;		//image to be encoded
unsigned short int Ximage,Yimage;	// 1/8サイズ

// YCbCr
static signed char DU_Y[256];
static signed char DU_Cb[64];
static signed char DU_Cr[64];

static signed short int DU_DCT[64];
static signed short int DU[64];

//FILE *fp_jpeg_stream;
unsigned char *fp_jpeg_stream;
int jpeg_ptr = 0;

//#define writebyte(b) fputc((b),fp_jpeg_stream)
//#define writebyte(b) fp_jpeg_stream[jpeg_ptr++] = (b);
//#define writeword(w) writebyte((w)/256);writebyte((w)%256);

void writebyte(unsigned char b)
{
  fp_jpeg_stream[jpeg_ptr++] = (b);
}

void writeword(unsigned short w)
{
  writebyte((w)/256);
  writebyte((w)%256);
}

void write_APP0info(){
  writeword(APP0info.marker);
  writeword(APP0info.length);
  writebyte('J');writebyte('F');writebyte('I');writebyte('F');writebyte(0);
  writebyte(APP0info.versionhi);
  writebyte(APP0info.versionlo);
  writebyte(APP0info.xyunits);
  writeword(APP0info.xdensity);
  writeword(APP0info.ydensity);
  writebyte(APP0info.thumbnwidth);
  writebyte(APP0info.thumbnheight);
}

void write_SOF0info(){
  writeword(SOF0info.marker);
  writeword(SOF0info.length);
  writebyte(SOF0info.precision);
  writeword(SOF0info.height);
  writeword(SOF0info.width);
  writebyte(SOF0info.nrofcomponents);
  writebyte(SOF0info.IdY);
  writebyte(SOF0info.HVY);
  writebyte(SOF0info.QTY);
  writebyte(SOF0info.IdCb);
  writebyte(SOF0info.HVCb);
  writebyte(SOF0info.QTCb);
  writebyte(SOF0info.IdCr);
  writebyte(SOF0info.HVCr);
  writebyte(SOF0info.QTCr);
}

void write_DQTinfo(){
  unsigned char i;
  writeword(DQTinfo.marker);
  writeword(DQTinfo.length);
  writebyte(DQTinfo.QTYinfo);
  for (i=0;i<64;i++) writebyte(DQTinfo.Ytable[i]);
  writebyte(DQTinfo.QTCbinfo);
  for (i=0;i<64;i++) writebyte(DQTinfo.Cbtable[i]);
}

void set_DQTinfo(){
  int i;
  DQTinfo.marker=0xFFDB;
  DQTinfo.length=132;
  DQTinfo.QTYinfo=0;
  DQTinfo.QTCbinfo=1;
  for(i=0;i<64;i++) DQTinfo.Ytable[i]  = DQT_Y[i];
  for(i=0;i<64;i++) DQTinfo.Cbtable[i] = DQT_C[i];
}

void write_DHTinfo(){
  unsigned char i;
  writeword(DHTinfo.marker);
  writeword(DHTinfo.length);
  writebyte(DHTinfo.HTYDCinfo);
  for (i=0;i<16;i++)  writebyte(DHTinfo.YDC_nrcodes[i]);
  for (i=0;i<=11;i++) writebyte(DHTinfo.YDC_values[i]);
  writebyte(DHTinfo.HTYACinfo);
  for (i=0;i<16;i++)  writebyte(DHTinfo.YAC_nrcodes[i]);
  for (i=0;i<=161;i++) writebyte(DHTinfo.YAC_values[i]);
  writebyte(DHTinfo.HTCbDCinfo);
  for (i=0;i<16;i++)  writebyte(DHTinfo.CbDC_nrcodes[i]);
  for (i=0;i<=11;i++)  writebyte(DHTinfo.CbDC_values[i]);
  writebyte(DHTinfo.HTCbACinfo);
  for (i=0;i<16;i++)  writebyte(DHTinfo.CbAC_nrcodes[i]);
  for (i=0;i<=161;i++) writebyte(DHTinfo.CbAC_values[i]);
}

void set_DHTinfo(){
  unsigned char i;
  DHTinfo.marker=0xFFC4;
  DHTinfo.length=0x01A2;
  DHTinfo.HTYDCinfo=0;
  for (i=0;i<16;i++)  DHTinfo.YDC_nrcodes[i]=std_dc_luminance_nrcodes[i+1];
  for (i=0;i<=11;i++)  DHTinfo.YDC_values[i]=std_dc_luminance_values[i];
  DHTinfo.HTYACinfo=0x10;
  for (i=0;i<16;i++)  DHTinfo.YAC_nrcodes[i]=std_ac_luminance_nrcodes[i+1];
  for (i=0;i<=161;i++) DHTinfo.YAC_values[i]=std_ac_luminance_values[i];
  DHTinfo.HTCbDCinfo=1;
  for (i=0;i<16;i++)  DHTinfo.CbDC_nrcodes[i]=std_dc_chrominance_nrcodes[i+1];
  for (i=0;i<=11;i++)  DHTinfo.CbDC_values[i]=std_dc_chrominance_values[i];
  DHTinfo.HTCbACinfo=0x11;
  for (i=0;i<16;i++)  DHTinfo.CbAC_nrcodes[i]=std_ac_chrominance_nrcodes[i+1];
  for (i=0;i<=161;i++) DHTinfo.CbAC_values[i]=std_ac_chrominance_values[i];
}

void write_SOSinfo(){
  writeword(SOSinfo.marker);
  writeword(SOSinfo.length);
  writebyte(SOSinfo.nrofcomponents);
  writebyte(SOSinfo.IdY);
  writebyte(SOSinfo.HTY);
  writebyte(SOSinfo.IdCb);
  writebyte(SOSinfo.HTCb);
  writebyte(SOSinfo.IdCr);
  writebyte(SOSinfo.HTCr);
  writebyte(SOSinfo.Ss);
  writebyte(SOSinfo.Se);
  writebyte(SOSinfo.Bf);
}

void writebits(bitstring bs){
  unsigned short int value;
  signed char posval;
  value=bs.value;
  posval=bs.length-1;
  while(posval>=0){
    if(value & mask[posval]) bytenew|=mask[bytepos];
    posval--;bytepos--;
    if(bytepos<0) {
      if(bytenew==0xFF) {
        writebyte(0xFF);
        writebyte(0);
      }else{
        writebyte(bytenew);
      }
      bytepos=7;
      bytenew=0;
    }
  }
}

void compute_Huffman_table(unsigned char *nrcodes,unsigned char *std_table,bitstring *HT){
  unsigned char k,j;
  unsigned char pos_in_table;
  unsigned short int codevalue;
  codevalue=0; pos_in_table=0;
  for (k=1;k<=16;k++){
    for (j=1;j<=nrcodes[k];j++) {
      HT[std_table[pos_in_table]].value=codevalue;
      HT[std_table[pos_in_table]].length=k;
      pos_in_table++;
      codevalue++;
    }
    codevalue*=2;
  }
}

void init_Huffman_tables(){
  compute_Huffman_table(std_dc_luminance_nrcodes,std_dc_luminance_values,YDC_HT);
  compute_Huffman_table(std_dc_chrominance_nrcodes,std_dc_chrominance_values,CbDC_HT);
  compute_Huffman_table(std_ac_luminance_nrcodes,std_ac_luminance_values,YAC_HT);
  compute_Huffman_table(std_ac_chrominance_nrcodes,std_ac_chrominance_values,CbAC_HT);
}

bitstring calc_bitcode(short code){
	unsigned short data;
	bitstring retcode;
	unsigned char scode;

	if(code < 0){
		data = (-1) * code;
		scode = 1;
	}else{
		data = code;
		scode = 0;
	}

	if((data & 0xFFFE) == 0){
		retcode.length	= 1;
		retcode.value	= 0x0001;
	}else if((data & 0xFFFC) == 0){
		retcode.length	= 2;
		retcode.value	= 0x0003;
	}else if((data & 0xFFF8) == 0){
		retcode.length	= 3;
		retcode.value	= 0x0007;
	}else if((data & 0xFFF0) == 0){
		retcode.length	= 4;
		retcode.value	= 0x000F;
	}else if((data & 0xFFE0) == 0){
		retcode.length	= 5;
		retcode.value	= 0x001F;
	}else if((data & 0xFFC0) == 0){
		retcode.length	= 6;
		retcode.value	= 0x003F;
	}else if((data & 0xFF80) == 0){
		retcode.length	= 7;
		retcode.value	= 0x007F;
	}else if((data & 0xFF00) == 0){
		retcode.length	= 8;
		retcode.value	= 0x00FF;
	}else if((data & 0xFE00) == 0){
		retcode.length	= 9;
		retcode.value	= 0x01FF;
	}else if((data & 0xFC00) == 0){
		retcode.length	= 10;
		retcode.value	= 0x03FF;
	}else if((data & 0xF800) == 0){
		retcode.length	= 11;
		retcode.value	= 0x07FF;
	}else if((data & 0xF000) == 0){
		retcode.length	= 12;
		retcode.value	= 0x0FFF;
	}else if((data & 0xE000) == 0){
		retcode.length	= 13;
		retcode.value	= 0x1FFF;
	}else if((data & 0xC000) == 0){
		retcode.length	= 14;
		retcode.value	= 0x3FFF;
	}else if((data & 0x8000) == 0){
		retcode.length	= 15;
		retcode.value	= 0x7FFF;
	}

	if(scode){
		retcode.value	= ~data & retcode.value;
	}else{
		retcode.value	= data & retcode.value;
	}

	return retcode;
}

#define C1_1 2896 /* 0.707106781 * 4096 */
#define C1_2 1567 /* 0.382683433 * 4096 */
#define C1_3 2216 /* 0.541196100 * 4096 */
#define C1_4 5351 /* 1.306562965 * 4096 */
#define C1_5 2896 /* 0.707106781 * 4096 */

void DCT(signed char *data,unsigned char *fdtbl,signed short int *outdata){
  signed long tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  signed long tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17, tmp18, tmp19, tmp20;
  signed long z1, z2, z3, z4, z5, z11, z13;
  signed long *dataptr;
  signed long datalong[64];
  signed char ctr;
  unsigned char i;
  for (i=0;i<64;i++) datalong[i]=data[i];

  dataptr=datalong;
  for (ctr = 7; ctr >= 0; ctr--) {
    // Phase 1
    tmp0 = dataptr[0] + dataptr[7];
    tmp7 = dataptr[0] - dataptr[7];
    tmp1 = dataptr[1] + dataptr[6];
    tmp6 = dataptr[1] - dataptr[6];
    tmp2 = dataptr[2] + dataptr[5];
    tmp5 = dataptr[2] - dataptr[5];
    tmp3 = dataptr[3] + dataptr[4];
    tmp4 = dataptr[3] - dataptr[4];

    // Phase 2
    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    tmp14 = tmp4 + tmp5;
    tmp15 = tmp5 + tmp6;
    tmp16 = tmp6 + tmp7;

    // Phase 3
    dataptr[0] = tmp10 + tmp11;
    dataptr[4] = tmp10 - tmp11;
    tmp17 = tmp12 + tmp13;
    tmp18 = tmp14 - tmp16;
    tmp19 = C1_3 * tmp14;
    tmp20 = C1_4 * tmp16;
    z3 = tmp15 * C1_5;

    // Phase 4
    z1 = tmp17 * C1_1;
    z5 = tmp18 * C1_2;
    z11 = (tmp7 << 12) + z3;
    z13 = (tmp7 << 12) - z3;

    // Phase 5
    dataptr[2] = ((tmp13 << 12) + z1) >> 12;
    dataptr[6] = ((tmp13 << 12) - z1) >> 12;
    z2 = tmp19 + z5;
    z4 = tmp20 + z5;

    // Phase 6
    dataptr[5] = (z13 + z2) >> 12;
    dataptr[3] = (z13 - z2) >> 12;
    dataptr[1] = (z11 + z4) >> 12;
    dataptr[7] = (z11 - z4) >> 12;

    dataptr += 8;
  }

  dataptr -= 64;
  for (ctr = 7; ctr >= 0; ctr--) {
    // Phase 1
    tmp0 = dataptr[8*0] + dataptr[8*7];
    tmp7 = dataptr[8*0] - dataptr[8*7];
    tmp1 = dataptr[8*1] + dataptr[8*6];
    tmp6 = dataptr[8*1] - dataptr[8*6];
    tmp2 = dataptr[8*2] + dataptr[8*5];
    tmp5 = dataptr[8*2] - dataptr[8*5];
    tmp3 = dataptr[8*3] + dataptr[8*4];
    tmp4 = dataptr[8*3] - dataptr[8*4];

    // Phase 2
    tmp10 = tmp0 + tmp3;
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;
    tmp14 = tmp4 + tmp5;
    tmp15 = tmp5 + tmp6;
    tmp16 = tmp6 + tmp7;

    // Phase 3
    dataptr[8*0] = tmp10 + tmp11;
    dataptr[8*4] = tmp10 - tmp11;
    tmp17 = tmp12 + tmp13;
    tmp18 = tmp14 - tmp16;
    tmp19 = C1_3 * tmp14;
    tmp20 = C1_4 * tmp16;
    z3 = tmp15 * C1_5;

    // Phase 4
    z1 = tmp17 * C1_1;
    z5 = tmp18 * C1_2;
    z11 = (tmp7 << 12) + z3;
    z13 = (tmp7 << 12) - z3;

    // Phase 5
    dataptr[8*2] = ((tmp13 << 12) + z1) >> 12;
    dataptr[8*6] = ((tmp13 << 12) - z1) >> 12;
    z2 = tmp19 + z5;
    z4 = tmp20 + z5;

    // Phase 6
    dataptr[8*5] = (z13 + z2) >> 12;
    dataptr[8*3] = (z13 - z2) >> 12;
    dataptr[8*1] = (z11 + z4) >> 12;
    dataptr[8*7] = (z11 - z4) >> 12;

    dataptr++;
  }

  for (i = 0; i < 64; i++) {
    outdata[i] = (signed short int) datalong[i];
  }
}

void process_DU(signed char *ComponentDU,unsigned char *fdtbl,
		signed short int *DC,
		bitstring *HTDC,bitstring *HTAC){
  unsigned char i;
  unsigned char startpos;
  unsigned char end0pos;
  unsigned char nrzeroes;
  unsigned char nrmarker;
  signed short int Diff;
  bitstring calc_data;

  // DCT変換
  DCT(ComponentDU,fdtbl,DU_DCT);

  // DQT
  for (i = 0; i < 64; i++) {
    // 小数点以下切り捨て(精度を求めるなら、切り上げ処理が必要)
    DU_DCT[i] = (signed short int) (DU_DCT[i] / (fdtbl[zigzag[i]] << 3));
  }

  // ジグザグ変換
  for (i=0;i<=63;i++) DU[zigzag[i]]=DU_DCT[i];

  // ハフマン符号化
  // DC成分
  Diff=DU[0]-*DC;	// 前回のDC成分と比較
  *DC=DU[0];
  // DC成分の比較が0なら0コード
  if (Diff==0) writebits(HTDC[0]);
  else {
	calc_data = calc_bitcode(Diff);
    writebits(HTDC[calc_data.length]);
    writebits(calc_data);
  }

  // AC成分
  // 0の数を後ろから数える
  for (end0pos=63;(end0pos>0)&&(DU[end0pos]==0);end0pos--) ;
  if (end0pos==0) {
    // 全部0なら0コード
    writebits(HTAC[0x00]);
    return;
  }

  i=1;
  while (i<=end0pos){
    startpos=i;
    for (; (DU[i]==0)&&(i<=end0pos);i++) ;
    nrzeroes=i-startpos;
    if (nrzeroes>=16) {
      // 0が16個、0が16個続いたコード
      for (nrmarker=1;nrmarker<=nrzeroes/16;nrmarker++) writebits(HTAC[0xF0]);
      nrzeroes=nrzeroes%16;
    }
	calc_data = calc_bitcode(DU[i]);
    writebits(HTAC[nrzeroes*16+calc_data.length]);
    writebits(calc_data);
    i++;
  }
  if (end0pos!=63) writebits(HTAC[0x00]); // 残りがあれば0コード
}

void RGB2YCbCr(unsigned short int xpos,unsigned short int ypos){
  unsigned char x,y;
  unsigned char R,G,B;
  unsigned int rR,rG,rB;
  for(y=0;y<16;y++){
    for(x=0;x<16;x++){
      R=buffer[ypos*Ximage*3+xpos*3+y*Ximage*3+x*3+2];
      G=buffer[ypos*Ximage*3+xpos*3+y*Ximage*3+x*3+1];
      B=buffer[ypos*Ximage*3+xpos*3+y*Ximage*3+x*3+0];
      rR = 19595 * R;	// 0.299 * 65536 * R
      rG = 38470 * G;	// 0.587 * 65536 * G
      rB =  7471 * B;	// 0.114 * 65536 * B
      DU_Y[y*16+x]  = (unsigned char)((rR + rG + rB) >> 16) -128;
      // 4画素の平均値にはしていない
      if((x%2==0) & (y%2==0)){
        rR = 11059 * R;	// 0.16874 * 65536 * R
        rG = 21709 * G;	// 0.33126 * 65536 * G
        rB = 32768 * B;	// 0.5     * 65536 * B
        DU_Cb[y/2*8+x/2] = (unsigned char)((rB + rG + rR) >> 16);
        rR = 32768 * R;	// 0.5     * 65536 * R
        rG = 27439 * G;	// 0.41869 * 65536 * G
        rB =  5328 * B;	// 0.08131 * 65536 * B
        DU_Cr[y/2*8+x/2] = (unsigned char)((rB + rG + rR) >> 16);
      }
    }
  }
}

void main_encoder(){
  signed short int DCY=0,DCCb=0,DCCr=0;
  unsigned short int xpos,ypos;
  signed char DU[64];
  int i;
  for (ypos=0;ypos<Yimage;ypos+=16){
    for (xpos=0;xpos<Ximage;xpos+=16){
      // RGB2YCbCr変換(4:2:0変換)
      RGB2YCbCr(xpos,ypos);
      // DU処理
      for(i=0;i<64;i++) DU[i] = DU_Y[(i>>3)*16+i&0x7+0];
      process_DU(DU,DQTinfo.Ytable,&DCY,YDC_HT,YAC_HT);
      for(i=0;i<64;i++) DU[i] = DU_Y[(i>>3)*16+i&0x7+8];
      process_DU(DU,DQTinfo.Ytable,&DCY,YDC_HT,YAC_HT);
      for(i=0;i<64;i++) DU[i] = DU_Y[(i>>3)*16+i&0x7+128];
      process_DU(DU,DQTinfo.Ytable,&DCY,YDC_HT,YAC_HT);
      for(i=0;i<64;i++) DU[i] = DU_Y[(i>>3)*16+i&0x7+136];
      process_DU(DU,DQTinfo.Ytable,&DCY,YDC_HT,YAC_HT);
      process_DU(DU_Cb,DQTinfo.Cbtable,&DCCb,CbDC_HT,CbAC_HT);
      process_DU(DU_Cr,DQTinfo.Cbtable,&DCCr,CbDC_HT,CbAC_HT);
    }
  }
}

void init_all(){
  set_DQTinfo();
  set_DHTinfo();
  init_Huffman_tables();
}

void create_jpeg(unsigned short int Ximage_original, unsigned short int Yimage_original)
{
  bitstring fillbits;

  // 初期化
  init_all();

  SOF0info.width=Ximage_original;
  SOF0info.height=Yimage_original;

  Ximage=(Ximage_original/16)*16+16;
  Yimage=(Yimage_original/16)*16+16;

  writeword(0xFFD8); //SOI

  // ヘッダ書き込み
  write_APP0info();
  write_DQTinfo();
  write_DHTinfo();
  write_SOF0info();
  write_SOSinfo();

  bytenew=0;bytepos=7;
  main_encoder();
  if(bytepos>=0){
    fillbits.length=bytepos+1;
    fillbits.value=(1<<(bytepos+1))-1;
    writebits(fillbits);
  }
  writeword(0xFFD9); //EOI
}

