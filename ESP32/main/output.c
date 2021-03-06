// Logical outputs
// Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
static const char TAG[] = "output";
#include "SS.h"
const char *output_fault = NULL;
const char *output_tamper = NULL;

#include <driver/gpio.h>

// Output ports
#define MAXOUTPUT 26
#define BITFIELDS "-"
#define PORT_INV 0x40
#define port_mask(p) ((p)&63)
static uint8_t output[MAXOUTPUT];

static uint64_t output_state = 0;       // Port state
static uint64_t output_state_set = 0;   // Output has been set

int
output_active (int p)
{
   if (p < 1 || p > MAXOUTPUT)
      return -1;
   p--;
   if (output[p])
      return 1;
   return 0;
}

void
output_set (int p, int v)
{
   if (p < 1 || p > MAXOUTPUT)
      return;
   p--;
   if (v)
   {
      v = 1;
      if ((output_state & (1ULL << p)) && (output_state_set & (1ULL << p)))
         return;                // No change
      output_state |= (1ULL << p);
   } else
   {
      if (!(output_state & (1ULL << p)) && (output_state_set & (1ULL << p)))
         return;                // No change
      output_state &= ~(1ULL << p);
   }
   if (output[p])
   {
      gpio_hold_dis (port_mask (output[p]));
      gpio_set_level (port_mask (output[p]), (output[p] & PORT_INV) ? 1 - v : v);
      gpio_hold_en (port_mask (output[p]));
   }
   output_state_set |= (1ULL << p);
}

int
output_get (int p)
{
   if (p < 1 || p > MAXOUTPUT)
      return -1;
   p--;
   if (!(output_state_set & (1ULL << p)))
      return -1;
   if (output_state & (1ULL << p))
      return 1;
   return 0;
}

const char *
output_command (const char *tag, unsigned int len, const unsigned char *value)
{
   if (!strncmp (tag, TAG, sizeof (TAG) - 1))
   {                            // Set output
      int index = atoi (tag + sizeof (TAG) - 1);
      if (index >= 1 && index <= MAXOUTPUT)
      {
         if (len && *value == '1')
            output_set (index, 1);
         else
            output_set (index, 0);
         return "";             // Done
      }
   }
   return NULL;
}

void
output_init (void)
{
   revk_register (TAG, MAXOUTPUT, sizeof (*output), &output, "-", SETTING_BITFIELD | SETTING_SET);
   int i,
     p;
   for (i = 0; i < MAXOUTPUT; i++)
      if (output[i])
      {
         const char *e = port_check (p = port_mask (output[i]), TAG, 0);
         if (e)
         {
            status (output_fault = e);
            output[i] = 0;
         } else
         {
            REVK_ERR_CHECK (gpio_reset_pin (p));
            if (output[i] & PORT_INV)
               REVK_ERR_CHECK (gpio_set_level (p, 1));
            REVK_ERR_CHECK (gpio_set_direction (p, GPIO_MODE_OUTPUT));
            gpio_hold_en (p);
         }
      }
}
