/**
******************************************************************************
* @file    bsp_can_if.c
* @author  Jalon
* @date    2016/09/14
* @brief   对 HAL 库 CAN 相关函数的封装
******************************************************************************
*/
#include "bsp_can_if.h"

u32 Rx0Count = 0, Rx1Count = 0;
u32 TxCount = 0;

/**
* @brief  CAN 发送函数
* @param  hc      : 句柄
* @param  pTxMsg  : 帧内容
* @param  timeout : 超时 ms
*/
HAL_StatusTypeDef BSP_CAN_Transmit(CAN_HandleTypeDef* hc, const CanTxMsgTypeDef* pTxMsg, u32 timeout)
{
    u32 tick;
    uint32_t transmitmailbox = CAN_TXSTATUS_NOMAILBOX;

    /* Check the parameters */
    assert_param(IS_CAN_IDTYPE(pTxMsg->IDE));
    assert_param(IS_CAN_RTR(pTxMsg->RTR));
    assert_param(IS_CAN_DLC(pTxMsg->DLC));
    
    tick = HAL_GetTick();
    
    do
    {
        if (((hc->Instance->TSR & CAN_TSR_TME0) == CAN_TSR_TME0) ||
            ((hc->Instance->TSR & CAN_TSR_TME1) == CAN_TSR_TME1) ||
            ((hc->Instance->TSR & CAN_TSR_TME2) == CAN_TSR_TME2))
        {
            /* Select one empty transmit mailbox */
            if (HAL_IS_BIT_SET(hc->Instance->TSR, CAN_TSR_TME0))
            {
                transmitmailbox = CAN_TXMAILBOX_0;
            }
            else if (HAL_IS_BIT_SET(hc->Instance->TSR, CAN_TSR_TME1))
            {
                transmitmailbox = CAN_TXMAILBOX_1;
            }
            else
            {
                transmitmailbox = CAN_TXMAILBOX_2;
            }

            /* Set up the Id */
            hc->Instance->sTxMailBox[transmitmailbox].TIR &= CAN_TI0R_TXRQ;
            if (pTxMsg->IDE == CAN_ID_STD)
            {
                assert_param(IS_CAN_STDID(pTxMsg->StdId));
                hc->Instance->sTxMailBox[transmitmailbox].TIR |= ((pTxMsg->StdId << CAN_TI0R_STID_Pos) |
                                                                    pTxMsg->RTR);
            }
            else
            {
                assert_param(IS_CAN_EXTID(pTxMsg->ExtId));
                hc->Instance->sTxMailBox[transmitmailbox].TIR |= ((pTxMsg->ExtId << CAN_TI0R_EXID_Pos) |
                                                                    pTxMsg->IDE |
                                                                    pTxMsg->RTR);
            }

            /* Set up the DLC */
            hc->Instance->sTxMailBox[transmitmailbox].TDTR &= 0xFFFFFFF0U;
            hc->Instance->sTxMailBox[transmitmailbox].TDTR |= (pTxMsg->DLC & (uint8_t)0x0000000F);

            /* Set up the data field */
            WRITE_REG(hc->Instance->sTxMailBox[transmitmailbox].TDLR, ((uint32_t)pTxMsg->Data[3] << CAN_TDL0R_DATA3_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[2] << CAN_TDL0R_DATA2_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[1] << CAN_TDL0R_DATA1_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[0] << CAN_TDL0R_DATA0_Pos));
            WRITE_REG(hc->Instance->sTxMailBox[transmitmailbox].TDHR, ((uint32_t)pTxMsg->Data[7] << CAN_TDL0R_DATA3_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[6] << CAN_TDL0R_DATA2_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[5] << CAN_TDL0R_DATA1_Pos) |
                                                                            ((uint32_t)pTxMsg->Data[4] << CAN_TDL0R_DATA0_Pos));
            /* Request transmission */
            SET_BIT(hc->Instance->sTxMailBox[transmitmailbox].TIR, CAN_TI0R_TXRQ);

            /* Return function status */
            TxCount++;
            return HAL_OK;
        }
        
        if(timeout == 0)
        {
            break;
        }
        
        HAL_Delay(1);
    }while((HAL_GetTick() - tick) < timeout);

    
    /* Change CAN state */
    hc->State = HAL_CAN_STATE_ERROR;

    /* Return function status */
    return HAL_ERROR;
}

/**
* @brief  开启 CAN 相应队列的接收中断
* @param  hc         : 句柄
* @param  FIFONumber : 队列编号
*/
HAL_StatusTypeDef BSP_CAN_Receive_IT(CAN_HandleTypeDef* hc, uint8_t FIFONumber)
{
    /* Check the parameters */
    assert_param(IS_CAN_FIFO(FIFONumber));

    /* Set CAN error code to none */
    hc->ErrorCode = HAL_CAN_ERROR_NONE;

    /* Enable interrupts: */
    /*  - Enable Error warning Interrupt */
    /*  - Enable Error passive Interrupt */
    /*  - Enable Bus-off Interrupt */
    /*  - Enable Last error code Interrupt */
    /*  - Enable Error Interrupt */
    /*  - Enable Transmit mailbox empty Interrupt */
    __HAL_CAN_ENABLE_IT(hc, CAN_IT_EWG |
                                  CAN_IT_EPV |
                                  CAN_IT_BOF |
                                  CAN_IT_LEC |
                                  CAN_IT_ERR);

    if (FIFONumber == CAN_FIFO0)
    {
        /* Enable FIFO 0 overrun and message pending Interrupt */
        __HAL_CAN_ENABLE_IT(hc, CAN_IT_FOV0 | CAN_IT_FMP0);
    }
    else
    {
        /* Enable FIFO 1 overrun and message pending Interrupt */
        __HAL_CAN_ENABLE_IT(hc, CAN_IT_FOV1 | CAN_IT_FMP1);
    }

    /* Return function status */
    return HAL_OK;
}


static HAL_StatusTypeDef BSP_CAN_ReadData(CAN_HandleTypeDef *hc, uint8_t FIFONumber, CanRxMsgTypeDef *pRxMsg)
{
    CAN_FIFOMailBox_TypeDef *mailBox = &hc->Instance->sFIFOMailBox[FIFONumber];

    /* Get the Id */
    pRxMsg->IDE = (uint8_t)0x04U & mailBox->RIR;
    if (pRxMsg->IDE == CAN_ID_STD)
    {
        pRxMsg->StdId = 0x000007FFU & (mailBox->RIR >> 21U);
    }
    else
    {
        pRxMsg->ExtId = 0x1FFFFFFFU & (mailBox->RIR >> 3U);
    }

    pRxMsg->RTR = (uint8_t)0x02U & mailBox->RIR;
    /* Get the DLC */
    pRxMsg->DLC = (uint8_t)0x0FU & mailBox->RDTR;
    /* Get the FIFONumber */
    pRxMsg->FIFONumber = FIFONumber;
    /* Get the FMI */
    pRxMsg->FMI = (uint8_t)0xFFU & (mailBox->RDTR >> 8U);
    /* Get the data field */
    pRxMsg->Data[0] = (uint8_t)0xFFU & mailBox->RDLR;
    pRxMsg->Data[1] = (uint8_t)0xFFU & (mailBox->RDLR >> 8U);
    pRxMsg->Data[2] = (uint8_t)0xFFU & (mailBox->RDLR >> 16U);
    pRxMsg->Data[3] = (uint8_t)0xFFU & (mailBox->RDLR >> 24U);
    pRxMsg->Data[4] = (uint8_t)0xFFU & mailBox->RDHR;
    pRxMsg->Data[5] = (uint8_t)0xFFU & (mailBox->RDHR >> 8U);
    pRxMsg->Data[6] = (uint8_t)0xFFU & (mailBox->RDHR >> 16U);
    pRxMsg->Data[7] = (uint8_t)0xFFU & (mailBox->RDHR >> 24U);
    /* Release the FIFO */
    
    __HAL_CAN_FIFO_RELEASE(hc, FIFONumber);
    
    BSP_CanRecvCallback(pRxMsg);

    /* Return function status */
    return HAL_OK;
}

/**
* @brief  CAN 接收中断处理函数
* @param  hc         : 句柄
* @param  FIFONumber : 队列编号
*/
void BSP_CAN_IRQHandler(CAN_HandleTypeDef *hc)
{
    uint32_t tmp1 = 0U, tmp2 = 0U, tmp3 = 0U;
    uint32_t errorcode = HAL_CAN_ERROR_NONE;

    /* Check Overrun flag for FIFO0 */
    tmp1 = __HAL_CAN_GET_FLAG(hc, CAN_FLAG_FOV0);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_FOV0);
    if ((tmp1 != 0U) && tmp2)
    {
        /* Set CAN error code to FOV0 error */
        errorcode |= HAL_CAN_ERROR_FOV0;

        /* Clear FIFO0 Overrun Flag */
        __HAL_CAN_CLEAR_FLAG(hc, CAN_FLAG_FOV0);
    }

    /* Check Overrun flag for FIFO1 */
    tmp1 = __HAL_CAN_GET_FLAG(hc, CAN_FLAG_FOV1);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_FOV1);
    if ((tmp1 != 0U) && tmp2)
    {
        /* Set CAN error code to FOV1 error */
        errorcode |= HAL_CAN_ERROR_FOV1;

        /* Clear FIFO1 Overrun Flag */
        __HAL_CAN_CLEAR_FLAG(hc, CAN_FLAG_FOV1);
    }

    tmp1 = __HAL_CAN_MSG_PENDING(hc, CAN_FIFO0);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_FMP0);
    /* Check End of reception flag for FIFO0 */
    if ((tmp1 != 0U) && tmp2)
    {
        /* Call receive function */
        BSP_CAN_ReadData(hc, CAN_FIFO0, hc->pRxMsg);
        Rx0Count++;
    }

    tmp1 = __HAL_CAN_MSG_PENDING(hc, CAN_FIFO1);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_FMP1);
    /* Check End of reception flag for FIFO1 */
    if ((tmp1 != 0U) && tmp2)
    {
        /* Call receive function */
        BSP_CAN_ReadData(hc, CAN_FIFO1, hc->pRx1Msg);
        Rx1Count++;
    }

    /* Set error code in handle */
    hc->ErrorCode |= errorcode;

    tmp1 = __HAL_CAN_GET_FLAG(hc, CAN_FLAG_EWG);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_EWG);
    tmp3 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_ERR);
    /* Check Error Warning Flag */
    if (tmp1 && tmp2 && tmp3)
    {
        /* Set CAN error code to EWG error */
        hc->ErrorCode |= HAL_CAN_ERROR_EWG;
        /* No need for clear of Error Warning Flag as read-only */
    }

    tmp1 = __HAL_CAN_GET_FLAG(hc, CAN_FLAG_EPV);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_EPV);
    tmp3 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_ERR);
    /* Check Error Passive Flag */
    if (tmp1 && tmp2 && tmp3)
    {
        /* Set CAN error code to EPV error */
        hc->ErrorCode |= HAL_CAN_ERROR_EPV;
        /* No need for clear of Error Passive Flag as read-only */
    }

    tmp1 = __HAL_CAN_GET_FLAG(hc, CAN_FLAG_BOF);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_BOF);
    tmp3 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_ERR);
    /* Check Bus-Off Flag */
    if (tmp1 && tmp2 && tmp3)
    {
        /* Set CAN error code to BOF error */
        hc->ErrorCode |= HAL_CAN_ERROR_BOF;
        /* No need for clear of Bus-Off Flag as read-only */
    }

    tmp1 = HAL_IS_BIT_CLR(hc->Instance->ESR, CAN_ESR_LEC);
    tmp2 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_LEC);
    tmp3 = __HAL_CAN_GET_IT_SOURCE(hc, CAN_IT_ERR);
    /* Check Last error code Flag */
    if ((!tmp1) && tmp2 && tmp3)
    {
        tmp1 = (hc->Instance->ESR & CAN_ESR_LEC);
        switch (tmp1)
        {
        case (CAN_ESR_LEC_0):
            /* Set CAN error code to STF error */
            hc->ErrorCode |= HAL_CAN_ERROR_STF;
            break;
        case (CAN_ESR_LEC_1):
            /* Set CAN error code to FOR error */
            hc->ErrorCode |= HAL_CAN_ERROR_FOR;
            break;
        case (CAN_ESR_LEC_1 | CAN_ESR_LEC_0):
            /* Set CAN error code to ACK error */
            hc->ErrorCode |= HAL_CAN_ERROR_ACK;
            break;
        case (CAN_ESR_LEC_2):
            /* Set CAN error code to BR error */
            hc->ErrorCode |= HAL_CAN_ERROR_BR;
            break;
        case (CAN_ESR_LEC_2 | CAN_ESR_LEC_0):
            /* Set CAN error code to BD error */
            hc->ErrorCode |= HAL_CAN_ERROR_BD;
            break;
        case (CAN_ESR_LEC_2 | CAN_ESR_LEC_1):
            /* Set CAN error code to CRC error */
            hc->ErrorCode |= HAL_CAN_ERROR_CRC;
            break;
        default:
            break;
        }

        /* Clear Last error code Flag */
        CLEAR_BIT(hc->Instance->ESR, CAN_ESR_LEC);
    }
}



