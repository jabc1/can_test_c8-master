#include <string.h>
#include "nubomed_can_transport_layer.h"

#define PACK_SIGN(_i)       ((_i) << 6)
#define GET_PACK_SIGN(_i)   (((_i) >> 6) & 0x03)

#pragma pack(push, 1) // 以一字节方式对齐，并保存当前
typedef struct{
    u8 cmd;
    u8 data[7];
}CanData_Type;
#pragma pack(pop)     // 恢复之前对齐方式


bool TransportPack(CanTransportPack_Type *pack, const void *inData, u32 len, u8 packFlag)
{
    u32 i;
    const u8 *data = inData;
    CanTxMsgTypeDef txMsg = *hcan.pTxMsg;
    CanData_Type *canData = (CanData_Type *)txMsg.Data;

    txMsg.RTR = CAN_RTR_DATA;
    canData->cmd = 0;

    if(packFlag & PACK_BEGIN)
    {
        pack->checksum = 0;
        SET_BIT(canData->cmd, PACK_SIGN(1));
    }
    else
    {
        CLEAR_BIT(canData->cmd, PACK_SIGN(3));
    }

    for(i=0; i < len; i++)
    {
        pack->checksum += data[i];
    }

    if(packFlag & PACK_END)
    {
        len += 1;   
    }

    if(len > Queue_GetFreeNum(pack->packQueue) * 7)
    {
        return false;
    }

    for(i=0; len > 7; len -= 7, i += 7)
    {
        txMsg.DLC = 8;
        memcpy(canData->data, data+i, 7);
        if(!Queue_Put(pack->packQueue, &txMsg))
        {
            return false;
        }

        CLEAR_BIT(canData->cmd, PACK_SIGN(3));
    }

    txMsg.DLC = len + 1;
    memcpy(canData->data, data+i, len);
    if(packFlag & PACK_END)
    {
        SET_BIT(canData->cmd, PACK_SIGN(2));
        canData->data[len-1] = pack->checksum;
    }

    if(!Queue_Put(pack->packQueue, &txMsg))
    {
        return false;
    }

    return true;
}

s32 TransportUnpack(CanTransportPack_Type *pack, CanRxMsgTypeDef *msg)
{
    CanData_Type *canData = (CanData_Type*)msg->Data;
    u8 sign = GET_PACK_SIGN(canData->cmd);
    u8 len = msg->DLC - 1;
    u8 *buffer = pack->unpackBuff;

    if(sign & 1)
    {
        pack->recvFlag = true;
        pack->offset = 0;
        pack->checksum = 0;
    }

    if(pack->recvFlag)
    {
        if(pack->offset + len > pack->buffSize)
        {
            pack->recvFlag = false;
            return -1;
        }

        memcpy(&buffer[pack->offset], canData->data, len);
        pack->offset += len;

        for(s8 i=0; i < len; i++)
        {
            pack->checksum += canData->data[i];
        }

        if(sign & 2)
        {
            pack->recvFlag = false;

            pack->offset -= 1;
            pack->checksum -= buffer[pack->offset];

            if(buffer[pack->offset] == pack->checksum)
            {
                return 1;
            }
            else
            {
                return -1;
            }
        }
    }

    return 0;
}
