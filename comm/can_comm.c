#include "can_comm.h"
#include "bsp_can.h"
#include "bsp_tim.h"

#pragma pack(push, 1) // 以一字节方式对齐，并保存当前
typedef struct{
    u16 len;
    u8 id;
    u32 number;
    u8 data[1];
}CmdHead;

typedef struct{
    u16 len;
    u8 id;
    u32 number;
    u32 errorCount;
    u32 tmp;
}CmdTest;
#pragma pack(pop)     // 恢复之前对齐方式

u32 RecvCmdCount = 0;
CmdTest Test = {sizeof(CmdTest), 0xfe, 0, 0};
CmdTest TestRecv[5];
u32 LastNum[5];

static bool CmdOne(void)
{
    BSP_CanSendData(&Test, Test.len, PACK_BEGIN | PACK_END);
    Test.number++;
    return false;
}

static bool CmdHandle(u32 id, u8 *data, u32 len)
{
    CmdHead *head = (CmdHead *)data;
    u8 num = id & 0xff;

    if(head->len != len)
    {
        Test.errorCount++;
        return false;
    }

    if(head->number != 0 && head->number != (LastNum[num] + 1))
    {
        Test.errorCount++;
    }

    LastNum[num] = head->number;

    if(head->number == 0)
    {
#ifndef MASTER_NODE
        Test.number = 0;
#endif
        Test.errorCount = 0;
    }

    if(head->id == Test.id)
    {
        TestRecv[num] = *(CmdTest *)head;
        // if(GET_PACK_PRIO(id) == 1)
        // {
        //     TestRecv = *(CmdTest *)head;
        // }
        // else
        // {
        //     TestRecv2 = *(CmdTest *)head;
        // }
    }

    RecvCmdCount++;
    return true;
}

void CanCommTask(void)
{
    CanRxMsgTypeDef rxMsg;
    
    // Rx
    while(Queue_Get(&RxQueue, &rxMsg))
    {
        CanNode_Type *node = BSP_GetCanNode(&rxMsg);
        if(node)
        {
            CmdHandle(node->canId, node->pack.unpackBuff, node->pack.offset);
            BSP_CanNodeFree(node);
        }
    }

    if(RunFlag.Hz500)
    {
        CmdOne();
    }

    // Tx
    BSP_CanTxTask();
}
