#ifndef MAIN_W25Q_SPI_H_
#define MAIN_W25Q_SPI_H_
//==================================================
#include "main.h"
//==================================================
#define    W25_READ   0x03
#define    WRITE_EN   0x06
#define    WRITE_DIS  0x04
#define    W25_GET_JEDEC_ID  0x9f
#define	   READ_UN_ID 		 0x4B
#define	   DEVICE_ID 		 0x90
#define	   RELEASE 		 0xAB
#define    READ_SR       0x05

//==================================================
typedef struct
{
    spi_device_interface_config_t spi_cfg;
    spi_device_handle_t spi_dev;
} w25q_t;
//==================================================
typedef struct
{
  uint16_t  PageSize;
  uint32_t  PageCount;
  uint32_t  SectorSize;
  uint32_t  SectorCount;
  uint32_t  BlockSize;
  uint32_t  BlockCount;
  uint32_t  NumKB;
  uint8_t   SR1;
  uint8_t   SR2;
  uint8_t   SR3;
}w25_info_t;
//==================================================
void w25_ini (w25q_t *dev, spi_host_device_t host, gpio_num_t cs_pin);
void W25_Read_SR(w25q_t *dev);
void W25_Read_ID(w25q_t *dev);
//==================================================
#endif /* MAIN_W25Q_SPI_H_ */
