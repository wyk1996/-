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
#include <string.h>
#include "ModuleA7680C.h"
#include "dwin_com_pro.h"
#include "HYFrame.h"
#include "APFrame.h"
#include "YKCFrame.h"
#include "4GRecv.h"
#include "main.h"
#include "chtask.h"
extern CH_TASK_T stChTcb;
/* Private define-----------------------------------------------------------------------------*/
#define   GPRSMAIN_Q_LEN  								20
//临时IP端口放在这里，后面应该重屏幕下发读取

/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/
static void *GPRSMAINOSQ[GPRSMAIN_Q_LEN];					// 消息队列
/*static OS_EVENT *GPRSMainTaskEvent;				            // 使用的事件
OS_EVENT *SendMutex;                 //互斥锁，同一时刻只能有一个任务进行临界点访问*/
_START_NET_STATE StartNetState[GUN_MAX] = {NET_STATE_ONLINE};		//启动的时候状态
_NET_CONFIG_INFO  NetConfigInfo[XY_MAX] =
{
#if(show_HYXY_name == show_HY)
    {XY_HY,		{116,62,125,35},8000, {"116.62.125.35"},		1},
#elif(show_HYXY_name == show_JG)
    {XY_HY,		{114,55,126,89},8000, {"114.55.126.89"},		1},
#elif(show_HYXY_name == show_YC)
    {XY_HY,		{114,55,126,89},8111, {"114.55.126.89"},		1},  //显示上海益虫的IP程序里面固定写死,不能修改
#endif
#if(show_XY_name == show_YKC)
    {XY_YKC,	{121,43,69,62},8767,  {"121.43.69.62"},		   	1},  //显示YKC-IP
#elif(show_XY_name == show_XH)
    {XY_YKC,	{106,55,221,146},5592, {"106.55.221.146"} ,		1},  //显示小鹤IP
#elif(show_XY_name == show_SY)
    {XY_YKC,	{81,70,61,150},50000, {"81.70.61.150"} ,	   1},  //显示塑云IP
#elif(show_XY_name == show_TD)
    {XY_YKC,	{121,36,64,61},8767, {"121.36.64.61"} ,		   1},  //显示铁塔IP
#elif(show_XY_name == show_KL)
    {XY_YKC,	{47,99,136,141},18063, {"47.99.136.141"} ,		   1},  //显示库伦IP
#endif
    {XY_AP,		{114,55,186,206},5738, {"114.55.186.206"}  ,	1},
};






_RESEND_BILL_CONTROL ResendBillControl[GUN_MAX] = { {0}};
OS_MUTEX  sendmutex;
/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/

_HTTP_INFO HttpInfo = {0};
_FTP_INFO FTPInfo = {0};
#if(UPDATA_STATE)
/*****************************************************************************
* Function     : APP_SetUpadaState
* Description  :设置升级是否成功   0表示失败   1表示成功
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
void APP_SetUpadaState(uint8_t state)
{
#if(NET_YX_SELCT == XY_YKC)
    {
        APP_SetYKCUpadaState(state);
    }
#endif
}
#endif

/*****************************************************************************
* Function     : APP_SetResendBillState
* Description  : 设置是否重发状态
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
void APP_SetResendBillState(uint8_t gun,uint8_t state)
{
    if(gun >= GUN_MAX)
    {
        return;
    }
    ResendBillControl[gun].ResendBillState = state;
    ResendBillControl[gun].SendCount = 0;
}

uint8_t   APP_GetStartNetState(uint8_t gun)
{
    if(gun >= GUN_MAX)
    {
        return _4G_APP_START;
    }
    return (uint8_t)StartNetState[gun];
}
//桩上传结算指令
/*****************************************************************************
* Function     : ReSendBill
* Description  : 重发订单
* Input        : void *pdata  ifquery: 1 查询  0：重复发送
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t  ReSendBill(_GUN_NUM gun,uint8_t* pdata,uint8_t ifquery)
{

    if(pdata == NULL)
    {
        return FALSE;
    }
    ResendBillControl[gun].CurTime = OSTimeGet(&timeerr);		//获取当前时间
    if(ifquery)
    {
#if(NET_YX_SELCT == XY_HY)
        {
            return HY_SendBillData(pdata,250,ifquery);
        }
#endif
        return TRUE;
    }
    if(ResendBillControl[gun].ResendBillState == FALSE)
    {
        return FALSE;			//不需要重发订单
    }
    if((ResendBillControl[gun].CurTime - ResendBillControl[gun].LastTime) >= CM_TIME_10_SEC)
    {
        if(++ResendBillControl[gun].SendCount > 3)
        {
            ResendBillControl[gun].ResendBillState = FALSE;		//发送三次没回复就不发了
            ResendBillControl[gun].SendCount = 0;
            return FALSE;
        }
        ResendBillControl[gun].LastTime = ResendBillControl[gun].CurTime;
#if(NET_YX_SELCT == XY_HY)
        {
            return HY_SendBillData(pdata,250,ifquery);
        }
#endif
#if(NET_YX_SELCT == XY_AP)
        {
            return AP_SendBillData(pdata,200);
        }
#endif
#if(NET_YX_SELCT == XY_YKC)
        {
            return YKC_SendBillData(pdata,200);
        }
#endif
    }


    return TRUE;
}

/*****************************************************************************
* Function     : APP_GetResendBillState
* Description  : 获取是否重发状态
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t APP_GetResendBillState(uint8_t gun)
{
    if(gun >= GUN_MAX)
    {
        return FALSE;
    }
    return ResendBillControl[gun].ResendBillState;
}

/*****************************************************************************
* Function     : ReSendOffLineBill
* Description  :
* Input        : 发送离线交易记录订单
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t  ReSendOffLineBill(void)
{
    static uint8_t count = 0;			//联网状态下，连续三次发送无反应则丢失
    uint8_t data[300];
    static uint8_t num = 0;
    //离线交易记录超时不管A枪和B枪都一样，目前都只用A枪
    ResendBillControl[GUN_A].OFFLineCurTime = OSTimeGet(&timeerr);		//获取当前时间

    //获取是否有离线交易记录
    ResendBillControl[GUN_A].OffLineNum = APP_GetNetOFFLineRecodeNum();		//获取离线交易记录
    if(ResendBillControl[GUN_A].OffLineNum > 0)
    {
        if((ResendBillControl[GUN_A].OFFLineCurTime - ResendBillControl[GUN_A].OFFLineLastTime) >= CM_TIME_30_SEC)
        {
            if(num == ResendBillControl[GUN_A].OffLineNum)
            {
                //第一次不会进来
                if(++count >= 3)
                {
                    //联网状态下连续三次未返回，则不需要发送
                    count = 0;
                    ResendBillControl[GUN_A].OffLineNum--;
                    APP_SetNetOFFLineRecodeNum(ResendBillControl[GUN_A].OffLineNum);
                }
            }
            else
            {
                count = 0;
                num = ResendBillControl[GUN_A].OffLineNum;
            }
            ResendBillControl[GUN_A].OFFLineLastTime = ResendBillControl[GUN_A].OFFLineCurTime;
#if(NET_YX_SELCT == XY_AP)
            {
                APP_RWNetOFFLineRecode(ResendBillControl[GUN_A].OffLineNum,FLASH_ORDER_READ,data);   //读取离线交易记录
                AP_SendOffLineBillData(data,300);
                APP_RWNetFSOFFLineRecode(ResendBillControl[GUN_A].OffLineNum,FLASH_ORDER_READ,data);   //读取离线交易记录
                AP_SendOffLineBillFSData(data,300);
            }
#endif
        }
    }
    return TRUE;
}

/*****************************************************************************
* Function     : APP_SetStartNetState
* Description  : 设置启动方式网络状态
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   APP_SetStartNetState(uint8_t gun ,_START_NET_STATE  type)
{
    if((type >=  NET_STATE_MAX) || (gun >= GUN_MAX))
    {
        return FALSE;
    }

    StartNetState[gun] = type;
    return TRUE;
}

/*****************************************************************************
* Function     : APP_GetGPRSMainEvent
* Description  :获取网络状态
* Input        : 那一路
* Output       : TRUE:表示有网络	FALSE:表示无网络
* Return       :
* Note(s)      :
* Contributor  : 2018-6-14
*****************************************************************************/
uint8_t  APP_GetNetState(uint8_t num)
{
    if(STATE_OK == APP_GetAppRegisterState(num))
    {
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
* Function     : 4G_RecvFrameDispose
* Description  :4G接收
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t _4G_RecvFrameDispose(uint8_t * pdata,uint16_t len)
{
#if(NET_YX_SELCT == XY_HY)
    {
        return  HY_RecvFrameDispose(pdata,len);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        return AP_RecvFrameDispose(pdata,len);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        return YKC_RecvFrameDispose(pdata,len);
    }
#endif
    return TRUE;
}


/*****************************************************************************
* Function     : APP_GetBatchNum
* Description  : 获取交易流水号
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
uint8_t *  APP_GetBatchNum(uint8_t gun)
{
#if(NET_YX_SELCT == XY_HY)
    {
        return APP_GetHYBatchNum(gun);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        return APP_GetAPBatchNum(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        return APP_GetYKCBatchNum(gun);
    }
#endif
    return NULL;
}

/*****************************************************************************
* Function     : APP_GetNetMoney
* Description  :获取账户余额
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
uint32_t APP_GetNetMoney(uint8_t gun)
{
#if(NET_YX_SELCT == XY_HY)
    {
        return APP_GetHYNetMoney(gun);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        return APP_GetAPQGNetMoney(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        return APP_GetYKCNetMoney(gun);
    }
#endif
    return 0;
}
/*****************************************************************************
* Function     : HY_SendFrameDispose
* Description  :4G发送
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t  _4G_SendFrameDispose()
{
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendFrameDispose();
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendFrameDispose();
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendFrameDispose();
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : Pre4GBill
* Description  : 保存订单
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   Pre4GBill(_GUN_NUM gun,uint8_t *pdata)
{
#if(NET_YX_SELCT == XY_HY)
    {
        PreHYBill(gun,pdata);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        PreAPBill(gun,pdata);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        PreYKCBill(gun,pdata);
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : PreHYBill
* Description  : 保存汇誉订单
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   APP_SetResetStartType(void)
{
#if(NET_YX_SELCT == XY_HY)
    {
        return  APP_SetHYResetStartType();
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : Pre4GBill
* Description  : 保存订单
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   _4G_SendDevState(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_HY)
    {
        if(gun == GUN_A)
        {
            HY_SendDevStateA();
        }
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendRateAck
* Description  : 费率应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   _4G_SendRateAck(uint8_t cmd)
{
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendRateAck(cmd);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendRateAck(cmd);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendRateAck(cmd);
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : HY_SendQueryRateAck
* Description  : 查询费率应答
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t    _4G_SendQueryRate(void)
{
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendPriReq();
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendRateMode
* Description  : 发送计费模型
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t    _4G_SendRateMode(void)
{
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendPriModel();
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendSetTimeAck
* Description  : 校时应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   _4G_SendSetTimeAck(void)
{

#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendSetTimeAck();
    }
#endif

#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendSetTimeAck();
    }
#endif
    return TRUE;
}




/***************************************************************
**Function   :_4G_SendSetftpudataAck
**Description:升级应答
**Input      :None
**Output     :
**Return     :
**note(s)    :
**Author     :CSH
**Create_Time:
***************************************************************/
uint8_t   _4G_SendSetftpudataAck(void)
{
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendSetupdataAck();
    }
#endif
    return TRUE;
}



/*****************************************************************************
* Function     : HY_SendBill
* Description  : 发送订单
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendBill(_GUN_NUM gun)
{
    if(gun >= GUN_MAX)
    {
        return FALSE;
    }
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendBill(gun);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendBill(gun);
        AP_SendTimeSharBill(gun);		//发送分时记录
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendBill(gun);
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendCardInfo
* Description  : 发送卡鉴权
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendCardInfo(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendCardInfo(gun);
    }
#endif
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendCardInfo(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendCardInfo(gun);
    }
#endif
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_GetStartType
* Description  : 获取快充启动方式
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
_4G_START_TYPE   _4G_GetStartType(uint8_t gun)
{
    if(gun >= GUN_MAX)
    {
        return _4G_APP_START;
    }
#if(NET_YX_SELCT == XY_AP)
    {
        return (_4G_START_TYPE)APP_GetAPStartType(gun);
    }
#endif
#if(NET_YX_SELCT == XY_HY)
    {
        return (_4G_START_TYPE)APP_GetHYStartType(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        return (_4G_START_TYPE)APP_GetYKCStartType(gun);
    }
#endif


    return _4G_APP_START;
}

/*****************************************************************************
* Function     : _4G_SetStartType
* Description  : 设置安培快充启动方式
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   _4G_SetStartType(uint8_t gun ,_4G_START_TYPE  type)
{
    if((type >=  _4G_APP_MAX) || (gun >= GUN_MAX))
    {
        return FALSE;
    }

#if(NET_YX_SELCT == XY_AP)
    {
        APP_SetAPStartType(gun,type);
    }
#endif
#if(NET_YX_SELCT == XY_HY)
    {
        APP_SetHYStartType(gun,type);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        APP_SetYKCStartType(gun,type);
    }
#endif


    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendVinInfo
* Description  : 发送Vin鉴权
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendVinInfo(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendVinInfo(gun);
    }
#endif
    return TRUE;
}


/*****************************************************************************
* Function     : _4G_SendCardVinCharge
* Description  : 发送卡Vin充电
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendCardVinCharge(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendCardVinStart(gun);
    }
#endif
    return TRUE;
}
/*****************************************************************************
* Function     : _4G_SendStOPtAck
* Description  : 停止应答
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   _4G_SendStopAck(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendStopAck(gun);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendStopAck(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendStopAck(gun);
    }
#endif
    return TRUE;
}



/*****************************************************************************
* Function     : HFQG_SendStartAck
* Description  : 合肥乾古开始充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t   _4G_SendStartAck(_GUN_NUM gun)
{
#if(NET_YX_SELCT == XY_HY)
    {
        HY_SendStartAck(gun);
    }
#endif
#if(NET_YX_SELCT == XY_AP)
    {
        AP_SendStartAck(gun);
    }
#endif
#if(NET_YX_SELCT == XY_YKC)
    {
        YKC_SendStartAck(gun);
    }
#endif
    return TRUE;
}


/*****************************************************************************
* Function     : UART_4GWrite
* Description  :串口写入，因多个任务用到了串口写入，因此需要加互斥锁
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2020-11-26     叶喜雨
*****************************************************************************/
uint8_t UART_4GWrite(uint8_t* const FrameBuf, const uint16_t FrameLen)
{
    OS_ERR ERR;

#if(USE_645 == 0)   //测试时
    printf("Txlen:%d,Txdata:%s\r\n",FrameLen,FrameBuf);
#endif
    OSMutexPend(&sendmutex,0,OS_OPT_PEND_BLOCKING,NULL,&ERR); //获取锁
    UART1SENDBUF(FrameBuf,FrameLen);
    if(FrameLen)
    {
        OSTimeDly((FrameLen/10 + 10)*1, OS_OPT_TIME_PERIODIC, &ERR);	//等待数据发送完成  115200波特率， 1ms大概能发10个字节（大于10个字节）
        //OSTimeDly(SYS_DELAY_5ms);
        OSTimeDly(CM_TIME_50_MSEC, OS_OPT_TIME_PERIODIC, &ERR);
    }
    OSMutexPost(&sendmutex,OS_OPT_POST_NONE,&ERR); //释放锁
    return FrameLen;
}

/*****************************************************************************
* Function     : Connect_4G
* Description  : 4G网络连接
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月11日
*****************************************************************************/
static uint8_t Connect_4G(void)
{
#define RESET_4G_COUNT	7    //目前连续3次未连接上服务器，则重新启动
    static uint8_t i = 8; //第一次上来先复位
    uint8_t count;
    if(APP_GetModuleConnectState(0) !=STATE_OK)   //判断主平台
    {
        if(++i > RESET_4G_COUNT)
        {
            i = 0;
            SIM7600Reset();
        }
    }
    if(APP_GetSIM7600Status() != STATE_OK)  //判断4g模块是否存在
    {
        Module_SIM7600Test();    //4g模块重联
    }
    if(APP_GetSIM7600Status() != STATE_OK)  //模块不存在
    {
        return FALSE;
    }

    //到此说明4g模块已经存在。连接服务器,可能又多个服务器
    for(count = 0; count < NetConfigInfo[NET_YX_SELCT].NetNum; count++)
    {
        if(APP_GetModuleConnectState(count) != STATE_OK) //未连接后台服务器，下面开始连接后台服务器
        {
            if(count == 0)
            {
                if(show_XY_name == show_KL)  //库伦单独用域名登录
                {
                    ModuleSIM7600_ConnectServer(count,(uint8_t*)"pile.coulomb-charging.com",18063);   //域名:pile.coulomb-charging.com 端口:18063
                }
                else
                {
                    ModuleSIM7600_ConnectServer(count,(uint8_t*)NetConfigInfo[NET_YX_SELCT].pIp,NetConfigInfo[NET_YX_SELCT].port);
                }
            }
            else
            {
                ModuleSIM7600_ConnectServer(count,(uint8_t*)GPRS_IP2,GPRS_PORT2);
            }
        }
        if(APP_GetModuleConnectState(count) != STATE_OK)  //未连到服务器
        {
            SIM7600CloseNet(count);			//关闭网络操作
        }
    }
    return TRUE;
}


#define MSG_NUM    5
/*****************************************************************************
* Function     : AppTask4GMain
* Description  : 4G主任务
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月14日
*****************************************************************************/
void AppTask4GMain(void *p_arg)
{
    OS_ERR ERR;
    uint8_t err;
    static uint32_t nowSysTime = 0,lastSysTime = 0;
    static uint8_t i = 1, download_count = 0;


    OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &timeerr);
    if(DisSysConfigInfo.standaloneornet != DISP_NET)
    {
        return;
    }

    OSMutexCreate(&sendmutex,"sendmutex",&ERR);
    if(ERR != OS_ERR_NONE)
    {
        return;
    }

    //打开电源
    _4G_POWER_ON;
    OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);

    _4G_PWKEY_OFF; //开机
    _4G_RET_OFF;   //复位
    OSTimeDly(250, OS_OPT_TIME_PERIODIC, &timeerr);


    //打开电源
    _4G_POWER_ON;
    OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);
    _4G_PWKEY_OFF;
    _4G_RET_OFF;
    OSTimeDly(250, OS_OPT_TIME_PERIODIC, &timeerr);
    //SIM7600Reset(); 
    while(1)
    {
        //什么状态下都可以接收升级更新  等到空闲时，再升级
        if((FTPInfo.FTPupadatflag) && ((stChTcb.ucState == STANDBY) ||(stChTcb.ucState == CHARGER_FAULT)))   //空闲和故障才可以升级
        {
            OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr); //等待，回应一下远程ACK
            APP_SetSIM7600Mode(MODE_FTP); //设置一下ftp模式
            FTPInfo.FTPupadatflag = 0;
        }
#if(UPDATA_STATE)
        if(APP_GetSIM7600Mode() == MODE_DATA)
        {
            Connect_4G();          //4G连接，包括模块是否存在，和连接服务
            //10分钟没连上服务器，而且没在充电中，就重启设备
            nowSysTime = OSTimeGet(&timeerr);
            if((APP_GetAppRegisterState(LINK_NUM) != STATE_OK)  && (stChTcb.ucState != CHARGING) )	//显示已经注册成功了
            {
                if((nowSysTime >= lastSysTime) ? ((nowSysTime - lastSysTime) >= (CM_TIME_5_MIN*2)) : \
                        ((nowSysTime + (0xffffffff - lastSysTime)) >=  (CM_TIME_5_MIN*2)) )
                {
                    SystemReset();			//软件复位
                }
            }
            else
            {
                lastSysTime = nowSysTime;
            }
        }
        else
        {
            if(APP_GetModuleConnectState(0) == STATE_OK)
            {
                //memcpy(HttpInfo.ServerAdd,"http://hy.shuokeren.com/uploads/xiaov1.6.bin",strlen("http://hy.shuokeren.com/uploads/xiaov1.6.bin"));
                if(Module_HTTPDownload(&HttpInfo))
                {
                    //升级成功
                    download_count = 0;
                    //APP_SetUpadaState(1);   //说明升级成功
                    HY_SendUpdataAck(0); //表示升级成功
                    APP_SetSIM7600Mode(MODE_DATA);
                    Send_AT_CIPMODE();
                    OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
                    OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &timeerr);
                    JumpToProgramCode();
                }
                else
                {
                    if(++download_count > 3)
                    {
                        //连续三次升级不成功，则返回升级失败
                        download_count = 0;
                        //APP_SetUpadaState(0);   //说明升级失败
                        //HY_SendUpdataAck(1); //表示升级失败
                        SystemReset();//升级失败，系统复位（自动上传当前的版本号）
                        APP_SetSIM7600Mode(MODE_DATA);
                        Send_AT_CIPMODE();
                        OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
                    }
                }
            }
        }
#else
        if(APP_GetSIM7600Mode() == MODE_DATA)
            //if(APP_GetSIM7600Mode() == 5)
        {
            Connect_4G();          //4G连接，包括模块是否存在，和连接服务
            nowSysTime = OSTimeGet(&timeerr);
            if((APP_GetAppRegisterState(LINK_NUM) != STATE_OK)&&(stChTcb.ucState != CHARGING) )	//显示已经注册成功了
            {
                if((nowSysTime >= lastSysTime) ? ((nowSysTime - lastSysTime) >= (CM_TIME_5_MIN*2)) : \
                        ((nowSysTime + (0xffffffff - lastSysTime)) >=  (CM_TIME_5_MIN*2)) )
                {
                    SystemReset();			//软件复位
                }
            }
            else
            {
                lastSysTime = nowSysTime;
            }
        }
        else
        {
#if(FTPUPDATA_FLAG)

//            //测试用自己
//            memcpy(FTPInfo.FTPServerAdd,"121.42.236.89",strlen("121.42.236.89"));
//            FTPInfo.FTPServerport = 8866;
//            memcpy(FTPInfo.FTPServeruser,"admin",strlen("admin"));
//            memcpy(FTPInfo.FTPServerpass,"admin123",strlen("admin123"));
//            memcpy(FTPInfo.FTPServer_Path,"/cehi/t34.bin",strlen("/cehi/t34.bin")); //路径和名字相同
//            //测试用

            //YKC上传的BIN文件
//            memcpy(FTPInfo.FTPServerAdd,"114.55.114.174",strlen("114.55.114.174"));
//            FTPInfo.FTPServerport = 21;
//            memcpy(FTPInfo.FTPServeruser,"sr",strlen("sr"));
//            memcpy(FTPInfo.FTPServerpass,"sr123",strlen("sr123"));
//            memcpy(FTPInfo.FTPServer_Path,"/1698310171996/FTA26.bin",strlen("/1698310171996/FTA26.bin")); //路径和名字相同
//            //测试用

            if(i)
            {
                SIM7600Reset123();
                i=0;
            }
            if(APP_GetSIM7600Status() != STATE_OK)  //判断4g模块是否存在
            {
                Module_SIM7600Test();    //4g模块重联
            }
            if(APP_GetSIM7600Status() == STATE_OK)  //模块不存在
            {

                if(Module_FTPDownload(&FTPInfo))   //正式开始下载
                {
                    OSTimeDly(CM_TIME_30_SEC, OS_OPT_TIME_PERIODIC, &timeerr);  //开始下载后，等待30s ，下载的过程中，一直在接收状态
                }
                else
                {
                    if(++download_count>5)
                    {
                        download_count = 0;
                        Send_AT_ftploginout();     //退出ftp
                        SystemReset();    //登录不成功或者没有下载，直接默认升级失败   系统复位
                    }
                    SIM7600Reset123();   //4g断电复位
                    Module_SIM7600Test(); //4g模块重联
                }
            }
#endif
        }

#if NET_YX_SELCT == XY_YKC
        APP_YKCSENDFEILV(CH_HALFHOUR_SEC*2);  //YKC在长连接时请求费率（防止在后台移除通道）
        //APP_YKCSENDFEILV(CH_TIME_60_SEC);  //YKC在长连接测试
#endif

#endif
        OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
    }
}



/************************(C)COPYRIGHT 2020 杭州汇誉*****END OF FILE****************************/

