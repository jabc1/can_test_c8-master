#ifndef __BSP_CAN_IF_H__
#define __BSP_CAN_IF_H__

#include "base.h"
#include "can.h"

HAL_StatusTypeDef BSP_CAN_Transmit(CAN_HandleTypeDef* hc, const CanTxMsgTypeDef* pTxMsg, u32 timeout);
HAL_StatusTypeDef BSP_CAN_Receive_IT(CAN_HandleTypeDef* hc, uint8_t FIFONumber);
void BSP_CAN_IRQHandler(CAN_HandleTypeDef *hcan);
void BSP_CanRecvCallback(CanRxMsgTypeDef *msg);

#endif
