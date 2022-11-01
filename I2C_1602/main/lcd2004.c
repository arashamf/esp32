#include "lcd2004.h"
//------------------------------------------------
uint8_t portlcd=0; 		//Ячейка для хранения данных порта микросхемы расширения
uint32_t CUR_POS;

//----------------------------------------------------------------------------------------//
inline void LCD_WriteByteI2C(uint8_t bt)
{
  I2C_SendByteByADDR(bt,0x4E);
}

//----------------------------------------------------------------------------------------//
void sendhalfbyte(uint8_t c)
{
  c<<=4;
  LCD_WriteByteI2C((portlcd|=0x04)|c);	//включаем линию E
  usleep(1);
  LCD_WriteByteI2C (portlcd|c);
  LCD_WriteByteI2C((portlcd&=~0x04)|c);	//выключаем линию E
  usleep(1);
}

//----------------------------------------------------------------------------------------//
void sendbyte(uint8_t c, uint8_t mode)
{
	uint8_t hc=0;
	if(mode==0)
		rs_reset();
	else
		rs_set();
	hc=c>>4;
	sendhalfbyte(hc);
	sendhalfbyte(c);
}

//-----------------------------------установка позиции курсора-----------------------------------//
void LCD_SetPos(uint8_t x, uint8_t y)
{
  switch(y)
  {
    case 0:
      CUR_POS = x|0x80;
      break;
    case 1:
      CUR_POS = (0x40+x)|0x80;
      break;
    case 2:
      CUR_POS = (0x14+x)|0x80;
      break;
    case 3:
      CUR_POS = (0x54+x)|0x80;
      break;
  }
  sendbyte(CUR_POS,0);
}

//----------------------------------------------------------------------------------------//
void LCD_String(char* st)
{
  uint8_t i=0;

  while (*(st+i) != 0)
  {
    sendbyte(*(st+i), 1);
    i++;
  }
}

//----------------------------------------------------------------------------------------//
void LCD_ini(void)
{
	// интерфейс - 4-битный, строк - 2, шрифт - 5х7 точек
	usleep(30000);
	LCD_WriteByteI2C(0);
	setwrite();			//запись
	usleep(50000);
	usleep(50000);
	sendhalfbyte(0x03);
	usleep(450);
	sendhalfbyte(0x03);
	usleep(450);
	sendhalfbyte(0x03);
	usleep(450);
	sendhalfbyte(0x02);
	sendbyte(0x28,0); 		//режим 4 бит, 2 линии (дл¤ нашего большого диспле¤ это 4 линии, шрифт 5х8
	sendbyte(0x08,0);		//дисплей пока выключаем
	usleep(1000);
	sendbyte(0x01,0);		//уберем мусор
	usleep(2000);
	sendbyte(0x06,0);		// пишем вправо
	usleep(1000);
	sendbyte(0x0C,0);		//дисплей включаем (D=1), курсоры никакие не нужны
	sendbyte(0x02,0);		//курсор на место
	sendbyte(0X80,0);		//SET POS LINE 0
	usleep(2000);
	setled();				//подсветка
}

//-----------------------------------очистка дисплея-----------------------------------//
void clear_LCD1602 (void)
{
	char buffer[16] ;
	uint8_t count = 0;

	for (count = 0; count < 16; count++)
	{
		buffer[count] = ' '; //заполнение буффера символами пробела
	}

	for (count = 0; count < 2; count++)
	{
		LCD_SetPos(0, count);
		LCD_String (buffer);
	}
}
