/*====================================================================================================================================
//file:UART2.c
//name:mowenxing
//data:2021/03/23
//readme:
//===================================================================================================================================*/
#include "485_1.h"
#include "at32f4xx_rcc.h"
#include "main.h"
/*====================================================================================================================================*/
//#define UART3_RS485_EN_PIN   18   //PB2  485控制引脚

void  UART3init(void)
{
    GPIO_InitType GPIO_InitStructure;
    USART_InitType USART_InitStructure;
    NVIC_InitType NVIC_InitStructure;

    /* config USART3 clock */
    RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_USART3 , ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2PERIPH_GPIOB, ENABLE);


    /* USART3 GPIO config */
    /* Configure USART3 Tx  as alternate function push-pull */
    GPIO_InitStructure.GPIO_Pins = GPIO_Pins_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_MaxSpeed = GPIO_MaxSpeed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure USART3 Rx as input floating */
    GPIO_InitStructure.GPIO_Pins = GPIO_Pins_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 485控制引脚初始化 */
    GPIO_InitStructure.GPIO_Pins = GPIO_Pins_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB,PB2);  //low


    /* USART3 mode config */
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_Even;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    USART_INTConfig(USART3, USART_INT_RDNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;//
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    USART_Cmd(USART3, ENABLE);
}


_UART3_RECV_CONTORL Uart3RecvControl = {0};
/*====================================================================================================================================
//name:mowenxing data:   2021/03/23
//fun name:   USART2_IRQHandler
//fun work:     UART2??
//in: ?
//out:   ?
//ret:   ?
//ver: ?
//===================================================================================================================================*/
void USART3_RecvDispose(uint8_t ch)
{
    Uart3RecvControl.recv_buf[Uart3RecvControl.recv_index] = ch;
    Uart3RecvControl.lastrecvtime = OSTimeGet(&timeerr);
    Uart3RecvControl.recv_index++;
    if(Uart3RecvControl.recv_index >= MSG_RECV_LEN)
    {
        Uart3RecvControl.recv_index = 0;
    }
}

/*====================================================================================================================================
//name:mowenxing data:   2021/03/23
//fun name:   USART2_IRQHandler
//fun work:     UART2??
//in: ?
//out:   ?
//ret:   ?
//ver: ?
//===================================================================================================================================*/
void USART3_IRQHandler(void)
{
    uint8_t ch;
    CPU_SR  cpu_sr = 0;

    CPU_CRITICAL_ENTER();                                       /* Tell the OS that we are starting an ISR            */

    OSIntEnter();

    CPU_CRITICAL_EXIT();

    if(USART_GetITStatus(USART3, USART_INT_RDNE) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_INT_RDNE);
        ch = USART_ReceiveData(USART3);
        USART3_RecvDispose(ch);
    }
    OSIntExit();
}



/*====================从串口发送数据===============*/
void UART3SendByte(unsigned char IN)
{
    while((USART3->STS&0X40)==0);//????,??????
    USART3->DT = IN;
}


void UART3SENDBUF(uint8_t *buf,uint16_t  len)
{
    uint16_t  i;
    GPIO_SetBits(GPIOB,PB2);  //high
    delay_u(2);
//	   __set_PRIMASK(1);
    for(i=0; i<len; i++) {
        UART3SendByte(buf[i]);
    }
    while((USART3->STS&0X40)==0);  //最后一个发送完成
    GPIO_ResetBits(GPIOB,PB2);  //low
//		 __set_PRIMASK(0);
}


