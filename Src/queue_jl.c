/**
******************************************************************************
* @file    queue_jl.c
* @author  Jalon
* @date    2016/08/19
* @brief   循环队列
******************************************************************************
*/
#include "queue_jl.h"
#include <string.h>

bool Queue_Init(QueueType* q, void* pool, u32 buffer_size, u32 item_size)
{
    if(pool == NULL || item_size == 0 || buffer_size/item_size < 2)
    {
        return false;
    }
    
    q->front = 0;
    q->rear = 0;
    q->itemSize = item_size;
    q->itemMax = buffer_size/item_size;
    q->pool = (u8*)pool;
    q->itemCount = 0;
    q->overwrite = false;
    
    return true;
}

bool Queue_Put(QueueType* q, const void* pdata)
{
    if(q->itemCount == q->itemMax)
    {
        if(q->overwrite)
        {
            q->front = (q->front + 1) % q->itemMax;
        }
        else
        {
            return false;
        }
    }
    
    memcpy(q->pool + (q->rear * q->itemSize), pdata, q->itemSize);
    
    q->rear = (q->rear + 1) % q->itemMax;
    q->itemCount++;
    return true;
}

bool Queue_Get(QueueType* q, void* pdata)
{
    if(q->itemCount == 0)
    {
        return false;
    }

    if(pdata)
    {
        memcpy(pdata, q->pool + (q->front * q->itemSize), q->itemSize);
    }
    
    q->front = (q->front + 1) % q->itemMax;
    q->itemCount--;
    return true;
}

bool Queue_Query(QueueType* q, void* pdata)
{
    if(q->itemCount == 0 || pdata == NULL)
    {
        return false;
    }
    
    memcpy(pdata, q->pool + (q->front * q->itemSize), q->itemSize);
    
    return true;
}

u32 Queue_GetFreeNum(QueueType* q)
{
    return q->itemMax - q->itemCount;
}
