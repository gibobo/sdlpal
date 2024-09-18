#ifndef OPLCORE_H
#define OPLCORE_H

#include <stdint.h>

class OPLCORE {
public:
    OPLCORE(uint32_t rate) : rate(rate) {}
    virtual ~OPLCORE() {}

    virtual void Reset() = 0;
    virtual void Write(uint32_t reg, uint8_t val) = 0;
    virtual void Generate(short* buf, int samples) = 0;
    virtual OPLCORE* Duplicate() = 0;

protected:
    uint32_t rate;
};

#endif