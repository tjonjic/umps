/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#include "umps/mp_controller.h"

#include <boost/bind.hpp>

#include "base/lang.h"
#include "umps/machine_config.h"
#include "umps/machine.h"
#include "umps/processor.h"
#include "umps/systembus.h"
#include "umps/arch.h"

MPController::MPController(const MachineConfig* config, Machine* machine)
    : config(config),
      machine(machine),
      bootPC(MPCONF_DEFAULT_BOOT_PC),
      bootSP(MPCONF_DEFAULT_BOOT_SP),
      bootArg(0)
{}

Word MPController::Read(Word addr, const Processor* cpu) const
{
    UNUSED_ARG(cpu);

    switch (addr) {
    case MPCONF_NCPUS:
        return config->getNumProcessors();

    case MPCONF_BOOT_PC:
        return bootPC;

    case MPCONF_BOOT_SP:
        return bootSP;

    case MPCONF_BOOT_ARG:
        return bootArg;

    default:
        return 0;
    }
}

void MPController::Write(Word addr, Word data, const Processor* cpu)
{
    UNUSED_ARG(cpu);

    Word cpuId;

    switch (addr) {
    case MPCONF_RESET:
        cpuId = data & MPCONF_RESET_CPUID_MASK;
        if (cpuId < config->getNumProcessors())
            machine->getBus()->ScheduleEvent(kCpuResetDelay * config->getClockRate(),
                                             boost::bind(&Processor::Reset, machine->getProcessor(cpuId),
                                                         bootPC, bootSP));
        break;

    case MPCONF_BOOT_PC:
        bootPC = data;
        break;

    case MPCONF_BOOT_SP:
        bootSP = data;
        break;

    case MPCONF_BOOT_ARG:
        bootArg = data;
        break;

    default:
        break;
    }
}
