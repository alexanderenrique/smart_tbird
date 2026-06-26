// ESP32 ATtiny UPDI Programmer firmware
// User guide: ../README.md

#include <Arduino.h>

#include "command_parser.h"
#include "hw_control.h"
#include "pins.h"
#include "stream_receiver.h"
#include "uart_bridge.h"
#include "updi/updi_link.h"
#include "updi/updi_nvm.h"
#include "updi/updi_phy.h"
#include "updi/updi_probe.h"

UpdiPhy gUpdiPhy;
UpdiLink gUpdiLink(gUpdiPhy);
UpdiNvm gUpdiNvm(gUpdiLink);
StreamReceiver gStreamReceiver(gUpdiNvm);
HwControl gHwControl;
UartBridge gUartBridge;
CommandParser gCommandParser(
    gUpdiLink,
    gUpdiPhy,
    gUpdiNvm,
    gStreamReceiver,
    gHwControl,
    gUartBridge
);

void setup() {
    Serial.begin(USB_SERIAL_BAUD);
    while (!Serial && millis() < 2000) {
        delay(10);
    }

    gHwControl.begin();
    gUartBridge.begin();

    Serial.println("READY ESP32-UPDI v1.0");
    Serial.println("INFO bridge disabled (phase 1)");

    UpdiProbe probe(gUpdiLink, gUpdiPhy);
    UpdiProbeResult probeResult;
    probe.run(probeResult);
    probe.printResult(Serial, probeResult);

    if (probeResult.success) {
        Serial.println("INFO waiting for .hex file (use attiny-uploader)");
    } else {
        Serial.println("INFO UPDI not ready - check wiring and target power");
        Serial.println("INFO send UPDI_PROBE to retry, or use attiny-uploader updi-probe");
    }

    gCommandParser.setup();
}

void loop() {
    gCommandParser.loop();
}
