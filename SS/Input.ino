// Solar System
// (c) Andrews & Arnold Ltd, Adrian Kennard, see LICENSE file (GPL)

// Input buttons

#include <ESPRevK.h>
const char* Input_fault = NULL;
const char* Input_tamper = NULL;

#define MAX_PIN 17
#define MAX_INPUT 9

byte inputpin[MAX_INPUT] = {};
unsigned int inputinvert = 0;
unsigned int inputactive = 0;
unsigned int inputstate = 0;

#define app_settings  \
  s(input,0);   \
  s(inputhold,100); \
  s(inputpoll,10); \

#define s(n,d) unsigned int n=d;
  app_settings
#undef s

  boolean Input_active(int i)
  { // If input is in use
    i--; // Starts from 1
    if (!(inputactive & (1 << i)))return false;
    return true;
  }

  boolean Input_get(int i)
  { // Read an input (saved state)
    i--; // Starts from 1
    if (inputstate & (1 << i))return true;
    return false;
  }

  boolean Input_direct(int i)
  { // Read input directly (or saved value if not an input pin)
    i--; // Starts from 1
    if (!(inputactive & (1 << i)))return ((inputstate & (1 << i)) ? true : false); // Logical input
    boolean r = (digitalRead(inputpin[i]) ? true : false); // Actual input
    if (inputinvert & (1 << i))
      r = !r;
    return r;
  }

  void Input_set(int o, boolean v)
  { // Set an input externally
    o--; // Starts from 1
    if (v)inputstate |= (1 << o);
    else inputstate &= ~(1 << o);
  }

  const char* Input_setting(const char *tag, const byte *value, size_t len)
  { // Called for settings retrieved from EEPROM
    if (!strncasecmp_P(tag, PSTR("input"), 5) && isdigit(tag[5]))
    { // Define input pin
      int i = atoi(tag + 5) - 1;
      if (i < 0 || i >= MAX_INPUT)
        return NULL;
      inputactive &= ~(1 << i);
      inputinvert &= ~(1 << i);
      if (len)
      {
        if (*value == '+' || *value == '-')
        { // active
          if (*value == '-')
            inputinvert |= (1 << i);
          value++;
          len--;
        }
        if (!len)
          return NULL;
        int p = 0;
        while (len && isdigit(*value))
        {
          p = p * 10 + *value++ -'0';
          len--;
        }
        if (len || p >= MAX_PIN)
          return NULL;
        inputpin[i] = p;
        inputactive |= (1 << i);
      }
      // Messy...
      const char*tagname[] = {PSTR("input1"), PSTR("input2"), PSTR("input3"), PSTR("input4"), PSTR("input5"), PSTR("input6"), PSTR("input7"), PSTR("input8"), PSTR("input9")};
      return tagname[i];
    }
#define s(n,d) do{const char *t=PSTR(#n);if(!strcmp_P(tag,t)){n=(value?atoi((char*)value):d);return t;}}while(0)
    app_settings
#undef s
    return NULL; // Done
  }

  boolean Input_command(const char*tag, const byte *message, size_t len)
  { // Called for incoming MQTT messages
    if (!inputactive)return false; // No inputs defined
    return false;
  }

  boolean Input_setup(ESPRevK&revk)
  {
    if (!inputactive && !input)return false; // No inputs defined
    debugf("GPIO available %X for %d inputs", gpiomap, input);
    int i;
    // Check assigned pins
    for (i = 0; i < MAX_INPUT; i++)
      if (inputactive & (1 << i))
      {
        if (!(gpiomap & (1 << inputpin[i])))
        { // Unusable
          Input_fault = PSTR("Pin assignment not available");
          inputactive &= ~(1 << i);
          continue;
        }
        gpiomap &= ~(1 << inputpin[i]);
      }
    if (input)
    { // Auto assign some pins (deprecated) - does not allocate 0/1/2
      for (i = 0; i < input; i++)
        if (!(inputactive & (1 << i)))
        {
          int p;
          for (p = 3; p < MAX_PIN && !(gpiomap & (1 << p)); p++); // Find a (safe) pin
          if (p == MAX_PIN)
          {
            Input_fault = PSTR("No input pins available to assign");
            break;
          }
          inputpin[i] = p;
          inputactive |= (1 << i);
          gpiomap &= ~(1 << inputpin[i]);
        }
    }
    debugf("GPIO remaining %X", gpiomap);
    for (i = 0; i < MAX_INPUT; i++)
      if (inputactive & (1 << i))
        pinMode(inputpin[i], inputpin[i] == 16 ? INPUT_PULLDOWN_16 : INPUT_PULLUP);
    debug("Input OK");
    return true;
  }

  boolean Input_loop(ESPRevK&revk, boolean force)
  {
    if (!inputactive)return false; // No inputs defined
    unsigned long now = millis();
    static unsigned long pincheck = 0;
    static unsigned long pinhold[MAX_INPUT] = {};
    if (force || (int)(pincheck - now) < 0)
    {
      pincheck = now + inputpoll;
      int i;
      for (i = 0; i < MAX_INPUT; i++)
        if (inputactive & (1 << i))
          if (force || !pinhold[i] || (int)(pinhold[i] - now) < 0)
          {
            pinhold[i] = 0;
            int r = (digitalRead(inputpin[i]) ? 1 : 0);
            if (inputinvert & (1 << i))
              r = 1 - r;
            if (force || r != ((inputstate & (1 << i)) ? 1 : 0))
            {
              if (r)inputstate |= (1 << i);
              else inputstate &= ~(1 << i);
              pinhold[i] = ((now + inputhold) ? : 1);
              char tag[8];
              snprintf_P(tag, sizeof(tag), "input%d", i + 1);
              revk.state(tag, F("%d"), r);
            }
          }
    }
    return true;
  }
