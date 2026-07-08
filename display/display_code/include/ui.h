#pragma once

#include <stdint.h>
#include <stdbool.h>

// Set to 0 when RPM comes from Modbus register 5.
#define UI_SIMULATE_RPM 1

struct UiDashboardData {
    bool valid;
    float oil_temp_f;
    float coolant_temp_f;
    float trans_temp_f;
    float o2_afr;
    float voltage_v;
};

void ui_init(void);
void ui_set_dashboard_placeholders(void);
void ui_update_dashboard(const UiDashboardData *data);
void ui_update_brightness_pct(int pct);
void ui_update_rpm(uint16_t rpm);

#if UI_SIMULATE_RPM
uint16_t ui_simulated_rpm(void);
#endif
