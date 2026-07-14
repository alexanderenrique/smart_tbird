#pragma once

#include "telemetry_data.h"

// Must be called while holding the LVGL port lock.
void ui_apply_telemetry(const TelemetryData &data);
