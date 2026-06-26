#include "updi_probe.h"

#include "pins.h"
#include "updi_debug.h"
#include <string.h>

UpdiProbe::UpdiProbe(UpdiLink &link, UpdiPhy &phy) : _link(link), _phy(phy) {}

void UpdiProbe::debugStep(Stream &out, const char *step, bool ok) const {
    out.print("DEBUG UPDI ");
    out.print(step);
    out.print(" ... ");
    out.println(ok ? "OK" : "FAIL");
}

void UpdiProbe::printSibFields(Stream &out, const char *sib, size_t length) const {
    if (length == 0) {
        return;
    }

    out.print("INFO SIB raw: ");
    for (size_t index = 0; index < length; ++index) {
        char character = sib[index];
        if (character == '\r' || character == '\n') {
            break;
        }
        if (character >= 0x20 && character <= 0x7E) {
            out.print(character);
        } else {
            out.print('.');
        }
    }
    out.println();

    if (length < 19) {
        out.println("INFO SIB parse: incomplete (need >= 19 chars)");
        return;
    }

    char family[8] = {0};
    char nvm[4] = {0};
    char ocd[4] = {0};
    char osc[5] = {0};

    memcpy(family, sib, min(static_cast<size_t>(7), length));
    if (length > 8) {
        memcpy(nvm, sib + 8, min(static_cast<size_t>(3), length - 8));
    }
    if (length > 11) {
        memcpy(ocd, sib + 11, min(static_cast<size_t>(3), length - 11));
    }
    if (length > 15) {
        memcpy(osc, sib + 15, min(static_cast<size_t>(4), length - 15));
    }

    out.print("INFO SIB family: ");
    out.println(family);
    out.print("INFO SIB NVM: ");
    out.println(nvm);
    out.print("INFO SIB OCD: ");
    out.println(ocd);
    out.print("INFO SIB OSC: ");
    out.println(osc);
}

bool UpdiProbe::run(UpdiProbeResult &result) {
    result = UpdiProbeResult{};

    Serial.println("DEBUG UPDI === probe start ===");
    Serial.print("DEBUG UPDI config GPIO RX=");
    Serial.print(PIN_UPDI_RX);
    Serial.print(" TX=");
    Serial.print(PIN_UPDI_TX);
    Serial.print(" baud=");
    Serial.println(UPDI_BAUD);
    updiDebugPinState(PIN_UPDI_RX);
    updiDebugPinState(PIN_UPDI_TX);

    if (!_phy.begin(PIN_UPDI_RX, PIN_UPDI_TX, UPDI_BAUD)) {
        updiDebugMsg("probe fail: UART begin");
        return false;
    }

    result.breakSent = _phy.sendDoubleBreak();
    debugStep(Serial, "step 1/3 send double BREAK", result.breakSent);
    if (!result.breakSent) {
        return false;
    }

    updiDebugPinState(PIN_UPDI_RX);
    updiDebugPinState(PIN_UPDI_TX);

    result.datalinkOk = _link.initDatalink();
    debugStep(Serial, "step 2/3 initialize datalink", result.datalinkOk);
    if (!result.datalinkOk) {
        return false;
    }

    if (!_link.checkDatalink(result.statusA)) {
        debugStep(Serial, "step 2/3 check STATUSA", false);

        result.breakSent = _phy.sendDoubleBreak();
        debugStep(Serial, "retry send double BREAK", result.breakSent);
        if (!result.breakSent) {
            return false;
        }

        result.datalinkOk = _link.initDatalink();
        debugStep(Serial, "retry initialize datalink", result.datalinkOk);
        if (!result.datalinkOk || !_link.checkDatalink(result.statusA)) {
            debugStep(Serial, "retry check STATUSA", false);
            updiDebugPinState(PIN_UPDI_RX);
            updiDebugPinState(PIN_UPDI_TX);
            Serial.println("DEBUG UPDI === probe failed (no datalink) ===");
            return false;
        }
    }

    debugStep(Serial, "step 2/3 check STATUSA", true);

    uint8_t sibBuffer[sizeof(result.sib)] = {0};
    result.sibRead = _link.readSib(sibBuffer, sizeof(sibBuffer) - 1, result.sibLength);
    debugStep(Serial, "step 3/3 read SIB", result.sibRead);

    if (result.sibRead) {
        memcpy(result.sib, sibBuffer, result.sibLength);
        result.sib[result.sibLength] = '\0';
    }

    result.success = result.breakSent && result.datalinkOk && result.sibRead;
    Serial.println(result.success ? "DEBUG UPDI === probe passed ===" : "DEBUG UPDI === probe failed ===");
    return result.success;
}

void UpdiProbe::printResult(Stream &out, const UpdiProbeResult &result) const {
    if (result.success) {
        printSibFields(out, result.sib, result.sibLength);
        return;
    }

    if (!result.breakSent) {
        out.println("INFO failed at: send BREAK");
    } else if (!result.datalinkOk) {
        out.println("INFO failed at: initialize UPDI datalink");
    } else if (!result.sibRead) {
        out.print("INFO failed at: read SIB (STATUSA=0x");
        out.print(result.statusA, HEX);
        out.println(")");
    }
}
