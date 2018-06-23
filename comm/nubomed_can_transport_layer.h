#ifndef  __NUBOMED_CAN_TRANSPORT_LAYER_H__
#define  __NUBOMED_CAN_TRANSPORT_LAYER_H__

#include "base.h"
#include "can.h"
#include "queue_jl.h"

#define PACK_BEGIN     0x01
#define PACK_END       0x02

typedef struct
{
    bool recvFlag;      ///< 接收标志
    u16 offset;         ///< 当前buf位置
    u8 checksum;        ///< 校验和
    u8 *unpackBuff;
    u16 buffSize;
    QueueType *packQueue;
}CanTransportPack_Type;

bool TransportPack(CanTransportPack_Type *pack, const void *data, u32 len, u8 packFlag);
s32 TransportUnpack(CanTransportPack_Type *pack, CanRxMsgTypeDef *msg);

#endif
