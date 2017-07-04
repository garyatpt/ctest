#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include "ascii2.h"

int g_fdoled;
char *g_pstroledDev="/dev/i2c-0";
uint8_t g_u8oledAddr=0x78>>1;
enum oled_sig{   //---command or data
oled_SIG_CMD,
oled_SIG_DAT
};
//struct i2c_rdwr_ioctl_data i2c_iodata;



/*--------------send command to oled---------------
  send DATA or COMMAND to oled 
----------------------------------------------*/
void sendDatCmdoled(enum oled_sig datcmd,uint8_t val) {
    int ret;
    uint8_t sig;

    if(datcmd == oled_SIG_CMD)
	sig=0x00; //----send command
    else
	sig=0x40; //--send data

    struct i2c_rdwr_ioctl_data i2c_iodata;

    i2c_iodata.nmsgs=1;
    i2c_iodata.msgs=(struct i2c_msg*)malloc(i2c_iodata.nmsgs*sizeof(struct i2c_msg));
    i2c_iodata.msgs[0].len=2;
    i2c_iodata.msgs[0].addr=g_u8oledAddr;
    i2c_iodata.msgs[0].flags=0; //write
    i2c_iodata.msgs[0].buf=(unsigned char*)malloc(2);
    i2c_iodata.msgs[0].buf[0]=sig; // 0x00 for Command, 0x40 for data
    i2c_iodata.msgs[0].buf[1]=val; 

    ioctl(g_fdoled,I2C_TIMEOUT,2);
    ioctl(g_fdoled,I2C_RETRIES,1);

    ret=ioctl(g_fdoled,I2C_RDWR,(unsigned long)&i2c_iodata);
    if(ret<0)
    {
	printf("i2c ioctl read error!\n");
    }

    free(i2c_iodata.msgs);
    free(i2c_iodata.msgs[0].buf);
}

/*------ send command to oled ---------*/
void sendCmdOled(uint8_t cmd)
{
   sendDatCmdoled(oled_SIG_CMD,cmd);
}

/*------ send  data to oled ---------*/
void sendDatOled(uint8_t dat)
{
   sendDatCmdoled(oled_SIG_DAT,dat);
}



/*----- open i2c slave and init oled with default param-----*/
void initOledDefault(void)
{

  if((g_fdoled=open(g_pstroledDev,O_RDWR))<0)
  {
	perror("fail to open i2c bus");
        exit(1);
  }
  else
   	printf("Open i2c bus successfully!\n");

  sendCmdOled(0xAE); //display off
  //-----------------------
  sendCmdOled(0x20);  //set memory addressing mode
  sendCmdOled(0x10); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)
  //-----------------------
  sendCmdOled(0xb0);  // 0xb0~b7 set page start address for page addressing mode
  //-----------------------
  sendCmdOled(0xc8);// set COM Output scan direction  0xc0-- COM0 to COM[N-1], 0xc8-- COM[N-1] to COM0
  //-----------------------
  sendCmdOled(0x0f);//2); //0x00~0F, set lower column start address for page addressing mode
  //-----------------------
  sendCmdOled(0x10); // 0x10~1F, set higher column start address for page addressing mode
  //-----------------------
  sendCmdOled(0x40); //40~7F set display start line 0x01(5:0)
  //--------------------
  sendCmdOled(0x81); // contrast control
  sendCmdOled(0x7f); //contrast value RESET=7Fh 
  //--------------------
  sendCmdOled(0xa1); //set segment remap a0h--column 0 is mapped to SEG0, a1h--column 127 is mapped to SEG0
  // ------------------
  sendCmdOled(0xa6); //normal / reverse a6h--noraml, 1 in RAM displays; a7h--inverse, 0 in RAM displays.
  //------------------
  sendCmdOled(0xa8); //multiplex ratio
  sendCmdOled(0x3F); // 16MUX to 64MUX RESET=111111b
  //----------------
  sendCmdOled(0xa4);// A4h-- resume to RAM content display(RESET), A5h--entire display ON.
  //----------------
  sendCmdOled(0xd3);// set display offset, set vertical shift by COM from 0d~63d
  sendCmdOled(0x00);
  //----------------
  sendCmdOled(0xd5); // set desplay clock divide,
  sendCmdOled(0xf0);// [7:4] osc. freq; [3:0] divide ratio, RESET is 00001000B 
  //----------------
  sendCmdOled(0xd9);// set pre-charge period 
  sendCmdOled(0x22);//[3:0] phase 1 period of up to 15 DCLK [7:4] phase 2 period of up to 15 DCLK 
  //----------------
  sendCmdOled(0xda);//--set COM pins hardware configuration
  sendCmdOled(0x12);//[5:4] 
  //----------------
  sendCmdOled(0xdb);//set Vcomh deselect level
  sendCmdOled(0x20);
  //=--------------
  sendCmdOled(0x8d);// charge pump setting 
  sendCmdOled(0x14);// [2]=0 disable charge pump, [2]=1 enbale charge pump
  //---------------
  sendCmdOled(0xaf);// AE, display off(sleep mode), AF, display on in normal mode.
}


void drawOledDat(uint8_t dat)
{
	uint8_t i,j;
	for(i=0;i<8;i++)
	{
		sendCmdOled(0xb0+i); //0xB0~B7, page0-7
		sendCmdOled(0x08); //00~0f,low column start address  ?? no use !!
		sendCmdOled(0x1b); //10~1f, ???high column start address
		for(j=0;j<64;j++) // write to 128 segs, 1 byte each seg.
		{
			sendDatOled(dat);
		}
	}
}


void clearOled(void)
{
    int i,j;

    sendCmdOled(0x20);  //set memory addressing mode
    sendCmdOled(0x02); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)

    for(i=0;i<8;i++)
    {
	  sendCmdOled(0xb0+i);  // 0xb0~b7 set page start address for page addressing mode
	  sendCmdOled(0x00); // low column start address
	  sendCmdOled(0x10); // high column start address
	  for(j=0;j<128;j++)
		sendDatOled(0x00); //--clear  GRAM
     }

}



const unsigned char asc_k[16]={0xf0,0x1f,0xf0,0x1f,0xf0,0x1f,0xc0,0x03,0xe0,0x07,0x70,0x0f,0x30,0x1c,0x10,0x18};

/*----------------------------------------------------
  draw Ascii symbol on Oled
  start_column: 0~7  (128x64)
  sart_row:  0~15;!!!!!!
------------------------------------------------------*/
void  drawOledAscii16x8(uint8_t start_row, uint8_t start_column,unsigned char c)
{
	int i,j;

	//----set mode---
	//-----set as vertical addressing mode -----
        sendCmdOled(0x20);  //set memory addressing mode
        sendCmdOled(0x01); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)
	//---set column addr. for horizontal mode
	sendCmdOled(0x21);
	sendCmdOled(start_column);//column start
	sendCmdOled(start_column+7);//column end, !!!! 8x16 for one line only!!!
	//---set page addr. 
	sendCmdOled(0x22);
	sendCmdOled(start_row);// 0-7 start page
	sendCmdOled(start_row+1);// 2 rows. 2x8=16  !!!! for one line only!!!!

	for(j=0;j<16;j++)
		sendDatOled(ascii_8x16[(c-0x20)*16+j]);


}

void  drawOledStr16x8(uint8_t start_row, uint8_t start_column,const char* pstr)
{
    int k;
    int len=strlen(pstr);
  
    for(k=0;k<len;k++)
        drawOledAscii16x8(start_row,start_column+9*k,*(pstr+k));

}


//--------- horizontal mode -----  FAIL !!!!
void  drawOledAsciiHRC(uint8_t start_row, uint8_t start_column,unsigned char c)
{
	int i,j;

	sendCmdOled(0xb0+start_row); //0xB0~B7, page0-7
	sendCmdOled(0x00+start_column); //00~0f,low column start address  ?? no use !!
	sendCmdOled(0x1f); //10~1f, ???high column start address

	for(j=0;j<16;j++) // write to 128 segs, 1 byte each seg.
	{
		sendDatOled(ascii_8x16[(c-0x20)*16+j]);
	}

}



//======================== MAIN ==================

void main()
{
 int k;
 char strtime[10]={0};
 time_t timep;
 struct tm *p_tm;

 //---init oled ----
 initOledDefault();
 //----clear display----
 clearOled();

 //drawOledAscii16x8(2*k,16*k,'W');
 drawOledStr16x8(0,0,"  Widora-NEO! ");
 drawOledStr16x8(6,0," Test for Ting");

  while(1)
  {
	timep=time(NULL);
	p_tm=localtime(&timep);
	strftime(strtime,sizeof(strtime),"%H:%M:%S",p_tm);
	drawOledStr16x8(3,30,strtime);
	usleep(200000);
  }

}
