//----------------------------------------------------
// Copyright (c) 2014, SHENZHEN PENGRUI SOFT CO LTD. All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------
// Copyright (c) 2014 著作权由深圳鹏瑞软件有限公司所有。著作权人保留一切权利。
//
// 这份授权条款，在使用者符合下列条件的情形下，授予使用者使用及再散播本
// 软件包装原始码及二进位可执行形式的权利，无论此包装是否经改作皆然：
//
// 1. 对于本软件源代码的再散播，必须保留上述的版权宣告、本条件列表，以
//    及下述的免责声明。
// 2. 对于本套件二进位可执行形式的再散播，必须连带以文件以及／或者其他附
//    于散播包装中的媒介方式，重制上述之版权宣告、本条件列表，以及下述
//    的免责声明。

// 免责声明：本软件是本软件版权持有人以及贡献者以现状（"as is"）提供，
// 本软件包装不负任何明示或默示之担保责任，包括但不限于就适售性以及特定目
// 的的适用性为默示性担保。版权持有人及本软件之贡献者，无论任何条件、
// 无论成因或任何责任主义、无论此责任为因合约关系、无过失责任主义或因非违
// 约之侵权（包括过失或其他原因等）而起，对于任何因使用本软件包装所产生的
// 任何直接性、间接性、偶发性、特殊性、惩罚性或任何结果的损害（包括但不限
// 于替代商品或劳务之购用、使用损失、资料损失、利益损失、业务中断等等），
// 不负任何责任，即在该种使用已获事前告知可能会造成此类损害的情形下亦然。
//-----------------------------------------------------------------------------
//所属模块:  CPU初始化
//作者：     lst
//版本：      V1.0.0
//初始版本完成日期：2013-05-27
//文件描述:  cpu时钟初始化
//其他说明:  无
//修订历史:
//2. ...
//1. 日期: 2014-01-20
//   作者: lst
//   新版本号：V1.0.0
//   修改说明: 本版是初版,用于验证目的
//		下一版需增加判断时钟是否已经起振的代码
//------------------------------------------------------
#include "stdint.h"
#include "cpu_peri.h"
#include "hard-exp.h"
#include "arch_feature.h"
#include "stm32f7xx.h"
#include "core_cmFunc.h"
#include "cpu-optional.h"
#include "core_cm7.h"
#ifndef __CHECK_DEVICE_DEFINES
#define __CHECK_DEVICE_DEFINES
#endif
//#define VECT_TAB_SRAM
#define VECT_TAB_OFFSET  0x0

extern   uint32_t   msp_top[ ];
extern void __set_PSP(uint32_t topOfProcStack);
extern void __set_PRIMASK(uint32_t priMask);
extern void __set_FAULTMASK(uint32_t faultMask);
extern void __set_CONTROL(uint32_t control);

extern bool_t SysClockInit(void);
extern void Load_Preload(void);

struct ScbReg volatile * const startup_scb_reg
                        = (struct ScbReg *)0xe000ed00;

void Startup_NMI(void)
{
    while(1);
}
void Startup_Hardfault(void)
{
    while(1);
}
void Init_Cpu(void);
//初始化异常向量表，此时所有中断以及可屏蔽异常都是关闭的，故只需要提供4个向量：
// 栈顶、复位、NMI、hard fault。而且均给空函数即可。
//系统运行时使用的异常表，在异常模块处加载，在ram中。
const u32 gc_u32StartupExpTable[4] __attribute__ ((section(".StartupExpTbl")))=
{
    (u32)msp_top,
    (u32)Init_Cpu,
    (u32) Startup_NMI,
    (u32) Startup_Hardfault
};
void Init_Cpu(void)
{
	__set_PSP((uint32_t)msp_top);
	__set_PRIMASK(1);
	__set_FAULTMASK(1);
	__set_CONTROL(0);

    #if (CN_CPU_OPTIONAL_FPU == 1)
        startup_scb_reg->CPACR = (3UL << 20)|(3UL << 22);    //使能FPU
        startup_scb_reg->FPCCR = (1UL << 31);                //关闭lazy stacking
    #endif
    switch(startup_scb_reg->CPUID)
    {
    }
    //系统时钟初始化Fsys=PLL=261MHZ   Fusb==48Mhz
    while(false==SysClockInit());

#ifdef USE_HAL_DRIVER
    HAL_TickInit();
#endif

    extern void SRAM_Init(void);
    SRAM_Init();		//片外RAM 初始化

#if (CN_CPU_OPTIONAL_CACHE == 1)
	SCB_EnableICache();			//指令、数据cache使能
	SCB_EnableDCache();
#endif
    IAP_SelectLoadProgam();
}

extern void Load_Preload(void);
//-----------------------------------------------------------------
//功能：APP应用程序的入口函数，iboot程序中不调用，app的lds文件中，须将该函数
//     的链接地址放在IbootSize + 512的起始位置。该函数配置栈指针并加载程序
//参数：无
//返回：无。
//-----------------------------------------------------------------
void AppStart(void)
{
	__set_MSP((uint32_t)msp_top);
	__set_PSP((uint32_t)msp_top);
	Load_Preload();
}

//-----------------------------------------------------------------
//功能：IAP组件控制运行模式所需的GPIO引脚初始化，由于此时系统还没有加载，只能使
//      用直接地址操作，不能调用gpio相关的库函数。
//      如果不是使用gpio做标志，本函数不是必须，可删掉。
//参数：无
//返回：无。
//-----------------------------------------------------------------
void IAP_GpioPinInit(void)
{
}

//-----------------------------------------------------------------
//功能：由硬件决定是否强制进入Iboot，若此函数返回TRUE，则强制运行Iboot。通常会使
//      用一个gpio，通过跳线决定。
//      正常情况下，如果正在运行APP，是可以用runiboot命令切换到Iboot状态的，设置
//      此硬件的目的有二：
//     1、在严重异常错误，不能用shell切换时，提供一个补救措施。
//     2、出于安全考虑，APP中没有包含切换代码，或者由于资源的关系，裁掉了shell。
//参数：无
//返回：无。
//说明：本函数所涉及到的硬件，须在本文件中初始化，特别需要注意的是，不允许调用未
//      加载的函数，特别是库函数。
//      本函数必须提供，如果没有设置相应硬件，可以简单返回false。
//-----------------------------------------------------------------
bool_t IAP_IsForceIboot(void)
{
//    u32 flag;
//    IAP_GpioPinInit( );
//    flag=pg_gpio_regc->IDR&(1<<10);
//    if(flag==0)
//        return false;
//    return true;

    return false;

}




