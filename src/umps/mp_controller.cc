/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "umps/mp_controller.h"

#include <umps/const.h>
#include "umps/types.h"

#include "umps/processor.h"
#include "umps/systembus.h"

#include "umps/arch.h"

MPController::MPController(SystemBus* bus)
    : bus(bus),
      bootPC(BSP_BOOT_PC),
      bootArg(0UL)
{}

Word MPController::Read(Word addr, Processor* cpu)
{
    switch (addr) {
    case MPC_NCPUS:
        return 0;

    case MPC_CPU_BOOT_PC:
        return bootPC;

    case MPC_CPU_BOOT_ARG:
        return 0;

    case MPC_CPUID:
        return 0;

    case MPC_SHADOW_0:
        return 0;

    case MPC_SHADOW_1:
        return 0;

    default:
        break;
    }
}

void MPController::Write(Word addr, Word data, Processor* cpu)
{
}

