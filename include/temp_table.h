#pragma once
#include <Arduino.h>

// Lookup table entry
struct TempEntry {
    uint16_t adc;
    float    tempC;
};

// Full table (use sorted by adc)
static const TempEntry coolantTable[] = {
    {  86, 302 }, { 109, 284 }, { 138, 266 }, { 178, 248 },
    { 232, 230 }, { 271, 212 }, { 327, 194 }, { 415, 176 },
    { 575, 158 }, { 916, 140 }, {1387, 122 }, {1601, 113 },
    {1930, 104 }, {2278,  95 }, {2492,  86 }, {2877,  77 },
    {3213,  68 }, {3463,  59 }, {3678,  50 }, {3474,  23 },
    {3605,  14 }, {3714,   5 }
};
static const int COOLANT_TABLE_SIZE =
    sizeof(coolantTable) / sizeof(coolantTable[0]);

// Linear interpolation function
static float Calc_Temp_fromADC(uint16_t adc)
{
    // Clamp endpoints
    if (adc <= coolantTable[0].adc) return coolantTable[0].tempC;
    if (adc >= coolantTable[COOLANT_TABLE_SIZE-1].adc)
        return coolantTable[COOLANT_TABLE_SIZE-1].tempC;

    // Search for interval (simple loop)
    for (int i = 0; i < COOLANT_TABLE_SIZE - 1; i++) {
        uint16_t a0 = coolantTable[i].adc;
        uint16_t a1 = coolantTable[i+1].adc;

        if (adc >= a0 && adc <= a1) {
            float t0 = coolantTable[i].tempC;
            float t1 = coolantTable[i+1].tempC;
            float frac = float(adc - a0) / float(a1 - a0);
            return t0 + frac * (t1 - t0);
        }
    }

    return NAN; // Should never happen
}
