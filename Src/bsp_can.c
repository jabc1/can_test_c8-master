/**
******************************************************************************
* @file    bsp_can.c
* @author  Jalon
* @date    2016/09/14
* @brief   CAN 过滤器配置及解包打包缓冲处理
******************************************************************************
*/
#include <string.h>
#include "bsp_can.h"
#include "queue_jl.h"

#define UNPACK_POOL_SIZE    10
#define SHORT_UNPACK_SIZE   16
#define LONG_UNPACK_SIZE    128

#define FILTER_STDID(_id)   (((_id) & 0x7FF) << (21 - 16))
#define SET_PACK_STDID(_id, _d)  MODIFY_REG(_id, 0xFF, _d)

static u8 CanUnpackBuffer[UNPACK_POOL_SIZE][SHORT_UNPACK_SIZE];
static u8 CanLongUnpackBuffer[LONG_UNPACK_SIZE];
static CanNode_Type CanUnpackPool[UNPACK_POOL_SIZE];
static CanNode_Type CanLongUnpack = {
    .isUsed = false, 
    .pack.recvFlag = false,
    .pack.unpackBuff = CanLongUnpackBuffer,
    .pack.buffSize = LONG_UNPACK_SIZE,
};
static u32 CanNodeFindFailedCount = 0;
static u32 CanUnpackFailedCount = 0;

static CanTxMsgTypeDef TxMsg;
static CanRxMsgTypeDef Rx0Msg;
static CanRxMsgTypeDef Rx1Msg;
static CanRxMsgTypeDef RxMsgPool[128];
static CanTxMsgTypeDef TxMsgPool[128];
QueueType RxQueue;
QueueType TxQueue;

CanTransportPack_Type TxPack = {.packQueue = &TxQueue};

u32 AddrInit(void)
{
#ifdef MASTER_NODE
    u32 addr = 1;
#else
    u32 addr = 0;

    addr |= !HAL_GPIO_ReadPin(ADDR3_GPIO_Port, ADDR3_Pin);
    addr |= ((u32)!HAL_GPIO_ReadPin(ADDR2_GPIO_Port, ADDR2_Pin)) << 1;
    addr |= ((u32)!HAL_GPIO_ReadPin(ADDR1_GPIO_Port, ADDR1_Pin)) << 2;
    addr |= ((u32)!HAL_GPIO_ReadPin(ADDR0_GPIO_Port, ADDR0_Pin)) << 3;
#endif
    return addr;
}

void BSP_CanInit(void)
{
    CAN_FilterConfTypeDef  sFilterConfig;

    __HAL_CAN_DBG_FREEZE(&hcan, DISABLE);

    TxMsg.StdId = AddrInit();
    TxMsg.IDE = CAN_ID_STD;

#ifdef MASTER_NODE
    sFilterConfig.FilterNumber = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = FILTER_STDID(1);
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = 0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.BankNumber = 14;

    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        /* Filter configuration Error */
        Error_Handler();
    }

    sFilterConfig.FilterNumber = 1;
    sFilterConfig.FilterIdHigh = FILTER_STDID(1);
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = FILTER_STDID(1);
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = 1;

    SET_PACK_PRIO(TxMsg.StdId, 0);
#else
    sFilterConfig.FilterNumber = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = FILTER_STDID(0x000);
    sFilterConfig.FilterIdLow = 0x0000;
    // sFilterConfig.FilterMaskIdHigh = FILTER_STDID(0xFF);
    sFilterConfig.FilterMaskIdHigh = FILTER_STDID(0x700);
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = 0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.BankNumber = 14;

    SET_PACK_PRIO(TxMsg.StdId, 1);
#endif
    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        /* Filter configuration Error */
        Error_Handler();
    }

    hcan.pTxMsg = &TxMsg;
    hcan.pRxMsg = &Rx0Msg;
    hcan.pRx1Msg = &Rx1Msg;

    BSP_CAN_Receive_IT(&hcan, CAN_FIFO0);
    BSP_CAN_Receive_IT(&hcan, CAN_FIFO1);
    
    Queue_Init(&RxQueue, RxMsgPool, sizeof(RxMsgPool), sizeof(RxMsgPool[0]));
    Queue_Init(&TxQueue, TxMsgPool, sizeof(TxMsgPool), sizeof(TxMsgPool[0]));

    for(s32 i=0; i < UNPACK_POOL_SIZE; i++)
    {
        CanUnpackPool[i].isUsed = false;
        CanUnpackPool[i].pack.recvFlag = false;
        CanUnpackPool[i].pack.unpackBuff = CanUnpackBuffer[i];
        CanUnpackPool[i].pack.buffSize = SHORT_UNPACK_SIZE;
    }
}

static CanNode_Type *CanNodeFind(u32 canId)
{
    static u8 number = 0;
    u8 maxDiff = 0;
    CanNode_Type *oldest;
    CanNode_Type *pFree = NULL;
    CanNode_Type *node = CanUnpackPool;

    if(GET_PACK_PRIO(canId) == 6)
    {
        if(canId != CanLongUnpack.canId || CanLongUnpack.isUsed == false)
        {
            CanLongUnpack.isUsed = true;
            CanLongUnpack.canId = canId;
            CanLongUnpack.pack.recvFlag = false;
        }
        return &CanLongUnpack;
    }

    for(; node < &CanUnpackPool[UNPACK_POOL_SIZE]; node++)
    {
        if(node->isUsed)
        {
            if(node->canId == canId)
            {
                return node;
            }

            u8 tmp = number - node->number;
            if(tmp > maxDiff)
            {
                oldest = node;
                maxDiff = tmp;
            }
        }
        else if(pFree == NULL)
        {
            pFree = node;
        }
    }

    if(pFree == NULL)
    {
        pFree = oldest;
        CanNodeFindFailedCount++;
    }

    pFree->isUsed = true;
    pFree->canId = canId;
    pFree->pack.recvFlag = false;
    pFree->number = number++;

    return pFree;
}

void BSP_CanNodeFree(CanNode_Type *node)
{
    node->isUsed = false;
}

CanNode_Type *BSP_GetCanNode(CanRxMsgTypeDef *msg)
{
    CanNode_Type *node = CanNodeFind(msg->StdId);

    s32 ret = TransportUnpack(&node->pack, msg);

    if(ret == 1)
    {
        return node;
    }
    else if(ret != 0)
    {
        BSP_CanNodeFree(node);
        CanUnpackFailedCount++;
    }

    return NULL;
}

void BSP_CanRecvCallback(CanRxMsgTypeDef *msg)
{
    Queue_Put(&RxQueue, msg);
}

#ifndef MASTER_NODE
// #define NODE_TEST
#endif

bool BSP_CanSendData(const void *data, u32 len, u8 packFlag)
{
#ifdef NODE_TEST
    if(GET_PACK_PRIO(hcan.pTxMsg->StdId) == 1)
    {
        SET_PACK_PRIO(hcan.pTxMsg->StdId, 2);
    }
    else
    {
        SET_PACK_PRIO(hcan.pTxMsg->StdId, 1);
    }
    return TransportPack(&TxPack, data, len, packFlag);
#else
    return TransportPack(&TxPack, data, len, packFlag);
#endif
}

void BSP_CanTxTask(void)
{
#ifdef NODE_TEST
    CanTxMsgTypeDef txMsg;
    static CanTxMsgTypeDef txMsg2 = {.DLC = 0};
    
    if(Queue_Query(TxPack.packQueue, &txMsg))
    {
        if(GET_PACK_PRIO(txMsg.StdId) == 1)
        {
            if(((txMsg.Data[0] >> 6) & 0x3) == 2)
            {
                txMsg2 = txMsg;
                Queue_Get(TxPack.packQueue, NULL);
            }
            else
            {
                if(txMsg2.DLC)
                {
                    BSP_CAN_Transmit(&hcan, &txMsg2, 0);
                    txMsg2.DLC = 0;
                }

                if(BSP_CAN_Transmit(&hcan, &txMsg, 0) == HAL_OK)
                {
                    Queue_Get(TxPack.packQueue, NULL);
                }
            }
        }
        else if(GET_PACK_PRIO(txMsg.StdId) == 2)
        {
            if(((txMsg.Data[0] >> 6) & 0x3) == 2)
            {
                txMsg2 = txMsg;
                Queue_Get(TxPack.packQueue, NULL);
            }
            else
            {
                if(BSP_CAN_Transmit(&hcan, &txMsg, 0) == HAL_OK)
                {
                    Queue_Get(TxPack.packQueue, NULL);

                    if(txMsg2.DLC)
                    {
                        BSP_CAN_Transmit(&hcan, &txMsg2, 0);
                        txMsg2.DLC = 0;
                    }
                }
            }
        }
    }
#else
    CanTxMsgTypeDef txMsg;
    
    if(Queue_Query(TxPack.packQueue, &txMsg))
    {
        if(BSP_CAN_Transmit(&hcan, &txMsg, 0) == HAL_OK)
        {
            Queue_Get(TxPack.packQueue, NULL);
        }
    }
#endif
}
