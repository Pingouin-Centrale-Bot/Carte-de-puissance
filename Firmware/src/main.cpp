#include <Arduino.h>
#include "config.hpp"

enum Msg
{
  CRITICAL_LOW_BATT_MSG,
  CRITICAL_HIGH_BATT_MSG,
  PI_NOT_OFF_MSG,
  NORMAL_OFF_MSG,
  POWERING_UP_MSG
};

enum BattState
{
  START_BATT_STATE,
  HIGH_BATT_STATE,
  NORMAL_BATT_STATE,
  MEDIUM_BATT_STATE,
  LOW_BATT_STATE,
  VERY_LOW_BATT_STATE,
  POWERING_OFF_ENTER,
  POWERING_OFF_WAITING_FOR_PI
};
volatile BattState batt_state = START_BATT_STATE;

enum BAUState
{
  IDLE_BAU,
  PRESSED_TIMEOUT_BEFORE_POWERCUT,
  POWERCUT
};

volatile BAUState BAU_state = IDLE_BAU;
volatile bool BAU_state_changed;

void handle_bau()
{
  if (BAU_state == IDLE_BAU && !(batt_state >= POWERING_OFF_ENTER))
  {
    BAU_state = PRESSED_TIMEOUT_BEFORE_POWERCUT;
    digitalWriteFast(BAU_OUT, LOW);
    BAU_state_changed = true;
    tone(BUZZER, LOW_NOTE, 2000);
  }
}

void handle_power_btn()
{
  if (millis() >= 1500 && batt_state < POWERING_OFF_ENTER) // To prevent issues if pressed too soon with debouncing
  {
    batt_state = POWERING_OFF_ENTER;
  }
}

uint16_t readVoltageMV()
{
  return analogRead(BATT_SENSE) * (2.5 / 4095) * ((47 + 4.7) / 4.7) * BATT_CORRECT * 1000;
}

inline void setRGB(Color c)
{
  digitalWriteFast(LED_R, ((c >> 2) & 0x01) == 0);
  digitalWriteFast(LED_G, ((c >> 1) & 0x01) == 0);
  digitalWriteFast(LED_B, ((c >> 0) & 0x01) == 0);
}

inline void playCode(Color c, unsigned int tone_f, bool end_delay = true)
{
  tone(BUZZER, tone_f);
  setRGB(c);
  delay(100);
  if (!end_delay)
  {
    delay(100);
  }
  noTone(BUZZER);
  setRGB(Black);
  if (end_delay)
  {
    delay(100);
  }
}

void playMsg(Msg error)
{
  switch (error)
  {
  case CRITICAL_HIGH_BATT_MSG:
    playCode(HIGH_COLOR, HIGH_NOTE);
    playCode(HIGH_COLOR, HIGH_NOTE);
    playCode(HIGH_COLOR, HIGH_NOTE);
    Serial1.println("Critical high batt");
    break;
  case CRITICAL_LOW_BATT_MSG:
    playCode(LOW_COLOR, LOW_NOTE);
    playCode(LOW_COLOR, LOW_NOTE);
    playCode(LOW_COLOR, LOW_NOTE);
    Serial1.println("Critical low batt");
    break;
  case PI_NOT_OFF_MSG:
    playCode(HIGH_COLOR, HIGH_NOTE);
    playCode(MEDIUM_COLOR, MEDIUM_NOTE);
    playCode(HIGH_COLOR, HIGH_NOTE);
    playCode(MEDIUM_COLOR, MEDIUM_NOTE);
    // Followed by normal power off.
  case NORMAL_OFF_MSG:
    playCode(HIGH_COLOR, HIGH_NOTE, false);
    playCode(MEDIUM_COLOR, MEDIUM_NOTE, false);
    playCode(LOW_COLOR, LOW_NOTE, false);
    break;
  case POWERING_UP_MSG:
    playCode(LOW_COLOR, LOW_NOTE, false);
    playCode(MEDIUM_COLOR, MEDIUM_NOTE, false);
    playCode(HIGH_COLOR, HIGH_NOTE, false);
    break;
  }
}

void unplugBattery()
{
  digitalWriteFast(EN_BATT, LOW);
  delay(2000);
  tone(BUZZER, 2489); // Should be off by now. The tone indicates user should disconnect power ASAP.
  while (true)
    ;
}

bool updateBattState() // Will unplug if battery critical
{
  uint16_t batt_voltage = readVoltageMV();
  Serial1.printf("Voltage: %d;\n", batt_voltage);
  if (batt_voltage >= CRITICAL_HIGH_BATT)
  {
    playMsg(CRITICAL_HIGH_BATT_MSG);
    unplugBattery(); // Never returns
  }
  else if (batt_voltage <= CRITICAL_LOW_BATT || (batt_state == START_BATT_STATE && batt_voltage <= VERY_LOW_BATT))
  {
    playMsg(CRITICAL_LOW_BATT_MSG);
    unplugBattery(); // Never returns
  }

  if (batt_voltage <= VERY_LOW_BATT && batt_state < POWERING_OFF_ENTER)
  {
    batt_state = POWERING_OFF_ENTER;
    return true;
  }
  else if (batt_voltage <= LOW_BATT && batt_state < VERY_LOW_BATT_STATE)
  {
    batt_state = VERY_LOW_BATT_STATE;
    return true;
  }
  else if (batt_voltage <= MEDIUM_BATT && batt_state < LOW_BATT_STATE)
  {
    batt_state = LOW_BATT_STATE;
    return true;
  }
  else if (batt_voltage <= NORMAL_BATT && batt_state < MEDIUM_BATT_STATE)
  {
    batt_state = MEDIUM_BATT_STATE;
    return true;
  }
  else if (batt_voltage <= HIGH_BATT && batt_state < NORMAL_BATT_STATE)
  {
    batt_state = NORMAL_BATT_STATE;
    return true;
  }
  else if (batt_state < HIGH_BATT_STATE)
  {
    batt_state = HIGH_BATT_STATE;
    return true;
  }
  return false;
}

void setup()
{
  Serial1.begin(115200);

  // Power control
  pinModeFast(EN_BATT, OUTPUT);
  digitalWriteFast(EN_BATT, HIGH); // Latch power
  pinModeFast(EN_EXTPWR, OUTPUT);
  digitalWriteFast(EN_EXTPWR, LOW);
  pinModeFast(EN_PI, OUTPUT);
  digitalWriteFast(EN_PI, LOW);

  // Batt sense
  pinModeFast(BATT_SENSE, INPUT);
  analogReference(INTERNAL2V5);
  analogReadResolution(12);

  // RGB LED
  pinModeFast(LED_R, OUTPUT);
  pinModeFast(LED_G, OUTPUT);
  pinModeFast(LED_B, OUTPUT);
  digitalWriteFast(LED_R, HIGH);
  digitalWriteFast(LED_G, HIGH);
  digitalWriteFast(LED_B, LOW);

  // Buzzer
  pinModeFast(BUZZER, OUTPUT);
  digitalWriteFast(BUZZER, LOW);

  // Check batt voltage
  updateBattState(); // Will prevent further power on if the batt is low.

  // BAU (Emergency Button)
  pinModeFast(BAU_IN, INPUT);
  pinModeFast(BAU_OUT, OUTPUT);
  digitalWriteFast(BAU_OUT, BAU_state);
  digitalWriteFast(EN_EXTPWR, BAU_state);
  attachInterrupt(BAU_IN, handle_bau, FALLING);
  if (digitalReadFast(BAU_IN))
  {
    BAU_state = IDLE_BAU;
  }
  else
  {
    BAU_state = POWERCUT;
  }

  // Pi Communications
  pinModeFast(SHUTDOWN_PI, OUTPUT);
  digitalWriteFast(SHUTDOWN_PI, LOW);
  pinModeFast(POWEROFF_PI, INPUT_PULLUP);

  // Power btn
  pinModeFast(PWR_BTN, INPUT_PULLUP);
  attachInterrupt(PWR_BTN, handle_power_btn, FALLING);

  // TODO : PAC1811

  playMsg(POWERING_UP_MSG);
}

void loop()
{
  unsigned long current = millis();
  static unsigned long last_batt_state_update = current;
  static unsigned long last_pac1811_measure = current;
  static unsigned long last_led_change = current;
  static unsigned long pi_poweroff_time;
  static unsigned long BAU_pressed_time;
  static bool led_state = true;

  static bool state_changed = true;

  // LED and main power control
  switch (batt_state)
  {
  case HIGH_BATT_STATE:
    if (state_changed)
    {
      setRGB(Green);
    }
    break;
  case NORMAL_BATT_STATE:
    if (state_changed)
    {
      setRGB(Yellow);
    }
    break;
  case MEDIUM_BATT_STATE:
    if (state_changed)
    {
      setRGB(Red);
    }
    break;
  case LOW_BATT_STATE:
    if (current - last_led_change > 1000)
    {
      setRGB(led_state ? Red : Black);
      if (led_state)
      {
        tone(BUZZER, DEFAULT_NOTE, 150);
      }
      led_state = !led_state;
      last_led_change = current;
    }
    break;
  case VERY_LOW_BATT_STATE:
    if (current - last_led_change > 250)
    {
      setRGB(led_state ? Red : Black);
      if (led_state)
      {
        tone(BUZZER, DEFAULT_NOTE, 200);
      }
      led_state = !led_state;
      last_led_change = current;
    }
    break;
  case POWERING_OFF_ENTER:
    digitalWriteFast(SHUTDOWN_PI, HIGH); // Ask pi to poweroff
    BAU_state = PRESSED_TIMEOUT_BEFORE_POWERCUT;
    BAU_state_changed = true;
    batt_state = POWERING_OFF_WAITING_FOR_PI;
    pi_poweroff_time = current;
    tone(BUZZER, MEDIUM_NOTE, 200);
    // no break on purpose, to continue for next state when using NINJA
  case POWERING_OFF_WAITING_FOR_PI:
    if (digitalReadFast(POWEROFF_PI) or NINJA)
    {
      playMsg(NORMAL_OFF_MSG);
      unplugBattery(); // Does not return
    }
    else if (current - pi_poweroff_time > 60 * 1000) // Timeout, if the pi does not poweroff...
    {
      playMsg(PI_NOT_OFF_MSG);
      unplugBattery(); // Does not return
    }
    if (current - last_led_change > 500)
    {
      setRGB(led_state ? Black : Blue);
      led_state = !led_state;
      last_led_change = current;
    }
    break;
  default:
    break;
  }

  switch (BAU_state)
  {
  case IDLE_BAU:
    if (BAU_state_changed && !(batt_state >= POWERING_OFF_ENTER))
    {
      tone(BUZZER, HIGH_NOTE, 500);
      digitalWriteFast(BAU_OUT, HIGH);
      digitalWriteFast(EN_EXTPWR, HIGH);
      Serial1.print("BAU: Depressed;\n");
      BAU_state_changed = false;
    }
    break;

  case PRESSED_TIMEOUT_BEFORE_POWERCUT:
    if (BAU_state_changed)
    {
      BAU_pressed_time = current;
      BAU_state_changed = false;
    }
    if (current - BAU_pressed_time > BAU_delay_before_powercut)
    {
      digitalWriteFast(EN_EXTPWR, LOW);
      BAU_state = POWERCUT;
      BAU_state_changed = true;
    }
    break;

  case POWERCUT:
  {
    static bool first_time = true;
    if (digitalReadFast(BAU_IN))
    {
      // Debouncing
      if (first_time)
      {
        first_time = false;
        BAU_pressed_time = current;
      }
      else if (current - BAU_pressed_time > 100)
      {
        BAU_state = IDLE_BAU;
        BAU_state_changed = true;
        break;
      }
    }
    else
    {
      first_time = true;
    }
    BAU_state_changed = false;
    break;
  }

  default:
    break;
  }

  // PAC1811
  if (current - last_pac1811_measure > pac1811_measure_period)
  {
    if (BAU_state == IDLE_BAU)
    {
      Serial1.println("BAU: Depressed;");
    }
    else
    {
      Serial1.println("BAU: Pressed;");
    }
    last_pac1811_measure = current;
  }

  if (current - last_batt_state_update > batt_state_update_period)
  {
    state_changed = updateBattState();
    last_batt_state_update = current;
  }
  else
  {
    state_changed = false;
  }
}
