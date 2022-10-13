#include "w25q_spi.h"
//==================================================
#define CLOCK_SPEED_HZ (10000000) // 10 MHz
//==================================================
//static const char *TAG = "w25q_spi";

//==================================================
void W25_Read_ID(w25q_t *dev)
{
  spi_transaction_t SPITransaction;
  uint8_t wdata[6];
//  uint16_t id = 0x0;
  wdata [0] = DEVICE_ID;
  wdata [1] = 0; wdata [2] = 0; wdata [3] = 0;
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 6 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );
  printf("ID:0x%X 0x%X \r\n", wdata[4], wdata[5]);
  return;
}

//==================================================
void W25_Read_SR(w25q_t *dev)
{
  spi_transaction_t SPITransaction;
  uint8_t wdata [2];
  wdata[0] =  READ_SR;
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 2 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );
  printf("StatusRegistr: 0x%X\r\n",wdata[1]);
  return;
}
//==================================================
void w25_ini (w25q_t *dev, spi_host_device_t host, gpio_num_t cs_pin)
{
  esp_err_t ret;
  spi_device_interface_config_t devcfg; //инициализация подключения устройства
  memset( &devcfg, 0, sizeof( spi_device_interface_config_t ) );
  devcfg.clock_speed_hz = CLOCK_SPEED_HZ;
  devcfg.spics_io_num = cs_pin;
  devcfg.queue_size = 7;
  devcfg.mode = 0;
  spi_device_handle_t handle;

  ret = spi_bus_add_device( HSPI_HOST, &devcfg, &handle);
  printf("spi_bus_add_device=%d\r\n",ret);
  dev->spi_dev = handle;

  vTaskDelay(100 / portTICK_PERIOD_MS);

// W25_Read_ID(dev);
// W25_Read_SR(dev);
 /* id &= 0x0000ffff;
  switch(id)
  {
    case 0x401A:
      w25_info.BlockCount=1024;
      printf("w25qxx Chip: w25q512\r\n");
      break;
    case 0x4019:
      w25_info.BlockCount=512;
      printf("w25qxx Chip: w25q256\r\n");
      break;
    case 0x4018:
      w25_info.BlockCount=256;
      printf("w25qxx Chip: w25q128\r\n");
      break;
    case 0x4017:
      w25_info.BlockCount=128;
      printf("w25qxx Chip: w25q64\r\n");
      break;
    case 0x4016:
      w25_info.BlockCount=64;
      printf("w25qxx Chip: w25q32\r\n");
      break;
    case 0x4015:
      w25_info.BlockCount=32;
      printf("w25qxx Chip: w25q16\r\n");
      break;
    case 0x4014:
      w25_info.BlockCount=16;
      printf("w25qxx Chip: w25q80\r\n");
      break;
    case 0x4013:
      w25_info.BlockCount=8;
      printf("w25qxx Chip: w25q40\r\n");
      break;
    case 0x4012:
      w25_info.BlockCount=4;
      printf("w25qxx Chip: w25q20\r\n");
      break;
    case 0x4011:
      w25_info.BlockCount=2;
      printf("w25qxx Chip: w25q10\r\n");
      break;
    default:
      printf("w25qxx Unknown ID\r\n");
      return;
  }
  w25_info.PageSize=256;
  w25_info.SectorSize=0x1000;
  w25_info.SectorCount=w25_info.BlockCount*16;
  w25_info.PageCount=(w25_info.SectorCount*w25_info.SectorSize)/w25_info.PageSize;
  w25_info.BlockSize=w25_info.SectorSize*16;
  w25_info.NumKB=(w25_info.SectorCount*w25_info.SectorSize)/1024;
  printf("Page Size: %d Bytes\r\n",(unsigned int)w25_info.PageSize);
  printf("Page Count: %u\r\n",(unsigned int)w25_info.PageCount);
  printf("Sector Size: %u Bytes\r\n",(unsigned int)w25_info.SectorSize);
  printf("Sector Count: %u\r\n",(unsigned int)w25_info.SectorCount);
  printf("Block Size: %u Bytes\r\n",(unsigned int)w25_info.BlockSize);
  printf("Block Count: %u\r\n",(unsigned int)w25_info.BlockCount);
  printf("Capacity: %u KB\r\n",(unsigned int)w25_info.NumKB);*/
}
//==================================================
