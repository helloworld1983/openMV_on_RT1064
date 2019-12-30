/*
 * The Clear BSD License
 * Copyright 2017 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "board.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
 
/* Get debug console frequency. */
uint32_t BOARD_DebugConsoleSrcFreq(void)
{
    uint32_t freq;

    /* To make it simple, we assume default PLL and divider settings, and the only variable
       from application is use PLL3 source or OSC source */
    if (CLOCK_GetMux(kCLOCK_UartMux) == 0) /* PLL3 div6 80M */
    {
        freq = (CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6U) / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }
    else
    {
        freq = CLOCK_GetOscFreq() / (CLOCK_GetDiv(kCLOCK_UartDiv) + 1U);
    }

    return freq;
} 
 
/* Initialize debug console. */
void BOARD_InitDebugConsole(void)
{
    uint32_t uartClkSrcFreq = BOARD_DebugConsoleSrcFreq();

    DbgConsole_Init(BOARD_DEBUG_UART_BASEADDR, BOARD_DEBUG_UART_BAUDRATE, BOARD_DEBUG_UART_TYPE, uartClkSrcFreq);
}
 
/* MPU configuration. */
void BOARD_ConfigMPU(void)
{
	uint32_t rgnNdx = 0;
    /* Disable I cache and D cache */ 
    SCB_DisableICache();
    SCB_DisableDCache();
    /* Disable MPU */ 
    ARM_MPU_Disable(); 
    /* Region 0 setting : ITCM */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x00000000U);	// itcm, max 512kB
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512KB);

//    /* Region 1 setting : ITCM RO zone*/
//	// itcm RO region, catch wild pointers that will corrupt firmware code, region number must be larger to enable nest
//    MPU->RBAR = ARM_MPU_RBAR(1, 0x00000000U);	
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_RO, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_8KB);

    /* Region 2 setting : DTCM */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20000000U);	// dtcm, max 512kB
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512KB);   
    
	/* Region 3 setting : OCRAM, non-cachable part*/
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20200000U);	// ocram
	// better to disable bufferable ---- write back, so CPU always write through, avoid DMA and CPU write same line, error prone
	// 20181011_2159: ARM announced a critical M7 bug that write through memory may wrongly load in a rare condition, so change back to enable bufferable
	MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512KB);
	
	/* Region 4 setting : OCRAM, cachable part*/
	// rocky: Must NOT set to device or strong ordered types, otherwise, unaligned access leads to fault
	// MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20200000U);	// ocram
	// MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_16KB);    

	/* Region 5 setting, flexspi region */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x60000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512MB); 

//	/* Region 6 setting, set whole SDRAM can be accessed by cache */
//    MPU->RBAR = ARM_MPU_RBAR(6, 0x80000000U);
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_32MB);    

//    /* Region 7 setting, set last 4MB of SDRAM can't be accessed by cache, glocal variables which are not expected to be accessed by cache can be put here */
//    MPU->RBAR = ARM_MPU_RBAR(7, 0x81C00000U);
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_4MB);   
 

    /* Enable MPU, enable background region for priviliged access */ 
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
	
    /* Enable I cache and D cache */ 
    SCB_EnableDCache();
    SCB_EnableICache();
}

/* MPU configuration. */
void BOARD_ConfigMPUForCacheErratumTest(void)
{
	uint32_t rgnNdx = 0;
    /* Disable I cache and D cache */ 
    SCB_DisableICache();
    SCB_DisableDCache();
    /* Disable MPU */ 
    ARM_MPU_Disable(); 
    /* Region 0 setting : ITCM */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x00000000U);	// itcm, max 512kB
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512KB);

//    /* Region 1 setting : ITCM RO zone*/
//	// itcm RO region, catch wild pointers that will corrupt firmware code, region number must be larger to enable nest
//    MPU->RBAR = ARM_MPU_RBAR(1, 0x00000000U);	
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_RO, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_8KB);

    /* Region 2 setting : DTCM */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20000000U);	// dtcm, max 512kB
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_512KB);   
    
	/* Region 3 setting : OCRAM, non-cachable part*/
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20200000U);	// ocram
	// better to disable bufferable ---- write back, so CPU always write through, avoid DMA and CPU write same line, error prone
	// 20181011_2159: ARM announced a critical M7 bug that write through memory may wrongly load in a rare condition, so change back to enable bufferable
	MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512KB);
	
	/* Region 4 setting : OCRAM, cachable part*/
	// rocky: Must NOT set to device or strong ordered types, otherwise, unaligned access leads to fault
	// MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x20200000U);	// ocram
	// MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_16KB);    

	/* Region 5 setting, flexspi region */
    MPU->RBAR = ARM_MPU_RBAR(rgnNdx++, 0x60000000U);
    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_512MB); 

//	/* Region 6 setting, set whole SDRAM can be accessed by cache */
//    MPU->RBAR = ARM_MPU_RBAR(6, 0x80000000U);
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 0, 0, 1, 1, 0, ARM_MPU_REGION_SIZE_32MB);    

//    /* Region 7 setting, set last 4MB of SDRAM can't be accessed by cache, glocal variables which are not expected to be accessed by cache can be put here */
//    MPU->RBAR = ARM_MPU_RBAR(7, 0x81C00000U);
//    MPU->RASR = ARM_MPU_RASR(0, ARM_MPU_AP_FULL, 1, 0, 0, 0, 0, ARM_MPU_REGION_SIZE_4MB);   
 

    /* Enable MPU, enable background region for priviliged access */ 
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
	
    /* Enable I cache and D cache */ 
    SCB_EnableDCache();
    SCB_EnableICache();
}
