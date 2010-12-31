/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */

#ifndef UMPS_MP_CONTROLLER_H
#define UMPS_MP_CONTROLLER_H

#include "umps/types.h"

class SystemBus;
class Processor;

class MPController {
public:
    MPController(SystemBus* bus);
    Word Read(Word addr, Processor* cpu);
    void Write(Word addr, Word data, Processor* cpu);

private:
    SystemBus* bus;
    Word bootPC;
    Word bootArg;
    Word* shadow0;
    Word* shadow1;
};

#endif // UMPS_MP_CONTROLLER_H
