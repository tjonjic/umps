/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef UMPS_MP_CONTROLLER_H
#define UMPS_MP_CONTROLLER_H

#include "umps/types.h"

class MachineConfig;
class Machine;
class SystemBus;
class Processor;

class MPController {
public:
    MPController(const MachineConfig* config, Machine* machine);

    Word Read(Word addr, const Processor* cpu) const;
    void Write(Word addr, Word data, const Processor* cpu);

private:
    static const unsigned int kCpuResetDelay = 50;
    static const unsigned int kPoweroffDelay = 1000;

    const MachineConfig* const config;
    Machine* const machine;

    Word bootPC;
    Word bootSP;
};

#endif // UMPS_MP_CONTROLLER_H
