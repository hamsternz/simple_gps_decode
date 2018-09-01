/******************************************************************************
* main.c - a simple extendable parser for GPS NAV messages
*
* Author: Mike Field - <hamster@snap.net.nz>
* 
******************************************************************************
* MIT License
*
* Copyright (c) 2018 Mike Field
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
******************************************************************************/
#include <stdio.h>
#include "../gps_parse.h"
/*****************************************************************************/
void test_GPGGA_callback( unsigned fix_quality, unsigned no_of_sats, double   timestamp,
       double   latitude, char     latitude_ns, double   longitude, char     longitude_ew,
       double   altitude, char     alt_units,   double   hor_dop)
{
    printf("Valid GPGGA fix %02u:%02u:%02u   %10.6f %c, %10.6f %c, "
           "fix quality %i, # of sats %i, D.O.P. %10.4f, Altitude %10.4f %c\n",
            (int)timestamp/10000, (int)timestamp/100%100, (int)timestamp%100,
            latitude,    latitude_ns,  longitude, longitude_ew,
	    fix_quality, no_of_sats,   hor_dop,   altitude, alt_units);
}

/*****************************************************************************/
void test_GPRMC_callback( double timestamp,   double   date_of_fix, char   nav_warning,
    double   latitude,    char   latitude_ns, double   longitude,   char   longitude_ew, 
    double   speed_knots, double course)
{
    printf("Valid GPRMC fix %02u:%02u:%02u %c %10.6f %c, %10.6f %c, speed %5.1f kts, course %5.1f, date %06u\n",
            (int)timestamp/10000, (int)timestamp/100%100, (int)timestamp%100,
	    nav_warning,
            latitude, latitude_ns,
            longitude, longitude_ew,
	    speed_knots, course, (unsigned)date_of_fix);
}

/*****************************************************************************/
void test_GPVTG_callback(double value, char unit) {
    printf("valid GPVTG: %10.6f %c\n", value, unit);
}

/*****************************************************************************/
void test_reject_callback(char *message, char *buffer) {
  if(buffer) 
    printf("Rejected %s: '%s'\n",message,buffer);
  else
    printf("Rejected %s\n",message);
}
/*****************************************************************************/
void test_GPGLL_callback( double   timestamp, 
		          double   latitude,  char     latitude_ns,
	                  double   longitude, char     longitude_ew) {
    printf("Valid GPGLL fix %f: %f %c, %f %c\n", timestamp,
            latitude, latitude_ns,
            longitude, longitude_ew);
}
/*****************************************************************************/
int main(int argc, char *argv[]) {
  int c;
  /* Set the callbacks */
  gps_GPGGA_callback_set(test_GPGGA_callback);
  gps_GPRMC_callback_set(test_GPRMC_callback);
  gps_GPVTG_callback_set(test_GPVTG_callback);
  gps_GPGLL_callback_set(test_GPGLL_callback);
  gps_reject_callback_set(test_reject_callback);

  /* Open the file */
  FILE *f = fopen(argv[1], "r");
  if(f == NULL) {
     printf("Unable to open input file\n");
     return 0;
  }

  /* Process the contents of the file */
  c = getc(f);
  while(c != EOF) {
     gps_add_char(c);
     c = getc(f);
  }
}
/************************ End of file  ***************************************/
