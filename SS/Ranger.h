// Solar System
// (c) Andrews & Arnold Ltd, Adrian Kennard, see LICENSE file (GPL)

// Laser ranger as button

#define	USE_RANGER

#include <ESP8266RevK.h>

extern boolean ranger_setting(const char *setting, const byte *value, size_t len);
extern boolean ranger_cmnd(const char*suffix, const byte *message, size_t len);
extern void ranger_setup(ESP8266RevK&);
extern void ranger_loop(ESP8266RevK&);


