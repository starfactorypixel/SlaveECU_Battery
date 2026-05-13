#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

void Error_Handler(void);

#define hDebugUart huart1
#define hBms1Uart huart2
#define hBms2Uart huart3

#ifdef __cplusplus
}
#endif

#endif
