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
void W25_Get_JEDEC_ID(w25q_t *dev)
{
  spi_transaction_t SPITransaction;
  uint8_t wdata[4];
  wdata [0] = GET_JEDEC_ID;
  wdata [1] = 0; wdata [2] = 0; wdata [3] = 0;
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 4 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );
  printf("JEDEC ID:0x%X 0x%X 0x%X\r\n", wdata[1], wdata[2], wdata[3]);
  return;
}

//==================================================
uint8_t W25_Read_SR(w25q_t *dev)
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
  return wdata[1];
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
}

//==================================================
void W25_Write_Page(w25q_t *dev)
{
  spi_transaction_t SPITransaction;
  uint8_t wdata [8];
  uint8_t status;
  wdata[0] =  WRITE_EN; //разрешение на запись
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 1 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );

  wdata[0] =  CHIP_ERASE; //стирание чипа
  wdata[1] = 0x0; wdata[2] = 0x0; wdata[3] = 0x0;
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 4 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );

  while (((status = W25_Read_SR(dev)) & 0x1) != 0) {}
 // vTaskDelay(50 / portTICK_PERIOD_MS);

  wdata[0] =  PAGE_PROGRAM; //запись
  wdata[1] = 0x00; wdata[2] = 0x00; wdata[3] = 0x00; //адрес страницы
  wdata[4] = 0x0C; wdata[5] = 0x0A; wdata[6] = 0x0F;  wdata[7] = 0x0E; //данные для записи
  memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
  SPITransaction.length = 8 * 8;
  SPITransaction.tx_buffer = wdata;
  SPITransaction.rx_buffer = wdata;
  spi_device_transmit(dev->spi_dev, &SPITransaction );

   wdata[0] =  WRITE_DIS; //запрет на запись
   memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
   SPITransaction.length = 1 * 8;
   SPITransaction.tx_buffer = wdata;
   SPITransaction.rx_buffer = wdata;
   spi_device_transmit(dev->spi_dev, &SPITransaction );
  return;
}

//==================================================
void W25_Read_Page(w25q_t *dev, uint8_t * buffer, uint8_t len)
{
	spi_transaction_t SPITransaction;
	uint8_t wdata [4+len];
	wdata[0] =  READ_DATA;
	wdata[1] = 0x00; wdata[2] = 0x00; wdata[3] = 0x00;
	memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
	SPITransaction.length = (4+len) * 8;
	SPITransaction.tx_buffer = wdata;
	SPITransaction.rx_buffer = wdata;
	spi_device_transmit(dev->spi_dev, &SPITransaction );

	//printf("data: 0x%X 0x%X 0x%X 0x%X\r\n", wdata[4], wdata[5], wdata[6], wdata[7]);
	for (uint8_t count = 0; count < len; count++)
	{
		*(buffer+count) =  wdata[4+count];
	}
	return ;
}

