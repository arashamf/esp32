/**************************************************************************/

#ifndef MAIN_ST7735_H_
#define MAIN_ST7735_H_
//---------------------------------------------------------------------
//#include "main.h"
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"
#include "fonts.h"
//---------------------------------------------------------------------

//---------------------------------------------------------------------
#define swap(a,b) {int16_t t=a;a=b;b=t;}
//---------------------------------------------------------------------
#define MADCTL_MY  	0x80
#define MADCTL_MX  	0x40
#define MADCTL_MV  	0x20
#define MADCTL_ML  	0x10
#define MADCTL_RGB 	0x00
#define MADCTL_BGR	0x08
#define MADCTL_MH  	0x04
#define LCD_MADCTL    	0x36

#define BLACK  		0x0000
#define BROWN 		0XBC40
#define GRAY  		0X8430
#define BLUE    		0x001F
#define RED  		0xF800
#define GREEN  		0x07E0
#define CYAN   		0x07FF
#define MAGENTA 	0xF81F
#define YELLOW 	 	0xFFE0
#define WHITE   	0xFFFF
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458
#define LIGHTGREEN  0X841F
#define LGRAY 		0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12
//---------------------------------------------------------------------
void LCD_FillScreen(spi_device_handle_t spi, uint16_t color);
void LCD_FillRect(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawPixel(spi_device_handle_t spi, int x, int y, uint16_t color);
void LCD_DrawLine(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2);
void LCD_DrawRect(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2);
void LCD_DrawCircle(spi_device_handle_t spi, uint16_t x0, uint16_t y0, int r, uint16_t color);
void LCD_SetTextColor(uint16_t color);
void LCD_SetBackColor(uint16_t color);
void LCD_SetFont(sFONT *pFonts);
void LCD_DrawChar(spi_device_handle_t spi, uint16_t x, uint16_t y, uint8_t c);
void LCD_ShowString (spi_device_handle_t spi, uint16_t x,uint16_t y, char *str);
void LCD_SetRotation(spi_device_handle_t spi, uint8_t rotation);
void init_lcd7735 (spi_device_handle_t spi);
//---------------------------------------------------------------------


#endif /* MAIN_ST7735_H_ */
