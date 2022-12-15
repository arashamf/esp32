#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_
//-------------------------------------------------------------
#include "main.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
//-------------------------------------------------------------
static void recv_task(void *pvParameters);
void udp_task(void *pvParameters);
//-------------------------------------------------------------
#endif /* MAIN_WIFI_H_ */
