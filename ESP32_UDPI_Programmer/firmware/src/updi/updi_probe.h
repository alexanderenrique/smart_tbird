#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>

#include "updi_link.h"
#include "updi_phy.h"

struct UpdiProbeResult {
    bool success = false;
    bool breakSent = false;
    bool datalinkOk = false;
    bool sibRead = false;
    uint8_t statusA = 0;
    char sib[128] = {0};
    size_t sibLength = 0;
};

class UpdiProbe {
public:
    UpdiProbe(UpdiLink &link, UpdiPhy &phy);

    // Run BREAK -> init datalink -> read SIB. Re-initializes the UART each time.
    bool run(UpdiProbeResult &result);
    void printResult(Stream &out, const UpdiProbeResult &result) const;

private:
    UpdiLink &_link;
    UpdiPhy &_phy;

    void debugStep(Stream &out, const char *step, bool ok) const;
    void printSibFields(Stream &out, const char *sib, size_t length) const;
};
