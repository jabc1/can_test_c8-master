/**
******************************************************************************
* @file    bsp_tim.h
* @author  Jalon
* @date    2018/03/12
* @brief   定时器相关驱动
******************************************************************************
*/

#ifndef __TIM_USER_H__
#define __TIM_USER_H__

#include "base.h"

typedef struct {
    uint32_t Hz1Tick;
    uint32_t Hz4Tick;
    uint32_t Hz10Tick;
    uint32_t Hz20Tick;
    uint32_t Hz50Tick;
    uint32_t Hz100Tick;
    uint32_t Hz250Tick;
    uint32_t Hz500Tick;
    uint32_t Hz1000Tick;
    uint8_t Hz1;
    uint8_t Hz4;
    uint8_t Hz10;
    uint8_t Hz20;
    uint8_t Hz50;
    uint8_t Hz100;
    uint8_t Hz250;
    uint8_t Hz500;
    uint8_t Hz1000;
}RunFlag_Type;

typedef struct {
    uint32_t capture;
    uint32_t count;
}IntervalTime_Type;

extern RunFlag_Type RunFlag;

void RunFlagInit(void);
void RunFlagHandler(void);
uint32_t GetIntervalCnt(IntervalTime_Type *last);
uint32_t GetIntervalTimeUs(IntervalTime_Type *last);
void BasicTIM_IRQHandler(void);

#endif
