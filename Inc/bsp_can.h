#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include "base.h"
#include "can.h"
#include "nubomed_can_transport_layer.h"
#include "bsp_can_if.h"

#define MASTER_NODE
#define TEST_RECV_SIZE 5

typedef struct
{
    bool isUsed;
    u32 canId;
    u8 number;
    CanTransportPack_Type pack;
}CanNode_Type;

#define GET_PACK_PRIO(_id)  ((_id) >> 8)
#define SET_PACK_PRIO(_id, _prio)  MODIFY_REG(_id, 7 << 8, (_prio) << 8)

void BSP_CanInit(void);
CanNode_Type *BSP_GetCanNode(CanRxMsgTypeDef *msg);
void BSP_CanNodeFree(CanNode_Type *pack);
bool BSP_CanSendData(const void *data, u32 len, u8 packFlag);
void BSP_CanTxTask(void);

extern QueueType RxQueue;

#endif
