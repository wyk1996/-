#ifndef _485_H
#define _485_H

#include "at32f4xx.h"

void  UART3init(void);
extern void Uart1_Init(uint32_t BaudRate, uint16_t DataBits, uint16_t Parity, uint16_t StopBits);
extern void Uart1_SendData(uint8_t *buf,uint8_t len);
void recive_485_test(void);
void UART3SENDBUF(uint8_t *buf,uint16_t len);

#endif



