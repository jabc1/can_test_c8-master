/**
******************************************************************************
* @file    queue_jl.h
* @author  Jalon
* @date    2016/08/19
* @brief   循环队列
******************************************************************************
*/
#ifndef __QUEUE_JL_H__
#define __QUEUE_JL_H__

#include  "base.h"

typedef struct QueueTypeStruct
{
    u32 front;
    u32 rear;
    u32 itemSize;
    u32 itemMax;
    u32 itemCount;
    u8* pool;
    bool overwrite;
}QueueType;

bool Queue_Init(QueueType* q, void* buffer, u32 buffer_size, u32 item_size);
bool Queue_Put(QueueType* q, const void* pdata);
bool Queue_Get(QueueType* q, void* pdata);
bool Queue_Query(QueueType* q, void* pdata);
u32 Queue_GetFreeNum(QueueType* q);

#endif
