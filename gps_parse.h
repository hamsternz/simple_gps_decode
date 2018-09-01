/******************************************************************************
* gps_parse.h - a simple extendable parser for GPS NAV messages
*
* Author: Mike Field - <hamster@snap.net.nz>
* 
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
******************************************************************************/

#ifndef GPS_PARSE_H
#define GPS_PARSE_H
void gps_add_char(int c);

typedef void (*gps_reject_callback_func)(char *message, char *buffer);
typedef void (*gps_no_fix_callback_func)(void);
typedef void (*gps_GPGGA_callback_func)( unsigned fix_quality, unsigned no_of_sats, double   timestamp,
       double   latitude, char     latitude_ns, double   longitude, char     longitude_ew,
       double   altitude, char     alt_units,   double   hor_dop);
typedef void (*gps_GPRMC_callback_func)( double timestamp,   double   date_of_fix, char   nav_warning,
    double   latitude,    char   latitude_ns, double   longitude,   char   longitude_ew, 
    double   speed_knots, double course);
typedef void (*gps_GPVTG_callback_func)(double value, char unit);
typedef void (*gps_GPGLL_callback_func)( double   timestamp, double   latitude,  char     latitude_ns,
	                  double   longitude, char     longitude_ew);

gps_reject_callback_func gps_reject_callback_set(gps_reject_callback_func cb);
gps_no_fix_callback_func gps_no_fix_callback_set(gps_no_fix_callback_func cb);
gps_GPGGA_callback_func  gps_GPGGA_callback_set(gps_GPGGA_callback_func cb);
gps_GPRMC_callback_func  gps_GPRMC_callback_set(gps_GPRMC_callback_func cb);
gps_GPVTG_callback_func  gps_GPVTG_callback_set(gps_GPVTG_callback_func cb);
gps_GPGLL_callback_func  gps_GPGLL_callback_set(gps_GPGLL_callback_func cb);
#endif
/************************ End of file  ***************************************/
