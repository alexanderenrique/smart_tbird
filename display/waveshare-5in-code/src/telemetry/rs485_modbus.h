#pragma once

#include "telemetry_data.h"

void initRs485();
void pollRs485();
bool rs485TakeFreshTelemetry(TelemetryData *out);
