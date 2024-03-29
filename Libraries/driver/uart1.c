/*====================================================================================================================================
//file:UART2.c
//name:mowenxing
//data:2021/03/23
//readme:
//===================================================================================================================================*/
#include "uart1.h"
#include "at32f4xx_rcc.h"
#include "common.h"
#include "4GMain.h"

/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   UART14Ginit
//fun work：    4G串口初始化
//in： 无
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
void  UART14Ginit(void)
{
   GPIO_InitType GPIO_InitStructure;
  USART_InitType USART_InitStructure;
	NVIC_InitType NVIC_InitStructure;

	/* config USART2 clock */
	RCC_APB2PeriphClockCmd(RCC_APB2PERIPH_USART1 , ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2PERIPH_GPIOA, ENABLE);

	
	/* USART2 GPIO config */
	/* Configure USART2 Tx (PA.2) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pins = GPIO_Pins_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_MaxSpeed = GPIO_MaxSpeed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    
	/* Configure USART2 Rx (PA.3) as input floating */
	GPIO_InitStructure.GPIO_Pins = GPIO_Pins_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* USART2 mode config */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	
	USART_INTConfig(USART1, USART_INT_RDNE, ENABLE);
	
	//中断优先级设置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//从优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
  NVIC_Init(&NVIC_InitStructure);
	
	USART_Cmd(USART1, ENABLE);
}


_UART_4GRECV_CONTORL Uart1RecvControl = {0};
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   USART2_IRQHandler
//fun work：     UART2中断
//in： 无
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
 void USART1_RecvDispose(uint8_t ch)
 {
	 
	 Uart1RecvControl.recv_buf[Uart1RecvControl.recv_index] = ch;
	 Uart1RecvControl.lastrecvtime = OSTimeGet(&timeerr);
	 Uart1RecvControl.recv_index++;
	 if(Uart1RecvControl.recv_index >= URART_4GRECV_LEN)
	 {
		 Uart1RecvControl.recv_index = 0;
	 }

    static uint8_t Lendatabyte; //长度占用几个字节
    static uint16_t lendata = 0;    //lendata ==每一个数组长度
    static char lenbuf[5] = {0};    //接收的数据长度

    if(strncmp((char*)Uart1RecvControl.recv_buf,"\r\n+CFTPSGET: DATA,",strlen("\r\n+CFTPSGET: DATA,"))==0)
    {
        // 数据长度最长4个字节 最短1个字节，长度后，就是固定的0x0D 0x0A    等到接收的字节数量大于最多的长度时
        if(lendata == 0)
        {
            for(uint8_t i=1; i<5; i++)
            {
                if((Uart1RecvControl.recv_buf[strlen("\r\n+CFTPSGET: DATA,")+i] == 0x0D)&&(Uart1RecvControl.recv_buf[strlen("\r\n+CFTPSGET: DATA,")+i+1] == 0x0A))
                {
                    memcpy(lenbuf,&Uart1RecvControl.recv_buf[strlen("\r\n+CFTPSGET: DATA,")],4);  //第一位是逗号，开始第二位就是长度，长度可能是（1-4位）,统一写4个
                    lendata = atoi(lenbuf);    //获取的长度
                    break;
                }
            }
        }


        if(lendata > 0)
        {
            //长度占用几个字节
            if((9 < lendata) &&(lendata < 100))
            {
                Lendatabyte = 2;
            }
            else if((99<lendata) &&(lendata<1000))
            {
                Lendatabyte = 3;
            }
            else if(lendata > 999)
            {
                Lendatabyte = 4;
            }
            else
            {
                Lendatabyte = 1;
            }

            if(Uart1RecvControl.recv_index >= (strlen("\r\n+CFTPSGET: DATA,\r\n") + Lendatabyte + lendata))
            {
                mq_service_4GUart_send_recv(0,0,Uart1RecvControl.recv_index - (strlen("\r\n+CFTPSGET: DATA,\r\n") + Lendatabyte),&Uart1RecvControl.recv_buf[strlen("\r\n+CFTPSGET: DATA,\r\n") + Lendatabyte]);
                Uart1RecvControl.recv_index = 0; //接收的buf也是从0开始
                memset(Uart1RecvControl.recv_buf,0,25); //清空一下接收的数组 第二次不影响接收
                lendata = 0; //清空一下for循环
            }
        }
    }



    //最后一下数据接收完毕标志位
    if(strncmp((char*)Uart1RecvControl.recv_buf,"\r\n+CFTPSGET: 0",strlen("\r\n\r\n+CFTPSGET: 0"))==0)
    {
        printf("22Recv:%s",(char *)Uart1RecvControl.recv_buf);    //测试最后一次打印出来
        Uart1RecvControl.recv_index = 0; //接收的buf也是从0开始
        memset(Uart1RecvControl.recv_buf,0,25); //清空一下接收的数组 第二次不影响接收
		//FTPInfo.FTPDownloadUP = 1; //接收完成
    }


}
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   USART2_IRQHandler
//fun work：     UART2中断
//in： 无
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
 void USART1_IRQHandler(void)
{
	uint8_t ch;
	 CPU_SR  cpu_sr = 0;
	
    CPU_CRITICAL_ENTER();                                       /* Tell the OS that we are starting an ISR            */

    OSIntEnter();

    CPU_CRITICAL_EXIT();
	if(USART_GetITStatus(USART1, USART_INT_RDNE) != RESET)
	{ 	
		USART_ClearITPendingBit(USART1, USART_INT_RDNE);
			ch = USART_ReceiveData(USART1);
			USART1_RecvDispose(ch);
	} 
	OSIntExit();
}

/*====================================================================================================================================
//name：mowenxing data：   2021/03/23 
//fun name：   UART1SendByte
//fun work：     UART2发送一个BYTE
//in： IN：发送的单字节
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
void UART1SendByte(unsigned char IN)
{      
	while((USART1->STS&0X40)==0);//循环发送,直到发送完毕   
	USART1->DT = IN;  
}
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   UART2SENDBUF
//fun work：     UART2发送一个缓冲数据
//in： buf：数据包缓冲  len：数据包长度
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
void UART1SENDBUF(uint8 *buf,uint16  len){  
     uint16  i;
	 #warning "是否需要关总中断"
//	   __set_PRIMASK(1);  
	   for(i=0;i<len;i++){
			 UART1SendByte(buf[i]);
	   }
//		 __set_PRIMASK(0);
}


 
//END
