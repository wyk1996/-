/**
  ******************************************************************************
  * File   : GPIO/LED_Toggle/at32f4xx_it.c
  * Version: V1.2.8
  * Date   : 2020-11-27
  * Brief  : Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  */


/* Includes ------------------------------------------------------------------*/
#include "at32f4xx_it.h"
#include "flashdispos.h"
#include "dwin_com_pro.h"
#include "string.h"


_siji_t sijit123;

//extern _siji_t sijit123;
/** @addtogroup AT32F415_StdPeriph_Examples
  * @{
  */

/** @addtogroup GPIO_LEDToggle
  * @{
  */

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
//====ye====
    uint32_t func;
    uint32_t mfunc;
    uint32_t psp = __get_PSP();
    uint32_t msp = __get_PSP();
    uint32_t *ppsp;
    uint32_t *pmsp;
    ppsp = (uint32_t *)psp;
    pmsp = 	(uint32_t *)msp;
    func = ppsp[5];
    mfunc = pmsp[5];

    fal_partition_read(HardFault_crash,0,(uint8_t*)&sijit123.NUM,4);
    if(sijit123.NUM == 0xFF)
    {
        sijit123.NUM = 0;
    }
    sijit123.NUM++; //读出后，条数加1

    fal_partition_write(HardFault_crash,4,(uint8_t*)&func,sizeof(func)); //把当前的地址写进
    fal_partition_write(HardFault_crash,8,(uint8_t*)&mfunc,sizeof(mfunc)); //把当前的地址写进
    fal_partition_write(HardFault_crash,0,(uint8_t*)&sijit123.NUM,1);//写入当前条数


    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {

    }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

///**
//  * @brief  This function handles SVCall exception.
//  * @param  None
//  * @retval None
//  */
//void SVC_Handler(void)
//{
//}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

///**
//  * @brief  This function handles PendSV_Handler exception.
//  * @param  None
//  * @retval None
//  */
//void PendSV_Handler(void)
//{
//}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
//void SysTick_Handler(void)
//{
//}

/******************************************************************************/
/*                 AT32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_at32f413_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */

/**
  * @}
  */

