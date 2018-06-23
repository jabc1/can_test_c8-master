#include "app_main.h"
#include "can.h"
#include "bsp_tim.h"
#include "bsp_can.h"
#include "can_comm.h"

extern CanTxMsgTypeDef TxMsg;
u32 Rx0 = 0, Rx1 = 0, Tx = 0;
// TestData_Type TestData = {0,0};
// TestData_Type TestRecvData[TEST_RECV_SIZE];

void AppMainLoop(void)
{
    BSP_CanInit();
    RunFlagInit();

    while(1)
    {
        RunFlagHandler();
#ifdef MASTER_NODE
        // if(RunFlag.Hz1)
        if(1)
#else
        if(RunFlag.Hz1000)
#endif
        {
            // if(BSP_CanSend((u8*)&TestData, 8))
            // {
            //     TestData.id += 1;
            // }
        }

        CanCommTask();
    }
}
