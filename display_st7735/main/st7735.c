#include "st7735.h"

LCD_DrawPropTypeDef lcdprop;

uint16_t LCD_WIDTH = 160;
uint16_t LCD_HEIGHT = 128;

//**********************************************************************************************************//
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
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

//**********************************************************************************************************//
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//**********************************************************************************************************//
void LCD_reset(void)
{
    gpio_set_level(CONFIG_PIN_NUM_RESET, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(CONFIG_PIN_NUM_RESET, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
}

//**********************************************************************************************************//
static void send_hor_blocks(spi_device_handle_t spi, int ypos, uint16_t *data)
{
  esp_err_t ret;
  int x;
  static spi_transaction_t trans[6];
  for (x=0; x<6; x++) {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x&1)==0) {
        //Even transfers are commands
        trans[x].length=8;
        trans[x].user=(void*)0;
    } else {
        //Odd transfers are data
        trans[x].length=8*4;
        trans[x].user=(void*)1;
    }
    trans[x].flags=SPI_TRANS_USE_TXDATA;
  }
  trans[0].tx_data[0]=0x2A;           //Column Address Set
  trans[1].tx_data[0]=0;              //Start Col High
  trans[1].tx_data[1]=0;              //Start Col Low
  trans[1].tx_data[2]=(LCD_WIDTH)>>8;       //End Col High
  trans[1].tx_data[3]=(LCD_WIDTH)&0xff;     //End Col Low
  trans[2].tx_data[0]=0x2B;           //Page address set
  trans[3].tx_data[0]=ypos>>8;        //Start page high
  trans[3].tx_data[1]=ypos&0xff;      //start page low
  trans[3].tx_data[2]=(ypos+16)>>8;    //end page high
  trans[3].tx_data[3]=(ypos+16)&0xff;  //end page low
  trans[4].tx_data[0]=0x2C;           //memory write
  trans[5].tx_buffer=data;        //finally send the line data
  trans[5].length=LCD_WIDTH*2*8*16;          //Data length, in bits
  trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag
  for (x=0; x<6; x++) {
      ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
      assert(ret==ESP_OK);
  }
}

//**********************************************************************************************************//
static void send_blocks(spi_device_handle_t spi, int x1, int y1,
    int x2, int y2, uint16_t *data)
{
  esp_err_t ret;
  int x;
  static spi_transaction_t trans[6];
  for (x=0; x<6; x++) {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x&1)==0) {
        //Even transfers are commands
        trans[x].length=8;
        trans[x].user=(void*)0;
    } else {
        //Odd transfers are data
        trans[x].length=8*4;
        trans[x].user=(void*)1;
    }
    trans[x].flags=SPI_TRANS_USE_TXDATA;
  }
  trans[0].tx_data[0]=0x2A;            //Column Address Set
  trans[1].tx_data[0]=(x1 >> 8) & 0xFF;//Start Col High
  trans[1].tx_data[1]=x1 & 0xFF;       //Start Col Low
  trans[1].tx_data[2]=(x2 >> 8) & 0xFF;//End Col High
  trans[1].tx_data[3]=x2 & 0xFF;       //End Col Low
  trans[2].tx_data[0]=0x2B;            //Page address set
  trans[3].tx_data[0]=(y1 >> 8) & 0xFF;//Start page high
  trans[3].tx_data[1]=y1 & 0xFF;       //start page low
  trans[3].tx_data[2]=(y2 >> 8) & 0xFF;//end page high
  trans[3].tx_data[3]=y2 & 0xFF;       //end page low
  trans[4].tx_data[0]=0x2C;           //memory write
  trans[5].tx_buffer=data;            //finally send the line data
  trans[5].length=(x2-x1+1)*(y2-y1+1)*2*8;//Data length, in bits
  trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag
  for (x=0; x<6; x++)
  {
      ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
      assert(ret==ESP_OK);
  }
}

//**********************************************************************************************************//
static void send_block_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x=0; x<6; x++)
    {
        ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret==ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

//**********************************************************************************************************//
void LCD_FillScreen(spi_device_handle_t spi, uint16_t color)
{
  uint16_t *blck;
  blck=heap_caps_malloc(LCD_WIDTH*16*sizeof(uint16_t), MALLOC_CAP_DMA); //swap bytes;

   color = color<<8 | color>>8;
   for(int y=0; y<16; y++)
   {
     for(int x=0; x<LCD_WIDTH; x++)
     {
       blck[y*LCD_WIDTH+x] = color;
     }
   }
   for (int i=0; i<LCD_HEIGHT/16; i++) {
     send_hor_blocks(spi, i*16, blck);
     send_block_finish(spi);
   }
   heap_caps_free(blck);
}

//**********************************************************************************************************//
void LCD_FillRect(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  if((x1 >= LCD_WIDTH) || (y1 >= LCD_HEIGHT) || (x2 >= LCD_WIDTH) || (y2 >= LCD_HEIGHT)) return;
  if(x1>x2) swap(x1,x2);
  if(y1>y2) swap(y1,y2);
  uint16_t xsize = (x2-x1+1), ysize = (y2-y1+1);
  uint16_t *blck;
  uint16_t ysize_block;
  uint32_t size_max = LCD_WIDTH*16;
  blck=heap_caps_malloc(size_max*sizeof(uint16_t), MALLOC_CAP_DMA);
  //swap bytes;
  color = color<<8 | color>>8;
  uint32_t size = xsize*ysize;
  uint32_t size_block;

  while(1)
  {
    if(size>size_max)
    {
      size_block = size_max - (size_max % xsize); //убираем остаток, чтобы полностью закрасилась линия
      size -= size_block;
      ysize_block = size_max / xsize;
      for(int y=0; y<ysize_block; y++)
      {
        for(int x=0; x<xsize; x++)
        {
          blck[y*xsize+x] = color;
        }
      }
      send_blocks(spi, x1, y1, x2, y1 + ysize_block - 1, blck);
      send_block_finish(spi);
      y1 += ysize_block;
    }
    else{
      ysize_block = size / xsize;
      for(int y=0; y<ysize_block; y++)
      {
        for(int x=0; x<xsize; x++)
        {
          blck[y*xsize+x] = color;
        }
      }
      send_blocks(spi, x1, y1, x2, y1 + ysize_block - 1, blck);
      send_block_finish(spi);
      break;
    }
  }
  heap_caps_free(blck);
}

//**********************************************************************************************************//
static void LCD_WriteData(spi_device_handle_t spi, uint8_t* buff, size_t buff_size)
{
  esp_err_t ret;
  spi_transaction_t t;
  while(buff_size > 0)
  {
    uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
    if (chunk_size==0)
    	return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=chunk_size*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=buff;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
    buff += chunk_size;
    buff_size -= chunk_size;
  }
}

//**********************************************************************************************************//
static void LCD_SetAddrWindow(spi_device_handle_t spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  // column address set
  lcd_cmd(spi, 0x2A); // CASET
  {
    uint8_t data[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
    LCD_WriteData(spi, data, sizeof(data));
  }
  // row address set
  lcd_cmd(spi, 0x2B); // RASET
  {
    uint8_t data[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
    LCD_WriteData(spi, data, sizeof(data));
  }
  // write to RAM
  lcd_cmd(spi, 0x2C); // RAMWR
}

//**********************************************************************************************************//
void LCD_DrawPixel(spi_device_handle_t spi, int x, int y, uint16_t color)
{
  uint8_t data[2];
  if((x<0)||(y<0)||(x>=LCD_WIDTH)||(y>=LCD_HEIGHT)) return;
  data[0] = color>>8;
  data[1] = color & 0xFF;
  LCD_SetAddrWindow(spi, x,y,x,y);
  lcd_cmd(spi, 0x2C);
  lcd_data(spi, data, 2);
}

//**********************************************************************************************************//
void LCD_DrawLine(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2)
{
  int steep = abs(y2-y1)>abs(x2-x1);
  if(steep)
  {
    swap(x1,y1);
    swap(x2,y2);
  }
  if(x1>x2)
  {
    swap(x1,x2);
    swap(y1,y2);
  }
  int dx,dy;
  dx=x2-x1;
  dy=abs(y2-y1);
  int err=dx/2;
  int ystep;
  if(y1<y2) ystep=1;
  else ystep=-1;
  for(;x1<=x2;x1++)
  {
    if (steep)
    	LCD_DrawPixel(spi, y1,x1,color);
    else
    	LCD_DrawPixel(spi, x1,y1,color);
    err-=dy;
    if(err<0)
    {
      y1 += ystep;
      err=dx;
    }
  }
}

//**********************************************************************************************************//
void LCD_DrawRect(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2)
{
  LCD_DrawLine(spi, color,x1,y1,x2,y1);
  LCD_DrawLine(spi, color,x2,y1,x2,y2);
  LCD_DrawLine(spi, color,x1,y1,x1,y2);
  LCD_DrawLine(spi, color,x1,y2,x2,y2);
}

//**********************************************************************************************************//
void LCD_DrawCircle(spi_device_handle_t spi, uint16_t x0, uint16_t y0, int r, uint16_t color)
{
  int f = 1-r;
  int ddF_x=1;
  int ddF_y=-2*r;
  int x = 0;
  int y = r;
  LCD_DrawPixel(spi, x0,y0+r,color);
  LCD_DrawPixel(spi, x0,y0-r,color);
  LCD_DrawPixel(spi, x0+r,y0,color);
  LCD_DrawPixel(spi, x0-r,y0,color);
  while (x<y)
  {
    if (f>=0)
    {
      y--;
      ddF_y+=2;
      f+=ddF_y;
    }
    x++;
    ddF_x+=2;
    f+=ddF_x;
    LCD_DrawPixel(spi, x0+x,y0+y,color);
    LCD_DrawPixel(spi, x0-x,y0+y,color);
    LCD_DrawPixel(spi, x0+x,y0-y,color);
    LCD_DrawPixel(spi, x0-x,y0-y,color);
    LCD_DrawPixel(spi, x0+y,y0+x,color);
    LCD_DrawPixel(spi, x0-y,y0+x,color);
    LCD_DrawPixel(spi, x0+y,y0-x,color);
    LCD_DrawPixel(spi, x0-y,y0-x,color);
  }
}

//**********************************************************************************************************//
void LCD_SetTextColor(uint16_t color)
{
  lcdprop.TextColor=color;
}

//**********************************************************************************************************//
void LCD_SetBackColor(uint16_t color)
{
  lcdprop.BackColor=color;
}

//**********************************************************************************************************//
void LCD_SetFont(sFONT *pFonts)
{
  lcdprop.pFont=pFonts;
}

//**********************************************************************************************************//
void LCD_DrawChar(spi_device_handle_t spi, uint16_t x, uint16_t y, uint8_t c)
{
  uint32_t i = 0, j = 0;
  uint16_t height, width;
  uint8_t offset;
  uint8_t *c_t;
  uint8_t *pchar;
  uint32_t line=0;
  height = lcdprop.pFont->Height;
  width  = lcdprop.pFont->Width;
  offset = 8 *((width + 7)/8) -  width ;
  c_t = (uint8_t*) &(lcdprop.pFont->table[(c-' ') * lcdprop.pFont->Height * ((lcdprop.pFont->Width + 7) / 8)]);
  for(i = 0; i < height; i++)
  {
    pchar = ((uint8_t *)c_t + (width + 7)/8 * i);
    switch(((width + 7)/8))
    {
      case 1:
          line =  pchar[0];
          break;
      case 2:
          line =  (pchar[0]<< 8) | pchar[1];
          break;
      case 3:
      default:
        line =  (pchar[0]<< 16) | (pchar[1]<< 8) | pchar[2];
        break;
    }
    for (j = 0; j < width; j++)
    {
      if(line & (1 << (width- j + offset- 1)))
      {
        LCD_DrawPixel(spi, (x + j), y, lcdprop.TextColor);
      }
      else
      {
        LCD_DrawPixel(spi, (x + j), y, lcdprop.BackColor);
      }
    }
    y++;
  }
}

//**********************************************************************************************************//
void LCD_ShowString(spi_device_handle_t spi, uint16_t x,uint16_t y, char *str)
{
  while(*str)
  {
    LCD_DrawChar(spi, x,y,str[0]);
    x+=(lcdprop.pFont->Width)-1;
    (void)*str++;
  }
}

//**********************************************************************************************************//
void LCD_SetRotation(spi_device_handle_t spi, uint8_t rotation)
{
  uint8_t data[1];
  lcd_cmd(spi, LCD_MADCTL);
  switch(rotation)
  {
    case 0:
      data[0] = 0x00;  // вертикальная ориентация
      lcd_data(spi, data, 1);
      LCD_WIDTH = 128;
      LCD_HEIGHT = 160;
      break;
    case 1:
      data[0] = 0xC0; //вертикальная ориентация, зеркальная
      lcd_data(spi, data, 1);
      LCD_WIDTH = 128;
      LCD_HEIGHT = 160;
      break;
    case 2:
      data[0] = 0x60; //горизонтальная ориентация
      lcd_data(spi, data, 1);
      LCD_WIDTH = 160;
      LCD_HEIGHT = 128;
      break;
  }
}

//**********************************************************************************************************//
void init_lcd7735 (spi_device_handle_t spi)
{

	 uint8_t data[15];

	 gpio_set_direction(CONFIG_PIN_NUM_DC, GPIO_MODE_OUTPUT);  //Initialize non-SPI GPIOs
	 gpio_set_direction(CONFIG_PIN_NUM_RESET, GPIO_MODE_OUTPUT);

	 LCD_reset();
	 lcd_cmd(spi, 0x01); //Software Reset
	 vTaskDelay(10 / portTICK_RATE_MS);

	lcd_cmd(spi, 0x26);  //выбор гамма-кривой для текущего отображения
	data[0] = 0x04;
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0xB1);  //Set Frame Rate
	data[0] = 0x0b;
	data[1] = 0x14;
	lcd_data(spi, data, 2);

	lcd_cmd(spi, 0xC0);  //Power_Control 1     //Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
	data[0] = 0x08;
	data[1] = 0x05;
	lcd_data(spi, data, 2);

	lcd_cmd(spi, 0xC1);  //Power_Control 2     //Set BT[2:0] for AVDD & VCL & VGH & VGL
	data[0] = 0x02;
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0xC5);  //Power_Control 3    //Set VMH[6:0] & VML[6:0] for VOMH & VCOML
	data[0] = 0x44;
	data[1] = 0x48;
	lcd_data(spi, data, 2);

	lcd_cmd(spi, 0xC7);  // Set VMF
	data[0] = 0xc2;
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0x3A);  // set color mode   Interface Pixel Format
	data[0] = 0x05; // 16-bit color
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0x2A);  //Set Column Address
	data[0] = 0x00; // XS [15..8]
	data[1] = 0x00; //XS [7..0]
	data[2] = 0x00; //XE [15..8]
	data[3] = 0x7F; //XE [7..0]
	lcd_data(spi, data, 4);

	lcd_cmd(spi, 0x2B);  //Set Page Address
	data[0] = 0x00; // YS [15..8]
	data[1] = 0x00;  //YS [7..0]
	data[2] = 0x00; //YE [15..8]
	data[3] = 0x9F; //YE [7..0]
	lcd_data(spi, data, 4);

	lcd_cmd(spi, 0x36);	 // Memory Data Access Control
	data[0] = 0xC8;  //!
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0xB7);  // Source Driver Direction
	data[0] = 0x00;
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0xF2); //Enable Gamma bit
	data[0] = 0x01;
	lcd_data(spi, data, 1);

	lcd_cmd(spi, 0xE0); //Positive Gamma Correction Setting
	data[0] = 0x3F;//p1
	data[1] = 0x25;//p2
	data[2] = 0x21;//p3
	data[3] = 0x24;//p4
	data[4] = 0x1d;//p5
	data[5] = 0x0d;//p6
	data[6] = 0x4c;//p7
	data[7] = 0xB8;//p8
	data[8] = 0x38;//p9
	data[9] = 0x17;//p10
	data[10] = 0x0f;//p11
	data[11] = 0x08;//p12
	data[12] = 0x04;//p13
	data[13] = 0x02;//p14
	data[14] = 0x00;//p15
	lcd_data(spi, data, 15);

	lcd_cmd(spi, 0xE1); //Negative Gamma Correction Setting
	data[0] = 0x00;//p1
	data[1] = 0x1a;//p2
	data[2] = 0x1e;//p3
	data[3] = 0x0b;//p4
	data[4] = 0x12;//p5
	data[5] = 0x12;//p6
	data[6] = 0x33;//p7
	data[7] = 0x47;//p8
	data[8] = 0x47;//p9
	data[9] = 0x08;//p10
	data[10] = 0x20;//p11
	data[11] = 0x27;//p12
	data[12] = 0x3c;//p13
	data[13] = 0x3d;//p14
	data[14] = 0x3f;//p15
	lcd_data(spi, data, 15);

	lcd_cmd(spi, 0x11);  //отключение спящего режима
	lcd_cmd(spi, 0x29); // Display On
//	WriteCmd(0x21); // Display inversion on
}

//**********************************************************************************************************//
