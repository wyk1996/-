/*******************************************************************************
 * @file
 * @note
 * @brief
 *
 * @author
 * @date     2021-05-02
 * @version  V1.0.0
 *
 * @Description cp采样
 *
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning
 *******************************************************************************/
#include <stdlib.h>
#include "adc.h"


/* 3300mv */
#define   CP_ADC_REFER_VOLT     3300
/* 转换位数为12位 */
#define   CP_ADC_CONVERT_BITS   (1ul << 12)
#define   CP_ADC_MIN_CONVERT_V  1300
#define   CP_ADC_DIFF            700

void Swap(uint16_t *a,uint16_t *b) 
{
    int t;
    t=*a;
    *a=*b;
    *b=t;
}
//void BubbleSort(uint32_t * a,uint8_t n) 
//	{
//    int i,j;
//    for(i=1; i<n; i++) 
//		{
//        int flag=0;
//        for(j=1; j<=n-i; j++) 
//			{
//            if(a[j]>a[j+1]) 
//				{
//                Swap(&a[j],&a[j+1]);
//                flag=1;
//            }
//        }
//        if(flag==0) break;
//    }
//}

void BubbleSort(uint16_t * a,uint8_t n)
{
    int i,j;
    for(i=0; i<(n-1); i++)      //外层循环 最后一个只有一个元素（修改正确）
    {
        int flag=0;
		for(j=0; j<(n-1)-i; j++)   //内层循环
        {
            if(a[j]>a[j+1])
            {
                Swap(&a[j],&a[j+1]);
                flag=1;
            }
        }
        if(flag==0) break;
    }
}



/**
 * @brief 获取cp电压采样值
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint16_t adcbuf[100];
uint32_t ch_get_cp_volt(uint32_t *puiCpVolt)
{
    uint8_t i;
    uint8_t  count = 0;
    uint32_t alladc = 0;
    uint8_t  adccount = 0;

    for(i = 0; i < 100; i++)
    {
        /* 读取采样值 */
        if(ADCConvertedValue[i] < CP_ADC_DIFF)
        {
            continue;
        }
        adcbuf[count] = ADCConvertedValue[i];

        count++;
    }
    if(count < 5)
    {
        *puiCpVolt = 0;
        return 1;
    }
    BubbleSort(adcbuf,count);
    if(count > 40)
    {
        for(i = 10; i<count-10; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    else if(count > 20)
    {
        for(i = 5; i<count-5; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    else
    {
        for(i = 0; i<count; i++)
        {
            alladc += adcbuf[i];
            adccount++;
        }
        alladc /= adccount;
    }
    *puiCpVolt = alladc * 3.117 + 700;
    return count;
}









