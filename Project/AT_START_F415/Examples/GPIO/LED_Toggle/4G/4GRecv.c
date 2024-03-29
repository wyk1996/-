/*****************************************Copyright(C)******************************************
*******************************************杭州汇誉*********************************************
*------------------------------------------文件信息---------------------------------------------
* FileName			: GPRSMain.c
* Author			:
* Date First Issued	:
* Version			:
* Description		:
*----------------------------------------历史版本信息-------------------------------------------
* History			:
* //2010		    : V
* Description		:
*-----------------------------------------------------------------------------------------------
***********************************************************************************************/
/* Includes-----------------------------------------------------------------------------------*/
#include "4GMain.h"
#include "4GRecv.h"
#include <string.h>
#include "common.h"
#include "dwin_com_pro.h"
#include "main.h"
#include "flashdispos.h"
/* Private define-----------------------------------------------------------------------------*/

/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/
_RECV_DATA_CONTROL RecvDataControl[LINK_NET_NUM];
OS_Q Recv4GMq;

/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/
//专门做GPRS接收处理


/*****************************************************************************
* Function     : APP_RecvDataControl
* Description  :
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月14日
*****************************************************************************/
_RECV_DATA_CONTROL	* APP_RecvDataControl(uint8_t num)
{
    if(num >= NetConfigInfo[NET_YX_SELCT].NetNum)
    {
        return NULL;
    }
    return &RecvDataControl[num];
}

extern uint8_t count4G;
//专门做GPRS接收处理
static uint8_t recvbuf[URART_4GRECV_LEN];
uint32_t HYCOUNT = 0;
uint32_t HYOUNT = 0;
uint32_t NetCount = 0;
/*****************************************************************************
* Function     : TaskGPRSRecv
* Description  : 串口测试任务
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月16日
*****************************************************************************/
void AppTask4GRecv(void *pdata)
{
    OS_ERR ERR;
    uint8_t err;
    uint16_t i;
    int iRxLen = 0;
    MQ_MSG_T stMsg = {0};
    //连续10次未收到数据，则主动读取数据,连续40次则重启
    static uint32_t NetRset[LINK_NET_NUM] = {0};
    extern MQ_MSG_T    st4GMsg[MSQ4GLEN];

    static uint32_t lastSysTime[LINK_NET_NUM] = {0};
    static uint32_t nowSysTime[LINK_NET_NUM] = {0};
    OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &timeerr);
    UART14Ginit();
    OSQCreate (&Recv4GMq,
               "4G send mq",
               20,
               &ERR);
    if(ERR != OS_ERR_NONE)
    {
        printf("OSQCreate %s Fail", "4G send mq");
        return;
    }
    if(DisSysConfigInfo.standaloneornet != DISP_NET)
    {
        return;
    }
    for(i = 0; i < NetConfigInfo[NET_YX_SELCT].NetNum; i++)
    {
        nowSysTime[i] = OSTimeGet(&timeerr);
        lastSysTime[i] = nowSysTime[i];
    }
    while(1)
    {
        //从串口读取一个消息GPRSTempBuf[GPRS_TEMP_BUF_LEN]
        for(i = 0; i < NetConfigInfo[NET_YX_SELCT].NetNum; i++)
        {
            nowSysTime[i] = OSTimeGet(&timeerr);
        }
        if(NetConfigInfo[NET_YX_SELCT].NetNum  == 0)
        {
            iRxLen = 0;
        }

        static uint32_t zonglen = 0;  //这个下载的数据总长度
        static uint8_t ftpdownnum = 1,ftpbuf = 0;
        static uint32_t startdowntime = 0,Enddowntime = 0;
        static  uint8_t   buff[5]= {0};
        buff[0] = 0x55; //升级的标志位



//在ftp的模式下，如果5分钟没有重启，强制重启
#if(FTPUPDATA_FLAG)
        if(APP_GetSIM7600Mode() == MODE_FTP)
        {
            if(Enddowntime == 0)
            {
                startdowntime = OSTimeGet(&timeerr);
                Enddowntime =1;
            }
            if((OSTimeGet(&timeerr) - startdowntime) > (CM_TIME_60_SEC * 5))
            {
                SystemReset();//系统复位
            }
        }
#endif

        if(mq_service_recv_msg(&Recv4GMq,&stMsg,recvbuf,sizeof(recvbuf),CM_TIME_1_SEC) == 0 )
        {

#if(FTPUPDATA_FLAG)
            //开始下载升级
            if((FTPInfo.FTPDownloadflag == 1) && (APP_GetSIM7600Mode() == MODE_FTP))
            {
                if( count4G == 0)    //在post里面 count4G已经加1
                {
                    ftpbuf = MSQ4GLEN - 1;
                }
                else
                {
                    ftpbuf = count4G - 1;
                }
                fal_partition_write(APP_CODE,1024*ftpdownnum,puc4GBuf[ftpbuf],st4GMsg[ftpbuf].uiLoadLen);   // 一次一次存储 //存储时，位置不能从第5个，从第5个存储会少
                zonglen = zonglen + st4GMsg[ftpbuf].uiLoadLen;  //得到数据的总长度

                if(zonglen == FTPInfo.FTPupadatSize)
                {
                    FTPInfo.FTPDownloadflag = 0;   //不存储远程升级下载区域
                    Send_AT_ftploginout();     //退出ftp

                    memcpy(&buff[1],(uint8_t *)&FTPInfo.FTPupadatSize,4);   //强制转换数据长度
                    fal_partition_write(APP_CODE,0,buff,5);       // 最后直接写入
                    OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
                    JumpToProgramCode();   //开始跳转升级
                }
                else if(zonglen > FTPInfo.FTPupadatSize)  //大于
                {
                    FTPInfo.FTPDownloadflag = 0; //不存储远程升级下载区域
                    Send_AT_ftploginout(); //退出ftp
                    SystemReset();//系统复位   //升级失败 跳到重启
                }

//                if(ftpdownnum == 1)
//                {
//                    startdowntime = OSTimeGet(&timeerr);  //下载的开始时间
//                }
//                Enddowntime = OSTimeGet(&timeerr);  //下载的开始时间
                ftpdownnum++;
            }
#endif



            if(stMsg.uiSrcMoudleId == CM_4GUARTRECV_MODULE_ID)
            {
                //如果已经连接上服务器
                if (NetConfigInfo[NET_YX_SELCT].NetNum == 1)
                {
                    if(APP_GetModuleConnectState(0) == STATE_OK) //已经连接上后台了
                    {
                        //					if((GPRSTempBuf[0] == 0x0d) && (GPRSTempBuf[1] == 0x0a) )
                        //					{
                        //						APP_SetNetNotConect(0);   //调试的时候出现0d 0a
                        //					}
                        //
#if(UPDATA_STATE)
                        if(APP_GetSIM7600Mode() == MODE_HTTP)
                        {
                            SIM7600_RecvDesposeCmd(&recvbuf[0],stMsg.uiLoadLen); //未连接上服务器，AT指令处理
//						GPRSTempBuf[2000] = '\0';  //防止无线打印
//						rt_kprintf("rx %s\r\n",GPRSTempBuf);
                        }
                        else
                        {

                            if(_4G_RecvFrameDispose(&recvbuf[0],stMsg.uiLoadLen))  //数据透传
                            {
                                lastSysTime[0] = nowSysTime[0];
                            }
                        }
#else
                        if(_4G_RecvFrameDispose(&recvbuf[0],stMsg.uiLoadLen))  //数据透传
                        {
                            lastSysTime[0] = nowSysTime[0];
                        }
#endif

#if(USE_645 == 0)
                        printf("houtai-RECV len:%d,rx data:",stMsg.uiLoadLen);
                        recvbuf[stMsg.uiLoadLen] = '\0';  //防止无线打印
                        if(stMsg.uiLoadLen < 300)
                        {
                            for(i = 0; i < stMsg.uiLoadLen; i++)
                            {
                                printf("%02X ",recvbuf[i]);
                            }
                        }else
						{
							printf("len>300budayin:%d\r\n",stMsg.uiLoadLen);
						}
                        printf("\r\n");
#endif
                        memset(recvbuf,0,sizeof(recvbuf));
                    }
                    else
                    {
                        SIM7600_RecvDesposeCmd(&recvbuf[0],stMsg.uiLoadLen); //未连接上服务器，AT指令处理
#if(USE_645 == 0)
                        if((stMsg.uiLoadLen < 100) && (FTPInfo.FTPDownloadflag == 0))  //开始下载时，不要打印
                        {
                            recvbuf[URART_4GRECV_LEN - 1] = '\0';  //防止无线打印
                            recvbuf[stMsg.uiLoadLen] = '\0';
                            printf("12rx: %s\r\n",recvbuf);
                        }
#endif
                    }
                }
                else
                {
                    //未连接上服务器，AT指令处理
                    SIM7600_RecvDesposeCmd(&recvbuf[0],stMsg.uiLoadLen);
                    for(i = 0; i < NetConfigInfo[NET_YX_SELCT].NetNum; i++)
                    {
                        if(RecvDataControl[i].RecvStatus == RECV_FOUND_DATA)
                        {
                            RecvDataControl[i].RecvStatus = RECV_NOT_DATA;
                            //接收数据处理

                            //临时接收什么发送什么
                            //ModuleSIM7600_SendData(i,RecvDataControl[i].DataBuf,RecvDataControl[i].len);
                            if(i == 0)
                            {
                                HYOUNT++;
                                if(_4G_RecvFrameDispose(RecvDataControl[i].DataBuf,RecvDataControl[i].len))
                                {
                                    lastSysTime[i] = nowSysTime[i];
                                    HYCOUNT++;
                                }
                            }
                            else
                            {
                                if(RecvDataControl[i].DataBuf[0] == 0x68)    //简单判读下
                                {
                                    //政府平台,有数据返回就是注册成功
                                    APP_SetAppRegisterState(1,STATE_OK);
                                    lastSysTime[i] = nowSysTime[i];
                                }
                            }
                            NetRset[i] = 0;
                        }
                    }
                }
            }
        }

        for(i = 0; i < NetConfigInfo[NET_YX_SELCT].NetNum; i++)
        {
            if(APP_GetModuleConnectState(i) == STATE_OK) //已经连接上后台了
            {
                if (NetConfigInfo[NET_YX_SELCT].NetNum > 1)
                {
                    if(++NetRset[i] >= 13)  // 15s
                    {
                        NetRset[i] = 0;
                        if(i == 0)
                        {
                            mq_service_send_to_4gsend(BSP_4G_SENDNET1,GUN_A ,0 ,NULL);
                        }

                    }
                }
//
                //重启时间根据周期性发送数据再放余量
#if(NET_YX_SELCT == XY_AP)
                {
                    if((nowSysTime[i] >= lastSysTime[i]) ? ((nowSysTime[i] - lastSysTime[i]) >= CM_TIME_60_SEC) : \
                            ((nowSysTime[i] + (0xffffffff - lastSysTime[i])) >= CM_TIME_60_SEC))
                    {
                        lastSysTime[i] = nowSysTime[i];
                        APP_SetNetNotConect(i);
                    }
                }
#else
                {
                    if(((nowSysTime[i] >= lastSysTime[i]) ? ((nowSysTime[i] - lastSysTime[i]) >= CM_TIME_60_SEC) : \
                            ((nowSysTime[i] + (0xffffffff - lastSysTime[i])) >= CM_TIME_60_SEC)) && (APP_GetSIM7600Mode() == MODE_DATA))
                    {
                        lastSysTime[i] = nowSysTime[i];
                        APP_SetNetNotConect(i);

                        if(i == 0)
                        {
                            NetCount++;
                        }
                    }
                }
#endif

            }
            else
            {
                lastSysTime[i] = nowSysTime[i];
            }
        }
    }

}



/************************(C)COPYRIGHT 2020 杭州汇誉*****END OF FILE****************************/

