/**
******************************************************************************
* @file    bsp_tim.c
* @author  Jalon
* @date    2018/03/12
* @brief   定时器相关驱动
******************************************************************************
*/

#include "bsp_tim.h"

#define TICK_FREQ       1000
#define TICK_CNT_FREQ   SYS_CLK
#define TICK_PERIOD     (TICK_CNT_FREQ/TICK_FREQ)

static uint8_t PriorityFlag;
static uint32_t RunTick;
RunFlag_Type RunFlag;

#define FREQ_FLAG_INIT(freq) do{ \
    RunFlag.Hz##freq = 0; \
    RunFlag.Hz##freq##Tick = tick; \
}while(0)

#define FREQ_FLAG(freq) FreqCalc(TICK_FREQ/freq, &(RunFlag.Hz##freq), &(RunFlag.Hz##freq##Tick))

void FreqCalc(uint32_t FreqFactor, uint8_t* pFlag, uint32_t* pTick)
{
    if(!PriorityFlag && RunTick - *pTick >= FreqFactor)
    {
        *pFlag = 1;
        (*pTick) += FreqFactor;
        PriorityFlag = 1;
    }
    else
    {
        *pFlag = 0;
    }
}

uint8_t CPU_AVG_Usage = 0;
uint32_t IT_BusyTime = 0;

void RunFlagInit(void)
{
    uint32_t tick = HAL_GetTick();
    
    FREQ_FLAG_INIT(1);
    FREQ_FLAG_INIT(4);
    FREQ_FLAG_INIT(10);
    FREQ_FLAG_INIT(20);
    FREQ_FLAG_INIT(50);
    FREQ_FLAG_INIT(100);
    FREQ_FLAG_INIT(250);
    FREQ_FLAG_INIT(500);
    FREQ_FLAG_INIT(1000);
}

void RunFlagHandler(void)
{
#ifdef _DEBUG
    static IntervalTime_Type last = {0,0};
    static uint32_t busyTime = 0;
    
    uint32_t time = GetIntervalCnt(&last);
    
    if(PriorityFlag || time > 2000)
    {
        busyTime += time;
    }
    
    if(RunFlag.Hz1)
    {
        CPU_AVG_Usage = ((busyTime + IT_BusyTime) / (1000 * 100)) / (TICK_CNT_FREQ / 1000);
        IT_BusyTime = 0;
        busyTime = 0;
    }
#endif
    
    PriorityFlag = 0;
    RunTick = HAL_GetTick();
    
    FREQ_FLAG(1000);
    FREQ_FLAG(500);
    FREQ_FLAG(250);
    FREQ_FLAG(100);
    FREQ_FLAG(50);
    FREQ_FLAG(20);
    FREQ_FLAG(10);
    FREQ_FLAG(4);
    FREQ_FLAG(1);
}

uint32_t GetIntervalCnt(IntervalTime_Type *last)
{
    uint32_t pluseCnt;
    uint32_t capture1, capture2, timCnt1, timCnt2;
    
    capture1 = HAL_GetTick();
    timCnt1 = SysTick->VAL;
    capture2 = HAL_GetTick();
    timCnt2 = SysTick->VAL;

    if(capture1 != capture2)
    {
        capture1 = capture2;
        timCnt1 = timCnt2;
    }

    // SysTick->VAL 向下计数
    pluseCnt = (uint32_t)(capture1 - last->capture) * TICK_PERIOD + (uint32_t)(last->count - timCnt1);
    
    last->count = timCnt1;
    last->capture= capture1;
    
    return pluseCnt;
}

uint32_t GetIntervalTimeUs(IntervalTime_Type *last)
{
    return GetIntervalCnt(last) / (TICK_CNT_FREQ / 1000000);
}
