#include <Arduino.h>

static const pin_size_t TX = PIN_PA1;
static const pin_size_t SHUTDOWN_PI = PIN_PA2; // To shutdown the Pi
static const pin_size_t POWEROFF_PI = PIN_PA3; // To tell if the Pi is shutdown
static const pin_size_t BUZZER = PIN_PA4;
static const pin_size_t EN_BATT = PIN_PA5;
static const pin_size_t BAU_IN = PIN_PA6;
static const pin_size_t LED_R = PIN_PA7;

// static const pin_size_t SCL = PIN_PB0; already exists
// static const pin_size_t SDA = PIN_PB1; already exists
static const pin_size_t EN_PI = PIN_PB2;
static const pin_size_t EN_EXTPWR = PIN_PB3;
static const pin_size_t PWR_BTN = PIN_PB4;
static const pin_size_t BATT_SENSE = PIN_PB5;

static const pin_size_t BAU_OUT = PIN_PC1;
static const pin_size_t LED_G = PIN_PC2;
static const pin_size_t LED_B = PIN_PC3;

static const float BATT_CORRECT = 24.18 / 24.00;
#ifdef ROBOT // 6s LiPo
static const int CELL_COUNT = 6;
static const uint16_t CRITICAL_HIGH_BATT = 1000 * 4.22 * CELL_COUNT; // Refuse to power on or power off if higher.
static const uint16_t HIGH_BATT = 1000 * 3.9 * CELL_COUNT;           // about 80% charge (green)
static const uint16_t NORMAL_BATT = 1000 * 3.75 * CELL_COUNT;        // about 40% charge (orange)
static const uint16_t MEDIUM_BATT = 1000 * 3.6 * CELL_COUNT;         // about 20% charge (red)
static const uint16_t LOW_BATT = 1000 * 3.5 * CELL_COUNT;            // about 10% charge (slow flashing red)
static const uint16_t VERY_LOW_BATT = 1000 * 3.4 * CELL_COUNT;       // about 5% charge (fast flashing red, force soft power off if lower)
static const uint16_t CRITICAL_LOW_BATT = 1000 * 3.1 * CELL_COUNT;   // If lower, refuse to power on or hard power off.
#endif
#ifdef NINJA // 4s LiFePo4
static const int CELL_COUNT = 4;
static const uint16_t CRITICAL_HIGH_BATT = 1000 * 3.65 * CELL_COUNT; // Refuse to power on or power off if higher.
static const uint16_t HIGH_BATT = 1000 * 3.32 * CELL_COUNT;          // about 80% charge (green)
static const uint16_t NORMAL_BATT = 1000 * 3.25 * CELL_COUNT;        // about 40% charge (orange)
static const uint16_t MEDIUM_BATT = 1000 * 3.2 * CELL_COUNT;         // about 20% charge (red)
static const uint16_t LOW_BATT = 1000 * 3.0 * CELL_COUNT;            // about 10% charge (slow flashing red)
static const uint16_t VERY_LOW_BATT = 1000 * 2.8 * CELL_COUNT;       // about 5% charge (fast flashing red, force soft power off if lower)
static const uint16_t CRITICAL_LOW_BATT = 1000 * 2.55 * CELL_COUNT;  // If lower, refuse to power on or hard power off.
#endif
#ifndef NINJA
#define NINJA false
#endif

enum Color : uint8_t
{
  Black = 0b000, // All off
  Red = 0b100,
  Green = 0b010,
  Blue = 0b001,
  Yellow = 0b110,  // Red + Green
  Magenta = 0b101, // Red + Blue
  Cyan = 0b011,    // Green + Blue
  White = 0b111    // All on
};

static const unsigned int LOW_NOTE = 1568;
static const unsigned int MEDIUM_NOTE = 1976;
static const unsigned int HIGH_NOTE = 2349;
static const unsigned int DEFAULT_NOTE = 2349;

static const Color LOW_COLOR = Cyan;
static const Color MEDIUM_COLOR = Magenta;
static const Color HIGH_COLOR = Yellow;

static const unsigned long batt_state_update_period = 200;  // ms
static const unsigned long pac1811_measure_period = 250;    // ms
static const unsigned long BAU_delay_before_powercut = 250; // ms