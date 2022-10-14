/**************************************************************************/
/*
    Driver for st7735 128x160 pixel TFT LCD displays.
    
    This driver uses a bit-banged SPI interface and a 16-bit RGB565
    colour palette.
*/
/**************************************************************************/
#include "st7735.h"

const unsigned char * GlobalFont = Arial_22x23;  
unsigned int Paint_Color = BLACK;
unsigned int Back_Color = WHITE;
unsigned char Lcd_buf[ST7735_PANEL_WIDTH*ST7735_PANEL_HEIGHT];

typedef struct
{
  uint16_t TextColor;
  uint16_t BackColor;
  sFONT *pFont;
}LCD_DrawPropTypeDef;
LCD_DrawPropTypeDef lcdprop;

//************************************************************************//
void lcd7735_reset()
{
    gpio_set_level (CONFIG_PIN_NUM_RESET, 0);
    vTaskDelay(10 / portTICK_RATE_MS);
    gpio_set_level(CONFIG_PIN_NUM_RESET, 1);
    vTaskDelay(10 / portTICK_RATE_MS);
}

//************************************************************************//
void WriteCmd (spi_device_handle_t spi, uint8_t cmd)
{  
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.length=8;                     //Command is 8 bits
	t.tx_buffer=&cmd;               //The data is the cmd itself
	t.user=(void*)0;                //D/C needs to be set to 0
	ret=spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret==ESP_OK);            //Should have had no issues.
}

//************************************************************************//
void WriteData (spi_device_handle_t spi, uint8_t *data, int len)
{  
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0)
    	return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}


//************************************************************************//
void init_lcd7735 (spi_device_handle_t spi, uint16_t w_size, uint16_t h_size)
{

	uint8_t data [16];
	gpio_set_level(CONFIG_PIN_NUM_CS, 0);
	lcd7735_reset();


	WriteCmd(spi, 0x01); //Software Reset
	vTaskDelay(100 / portTICK_RATE_MS);

	data[0] = 0x39; //Power control A, Vcore=1.6V, DDVDH=5.6V
	data[1] = 0x2C;
	data[2] = 0x00;
	data[3] = 0x34;
	data[4] = 0x02;
	WriteCmd (spi, 0xCB);
	WriteData (spi, data, 5);

	data[0] = 0x00;  //Power contorl B, power control = 0, DC_ENA = 1
	data[1] = 0x83;
	data[2] = 0x30;
	WriteCmd(spi, 0xCF);
	WriteData(spi, data, 3);

	data[0] = 0x85; //Driver timing control A, non-overlap=default +1, EQ=default - 1, CR=default, pre-charge=default - 1
	data[1] = 0x01;
	data[2] = 0x79;
	WriteCmd(spi, 0xE8);
	WriteData(spi, data, 3);

	data[0] = 0x00; //Driver timing control, all=0 unit
	data[1] = 0x00;
	WriteCmd(spi, 0xEA);
	WriteData(spi, data, 2);

	data[0] = 0x64;  //Power on sequence control, cp1 keeps 1 frame, 1st frame enable, vcl = 0, ddvdh=3, vgh=1, vgl=2, DDVDH_ENH=1
	data[1] = 0x03;
	data[2] = 0x12;
	data[3] = 0x81;
	WriteCmd(spi, 0xED);
	WriteData(spi, data, 4);

	data[0] = 0x20;  //Pump ratio control, DDVDH=2xVCl
	WriteCmd(spi, 0xF7);
	WriteData(spi, data, 1);

	data[0] = 0x26; //Power control 1, GVDD=4.75V
	WriteCmd(spi, 0xC0);
	WriteData(spi, data, 1);

	data[0] = 0x11; //Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3
	WriteCmd(spi, 0xC1);
	WriteData(spi, data, 1);

	data[0] = 0x35; //VCOM control 1, VCOMH=4.025V, VCOML=-0.950V
	data[1] = 0x3E;
	WriteCmd(spi, 0xC5);
	WriteData(spi, data, 2);

	data[0] = 0xBE; //VCOM control 2, VCOMH=VMH-2, VCOML=VML-2
	WriteCmd(spi, 0xC7);
	WriteData(spi, data, 1);

	data[0] = 0x28; //Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0
	WriteCmd(spi, 0x36);
	WriteData(spi, data, 1);

	data[0] = 0x55; //Pixel format, 16bits/pixel for RGB/MCU interface
	WriteCmd(spi, 0x3A);
	WriteData(spi, data, 1);

	data[0] = 0x00;  //Frame rate control, f=fosc, 70Hz fps
	data[1] = 0x1B;
	WriteCmd(spi, 0xB1);
	WriteData(spi, data, 2);

	data[0] = 0x08;  //Display function control
	data[1] = 0x82;
	data[2] = 0x27;
	WriteCmd(spi, 0xB6);
	WriteData(spi, data, 3);

	data[0] = 0x08; //Enable 3G, disabled
	WriteCmd(spi, 0xF2);
	WriteData(spi, data, 1);

	data[0] = 0x01; //Gamma set, curve 1
	WriteCmd(spi, 0x26);
	WriteData(spi, data, 1);

	data[0] = 0x0F; //Positive gamma correction
	data[1] = 0x31;
	data[2] = 0x2B;
	data[3] = 0x0C;
	data[4] = 0x0E;
	data[5] = 0x08;
	data[6] = 0x4E;
	data[7] = 0xF1;
	data[8] = 0x37;
	data[9] = 0x07;
	data[10] = 0x10;
	data[11] = 0x03;
	data[12] = 0x0E;
	data[13] = 0x09;
	data[14] = 0x00;
	WriteCmd(spi, 0xE0);
	WriteData(spi, data, 15);

	data[0] = 0x00;  //Negative gamma correction
	data[1] = 0x0E;
	data[2] = 0x14;
	data[3] = 0x03;
	data[4] = 0x11;
	data[5] = 0x07;
	data[6] = 0x31;
	data[7] = 0xC1;
	data[8] = 0x48;
	data[9] = 0x08;
	data[10] = 0x0F;
	data[11] = 0x0C;
	data[12] = 0x31;
	data[13] = 0x36;
	data[14] = 0x0F;
	WriteCmd(spi, 0xE1);
	WriteData(spi, data, 15);

	data[0] = 0x00; //Column address set, SC=0, EC=0xEF
	data[1] = 0x00;
	data[2] = 0x00;
	data[3] = 0xEF;
	WriteCmd(spi, 0x2A);
	WriteData(spi, data, 4);

	data[0] = 0x00; //Page address set, SP=0, EP=0x013F
	data[1] = 0x00;
	data[2] = 0x01;
	data[3] = 0x3F;
	WriteCmd(spi, 0x2B);
	WriteData(spi, data, 4);

	WriteCmd(spi, 0x2C); //Memory write

	data[0] = 0x07; //Entry mode set, Low vol detect disabled, normal display
	WriteCmd(spi, 0xB7);
	WriteData(spi, data, 1);

	WriteCmd(spi, 0x11); //Sleep out

	WriteCmd(spi, 0x29);  //Display on
	//TFT9341_WIDTH = w_size;
	//TFT9341_HEIGHT = h_size;
}

//************************************************************************//
void st7735SetAddrWindow(spi_device_handle_t spi, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	WriteCmd(spi,ST7735_CASET);   // column addr set
	WriteData(spi, 0x00, 1);
	WriteData(spi, x0, 1);          // XSTART
	WriteData(spi, 0x00, 1);
	WriteData(spi, x1, 1);          // XEND

	WriteCmd (spi,ST7735_RASET);   // row addr set
	WriteData(spi, 0x00, 1);
	WriteData(spi, y0, 1);          // YSTART
	WriteData(spi, 0x00, 1);
	WriteData(spi, y1, 1);          // YEND
}

//************************************************************************//
void lcdDrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	x=y*ST7735_PANEL_WIDTH+x;
	Lcd_buf[x]= (unsigned char) color;	
	
}

//************************************************************************//
void LCD_Refresh(spi_device_handle_t spi)
{  	
	int x, y;
	
	st7735SetAddrWindow(spi, 0, 0, lcdGetWidth() - 1, lcdGetHeight() - 1);
	WriteCmd (spi,ST7735_RAMWR);  // write to RAM
	for (x=0; x < (ST7735_PANEL_WIDTH*ST7735_PANEL_HEIGHT); x++)
	{
		y=FindColor(Lcd_buf[x]);
		WriteData(spi,y >> 8,1);
		WriteData(spi,y,1);
	}
	WriteCmd (spi,ST7735_NOP);
}

//************************************************************************//
// pictures were converted by https://littlevgl.com/image-to-c-array
void LCD_DrawBMP(spi_device_handle_t spi, const char* buf, int x0, int y0, int w, int h)
{  	
	int x, y;
	
	st7735SetAddrWindow (spi, x0+0, y0+0, x0+w-1, y0+h-1);
	WriteCmd (spi,ST7735_RAMWR);  // write to RAM
	for (x=0; x < (w*h*2); x++)
	{
		y =  buf[x]; 
		WriteData(spi,y,1);
	}
	WriteCmd (spi, ST7735_NOP);
}


//************************************************************************//
void lcdFillRGB (spi_device_handle_t spi, uint16_t color)
{
	uint16_t x;
	uint16_t color2;
	
	color2=color >> 8;
	st7735SetAddrWindow (spi, 0, 0, lcdGetWidth() - 1, lcdGetHeight() - 1);
	WriteCmd (spi,ST7735_RAMWR);  // write to RAM
	for (x=0; x < ST7735_PANEL_WIDTH*ST7735_PANEL_HEIGHT; x++)
	{
	  WriteData (spi,color2,1);
	  WriteData (spi,color,1);
 	}
	WriteCmd (spi,ST7735_NOP);
}


//************************************************************************//
void lcdDrawHLine(uint16_t x0, uint16_t x1, uint16_t y, uint16_t color)
{
	uint16_t x, pixels;

	if (x1 < x0)
	{
		x = x1;
		x1 = x0;
		x0 = x;
	}

	if (x1 >= 160) {	// Check limits
    x1 = 159;}

	if (x0 >= 160) {
    x0 = 159;}
	
	for (pixels = x0; pixels < x1+1; pixels++)
	{
		lcdDrawPixel(pixels,y,color);
	}
}

//************************************************************************//
void lcdDrawVLine(uint16_t x, uint16_t y0, uint16_t y1, uint16_t color)
{
	uint16_t y, pixels;

	if (y1 < y0)
	{
		y = y1;
		y1 = y0;
		y0 = y;
	}

	if (y1 >= 128) {	// Check limits
		y1 = 127;}
	if (y0 >= 128) {
		y0 = 127;}

	for (pixels = y0; pixels < y1 + 1; pixels++)
	{
		lcdDrawPixel(x,pixels,color);
	}
}

//************************************************************************//
void LcdDrawRectangle(uint16_t x0,uint16_t x1,uint16_t y0,uint16_t y1,uint16_t color)
{
	uint16_t x,y;
	
	if(x0>x1)
	{
		x=x0;
		x0=x1;
		x1=x;
	}
	if(y0>y1)
	{
		y=y0;
		y0=y1;
		y1=y;
	}
	if(x0>=160) { x0 = 159;}
	if(x1>=160) { x1 = 159;}
	if(y0>=128) { y0 = 127;}
	if(y1>=128) { y1 = 127;}
	
	for(x=x0;x<=x1;x++)
	{
		for(y=y0;y<y1;y++)
		{
			lcdDrawPixel(x,y,color);
		}
	}
}

//************************************************************************//
void LcdDrawGraphSimple(unsigned int *buf, unsigned int color)
{
	unsigned int i;
	signed int tmp;
	for(i=0;i<22;i++) {
		tmp=124-buf[i]/1;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(4+7*i,10+7*i,124,tmp,color);
	}
}

//************************************************************************//
void LcdDrawGraph(unsigned int *bufLow,unsigned int *bufMiddle, unsigned int *bufHigh)
{
	unsigned int i;
	signed int tmp;
	
	for(i=0;i<22;i++)
	{
		tmp=124-bufMiddle[i]/8;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(4+7*i,10+7*i,124,tmp,blue);
		
		tmp=124-bufLow[i]/8;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(5+7*i,9+7*i,tmp,tmp+2,green);
		
		tmp=124-bufHigh[i]/8;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(5+7*i,9+7*i,tmp,tmp+2,red);
	}
}

//************************************************************************//
void LcdDrawUvGraph(unsigned int Low,unsigned int Middle, unsigned int High)
{
	signed int tmp;
	
	tmp=124-Middle/8;
	if(tmp < 0) { tmp = 0;}
	LcdDrawRectangle(70,90,124,tmp,blue);
	
	tmp=124-Low/8;
	if(tmp < 0) { tmp = 0;}
	LcdDrawRectangle(70,90,tmp,tmp+2,green);
	
	tmp=124-High/8;
	if(tmp < 0) { tmp = 0;}
	LcdDrawRectangle(70,90,tmp,tmp+2,red);
}

//************************************************************************//
void LcdDrawASGraph(unsigned int left,unsigned int right)
{
	signed int tmp;
	
	tmp=123-left/4;
	if(tmp < 24) { tmp = 24;}
	LcdDrawRectangle(40,60,124,tmp,blue);
	
	tmp=123-right/4;
	if(tmp < 24) { tmp = 24;}
	LcdDrawRectangle(100,120,124,tmp,blue);
}

//************************************************************************//
void LcdDrawMgGraph(int *buf, int low, int high)
{
	signed int tmp;
	int i;
	
	for(i=0;i<6;i++)
	{
		tmp=123-buf[i]/3;
		if(tmp < 24) { tmp = 24;}
		LcdDrawRectangle(20+20*i,38+20*i,124,tmp,blue);
		
		tmp=124-high/3;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(10,150,tmp,tmp+2,red);
		
		tmp=124-low/3;
		if(tmp < 0) { tmp = 0;}
		LcdDrawRectangle(10,150,tmp,tmp+2,green);
	}
}

//************************************************************************//
void lcdSetOrientation(spi_device_handle_t spi, unsigned char orientation)
{
  switch (orientation)
	{
		case 0:
			WriteCmd (spi,ST7735_MADCTL);  // Memory Data Access Control
			WriteData(spi, 0x60, 1);
	    break;    
		
		case 1:
			WriteCmd (spi,ST7735_MADCTL);  // Memory Data Access Control
			WriteData(spi, 0xC0, 1);          // 101 - X-Y Exchange, Y-Mirror
			break;
		
		case 2:
			WriteCmd (spi,ST7735_MADCTL);  // Memory Data Access Control
			WriteData(spi,0x00,1);           // 000 - Normal
			break;
				
		default:
			break;
	}
}

//************************************************************************//
uint16_t lcdGetWidth(void)
{
  return ST7735_PANEL_WIDTH;
}

//************************************************************************//
uint16_t lcdGetHeight(void)
{
  return ST7735_PANEL_HEIGHT;
}

//************************************************************************//
void LCD_SetFont(const unsigned char * font, uint32_t color)
{
 	GlobalFont=font;
	Paint_Color = color;
}

//************************************************************************//
uint32_t LCD_FastShowChar(uint32_t x,uint32_t y,uint8_t num)
{        
  uint8_t tmp;
  uint16_t tmp2,tmp3,tmp4;
  uint32_t dy,i;
  uint32_t tmpMaxWidth = 0, maxWidth = 0;
  uint32_t symbolHeight, symbolLeghth, symbolByteWidth;
	 
   if(x>ST7735_PANEL_WIDTH||y>ST7735_PANEL_HEIGHT)return 0;

	if (num == ' ') { return 10; }		  	// special case - " " symbol doesn't have any width
	if (num < ' ') return 0;	            					
	else num=num-' ';

	symbolByteWidth = GlobalFont[0];	 	
	symbolHeight =  GlobalFont[2];
	symbolLeghth = (GlobalFont[0])*(GlobalFont[2]);
	//symbolLeghth =  GlobalFont[3];

 	for(dy=0;dy<symbolHeight;dy++) 
	{
		tmp4=y+dy;
		for(i=0;i<symbolByteWidth;i++)
		{
			tmp2=i*8;
			tmp3=x+i*8;
			tmp = GlobalFont[num*symbolLeghth + dy*symbolByteWidth+i]; 
			
			if (tmp&0x80) {lcdDrawPixel(tmp3+0,tmp4,Paint_Color); tmpMaxWidth = tmp2+1; }
			if (tmp&0x40) {lcdDrawPixel(tmp3+1,tmp4,Paint_Color); tmpMaxWidth = tmp2+2; }
			if (tmp&0x20) {lcdDrawPixel(tmp3+2,tmp4,Paint_Color); tmpMaxWidth = tmp2+3; }
			if (tmp&0x10) {lcdDrawPixel(tmp3+3,tmp4,Paint_Color); tmpMaxWidth = tmp2+4; }
			if (tmp&0x08) {lcdDrawPixel(tmp3+4,tmp4,Paint_Color); tmpMaxWidth = tmp2+5; }
			if (tmp&0x04) {lcdDrawPixel(tmp3+5,tmp4,Paint_Color); tmpMaxWidth = tmp2+6; }
			if (tmp&0x02) {lcdDrawPixel(tmp3+6,tmp4,Paint_Color); tmpMaxWidth = tmp2+7; }
			if (tmp&0x01) {lcdDrawPixel(tmp3+7,tmp4,Paint_Color); tmpMaxWidth = tmp2+8; }

			if (tmpMaxWidth > maxWidth) { maxWidth = tmpMaxWidth; }
		}
 	}
	return (maxWidth+maxWidth/8+1);
}

//************************************************************************//
uint32_t LCD_GetCharWidth(uint32_t y,uint8_t num)
{        
  uint8_t tmp;
	uint16_t tmp2;
  uint32_t dy,i;
	uint32_t tmpMaxWidth = 0, maxWidth = 0;
	uint32_t symbolHeight, symbolLeghth, symbolByteWidth;
	 
	if(y>ST7735_PANEL_HEIGHT)return 0;

	if (num == ' ') { return 10; }		  	// special case - " " symbol doesn't have any width
	if (num < ' ') return 0;	            					
	else num=num-' ';

	symbolByteWidth = GlobalFont[0];	 	
	symbolHeight =  GlobalFont[2];
	symbolLeghth = (GlobalFont[0])*(GlobalFont[2]);
	//symbolLeghth =  GlobalFont[3];

 	for(dy=0;dy<symbolHeight;dy++) 
	{
		for(i=0;i<symbolByteWidth;i++)
		{
			tmp2=i*8;
			tmp = GlobalFont[num*symbolLeghth + dy*symbolByteWidth+i]; 
			
			if (tmp&0x80) {tmpMaxWidth = tmp2+1; }
			if (tmp&0x40) {tmpMaxWidth = tmp2+2; }
			if (tmp&0x20) {tmpMaxWidth = tmp2+3; }
			if (tmp&0x10) {tmpMaxWidth = tmp2+4; }
			if (tmp&0x08) {tmpMaxWidth = tmp2+5; }
			if (tmp&0x04) {tmpMaxWidth = tmp2+6; }
			if (tmp&0x02) {tmpMaxWidth = tmp2+7; }
			if (tmp&0x01) {tmpMaxWidth = tmp2+8; }

			if (tmpMaxWidth > maxWidth) { maxWidth = tmpMaxWidth; }
		}
 	}
	return (maxWidth+maxWidth/8+1);
}

//************************************************************************//
void LCD_ShowString(uint16_t x,uint16_t y, char *p)
{         
	while(*p!='\0')	{       
		if(x>=ST7735_PANEL_WIDTH){ x=0; y=y+GlobalFont[2]-1; }
		if(y>=ST7735_PANEL_HEIGHT){y=x=0;}
		x+=LCD_FastShowChar(x,y,*p);
		p++;
	}  
}

//************************************************************************//
void LCD_ShowStringSize(uint16_t x,uint16_t y, char *p,unsigned int size)
{         
	while(size--)	{       
		if(x>=ST7735_PANEL_WIDTH){ x=0; y=y+GlobalFont[2]-1; }
		if(y>=ST7735_PANEL_HEIGHT){y=x=0;}
		x+=LCD_FastShowChar(x,y,*p);
		p++;
	}  
}


//************************************************************************//
uint16_t FindColor (unsigned char color)
{
	if(color==black){return BLACK;}
	if(color==white){return WHITE;}	
	if(color==green){return GREEN;}	
	if(color==red){return RED;}	
	if(color==blue){return BLUE;}
	if(color==yellow){return YELLOW;}
	if(color==grey){return GRAY;}
	return 0;
}

//************************************************************************//
void ClearLcdMemory(void)
{
	int tmp=0;
	for(tmp=0;tmp<20480;tmp++)
	{
		Lcd_buf[tmp]=0xFF;
	}
}
	
//************************************************************************//
	
