/******************************************************************************
* gps_parse.c - a simple extendable parser for GPS NAV messages
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
#include <stdio.h>
#include "gps_parse.h"

static enum {
	state_wait_for_nl,
	state_should_be_dollar,
	state_should_be_NMEA,
	state_checksum1,
	state_checksum2,
	state_should_be_nl
} state = state_wait_for_nl;

static char checksum = 0;
static char synced = 0;
#define BUFFER_SIZE 128
static int  buffer_used = 0;
static char buffer[BUFFER_SIZE];

static gps_GPGGA_callback_func  GPGGA_callback;
static gps_GPRMC_callback_func  GPRMC_callback;
static gps_GPVTG_callback_func  GPVTG_callback;
static gps_GPGLL_callback_func  GPGLL_callback;
static gps_reject_callback_func reject_callback;
static gps_no_fix_callback_func no_fix_callback;

/****************************************************************************/
static void reject(char *msg, char *buffer) {
  if(reject_callback != NULL) 
    reject_callback(msg, buffer);
}
/****************************************************************************/
static int sentence_type(char *header) {
  int i = 0;
  while(header[i] != '\0') {
    if(header[i] != buffer[i])
      return 0;
    i++;
  }

  if(buffer[i] != ',')
    return 0;
  /* Match */
  return 1;
}
/****************************************************************************/
static int field_present(int fieldno) {
  int i = 0;
  while(fieldno > 0) {
    if(buffer[i] == '\0') return 0;
    if(buffer[i] == ',') fieldno--;
    i++;
  }
  if(buffer[i] == ',')  return 0;
  if(buffer[i] == '\0') return 0;
  return 1;
}
/****************************************************************************/
static int parse_char(int fieldno, char *dest, char*acceptible) {
  int i = 0;
  while(fieldno > 0) {
    if(buffer[i] == '\0') return 0;
    if(buffer[i] == ',') fieldno--;
    i++;
  }

  /* Look for match */
  while(*acceptible != '\0' && *acceptible != buffer[i]) 
     acceptible++;

  /* Check a match was found */
  if(*acceptible == '\0') return 0;

  /* Check that field is correctly terminated */
  if(buffer[i+1] != ',' && buffer[i+1] != '\0') return 0;

  *dest = buffer[i];
  return 1;
}

/****************************************************************************/
static int parse_uint(int fieldno, unsigned *dest) {
  unsigned i = 0;
  unsigned value = 0;
  while(fieldno > 0) {
    if(buffer[i] == '\0') return 0;
    if(buffer[i] == ',') fieldno--;
    i++;
  }

  while(buffer[i] >= '0' && buffer[i] <= '9') {
    value = value*10 + buffer[i]-'0';
    i++;
  }
  /* Check that field is correctly terminated */
  if(buffer[i] != ',' && buffer[i] != '\0') return 0;

  *dest = value;
  return 1;
}

/****************************************************************************/
static int parse_double(int fieldno, double *dest) {
  unsigned i = 0;
  double value = 0;
  int decimals = 0;
  while(fieldno > 0) {
    if(buffer[i] == '\0') return 0;
    if(buffer[i] == ',') fieldno--;
    i++;
  }

  while(buffer[i] >= '0' && buffer[i] <= '9') {
    value = value*10 + buffer[i]-'0';
    i++;
  }
  if(buffer[i] == '.') {
    i++;
    while(buffer[i] >= '0' && buffer[i] <= '9') {
      value = value*10 + buffer[i]-'0';
      i++;
      decimals++;
    }
  }
  while(decimals > 0) {
    decimals--;
    value /= 10.0;
  }
  /* Check that field is correctly terminated */
  if(buffer[i] != ',' && buffer[i] != '\0') return 0;

  *dest = value;
  return 1;
}

/****************************************************************************/
static int parse_angle(int fieldno, double *dest) {
  unsigned i = 0;
  unsigned degrees = 0;
  double   minutes = 0;
  int      decimals = 0;
  int      sign;

  while(fieldno > 0) {
    if(buffer[i] == '\0') return 0;
    if(buffer[i] == ',') fieldno--;
    i++;
  }

  if(buffer[i] == '-') {
    sign = -1;
    i++;
  } else {
    sign = 1;
  }

  while(buffer[i] >= '0' && buffer[i] <= '9') {
    degrees = degrees*10 + buffer[i]-'0';
    i++;
  }

  minutes  = degrees%100;
  degrees /= 100;

  if(buffer[i] == '.') {
    i++;
    while(buffer[i] >= '0' && buffer[i] <= '9') {
      minutes = minutes*10 + buffer[i]-'0';
      i++;
      decimals++;
    }
  }
  while(decimals > 0) {
    decimals--;
    minutes /= 10.0;
  }

  /* Check that field is correctly terminated */
  if(buffer[i] != ',' && buffer[i] != '\0') return 0;

  minutes /=60;
  *dest = degrees+minutes;
  if(sign < 0)
    *dest = -*dest;
  return 1;
}

/****************************************************************************/
static int parse_GPGGA(void) {
  unsigned fix_quality = 0;
  unsigned no_of_sats = 0;
  double   timestamp = 0;
  double   latitude = 0;
  char     latitude_ns = 'N';
  double   longitude = 0;
  char     longitude_ew = 'E';
  double   altitude = 0;
  char     alt_units = 0;
  double   hor_dop = 0;

  if(!parse_double( 1, &timestamp         )) return 0;
  if(!field_present(2)) return 1;
  if(!parse_angle(  2, &latitude          )) return 0;
  if(!parse_char(   3, &latitude_ns,  "NS")) return 0;
  if(!parse_angle(  4, &longitude         )) return 0;
  if(!parse_char(   5, &longitude_ew, "EW")) return 0;
  if(!parse_uint(   6, &fix_quality       )) return 0;
  if(!parse_uint(   7, &no_of_sats        )) return 0;
  if(!parse_double( 8, &hor_dop           )) return 0;
  if(!parse_double( 9, &altitude          )) return 0;
  if(!parse_char(  10, &alt_units,     "M")) return 0;


  if(GPGGA_callback) {
    GPGGA_callback(fix_quality, no_of_sats,  timestamp,
                   latitude,    latitude_ns, longitude, longitude_ew,
                   altitude,    alt_units,   hor_dop);
  }
  return 1;
}

/****************************************************************************/
static int parse_GPRMC(void) { 
  double   timestamp    = 0.0;
  double   date_of_fix  = 0.0;
  char     nav_warning  = 'A';
  double   latitude     = 0.0;
  char     latitude_ns  = 'N';
  double   longitude    = 0.0;
  char     longitude_ew = 'E';
  double   speed_knots  = 0.0;
  double   course       = 0.0;


  if(!parse_double( 1, &timestamp        )) return 0;
  if(!parse_char(   2, &nav_warning, "AV")) return 0;
  if(nav_warning == 'V') return 1;
  if(!parse_angle(  3, &latitude         )) return 0;
  if(!parse_char(   4, &latitude_ns, "NS")) return 0;
  if(!parse_angle(  5, &longitude        )) return 0;
  if(!parse_char(   6, &longitude_ew,"EW")) return 0;
  if(!parse_double( 7, &speed_knots      )) return 0;
  if(!parse_double( 8, &course           )) return 0;
  if(!parse_double( 9, &date_of_fix      )) return 0;

  if(GPRMC_callback) 
     GPRMC_callback(timestamp,   date_of_fix, nav_warning,
                    latitude,    latitude_ns, longitude,   longitude_ew,
                    speed_knots, course);

  return 1;
}
/****************************************************************************/
static int parse_GPGLL(void) {
  double   timestamp    = 0.0;
  double   latitude     = 0;
  char     latitude_ns  = 'N';
  double   longitude    = 0;
  char     longitude_ew = 'E';

  if(!parse_angle(  1, &latitude         )) return 0;
  if(!parse_char(   2, &latitude_ns, "NS")) return 0;
  if(!parse_angle(  3, &longitude        )) return 0;
  if(!parse_char(   4, &longitude_ew,"EW")) return 0;
  if(!parse_double( 5, &timestamp        )) return 0;

  if(GPGLL_callback)
	GPGLL_callback(timestamp, latitude, latitude_ns, longitude, longitude_ew);
  return 1;
}

/****************************************************************************/
static int parse_GPGSA(void) {
  return 1;
}

/****************************************************************************/
static int parse_GPGSV(void) {
  return 1;
}

/****************************************************************************/
static int parse_GPVTG(void) {
  double value = 0;
  char   ref   = 'X';
  int i;
  i = 1;
  while(1) {
    if(!parse_double(  i, &value        )) return 1;
    if(!parse_char(  i+1, &ref,   "TMNK")) return 0;
    if(GPVTG_callback) {
      GPVTG_callback(value,ref);
    }
    i += 2;
  }
  return 1;
}

/****************************************************************************/
static void parse_data(void) {
  if(sentence_type("GPGGA")) {
     if(!parse_GPGGA())
       reject("Parse error",buffer);
     return;
  }

  if(sentence_type("GPGLL")) {
     if(!parse_GPGLL())
       reject("Parse error",buffer);
     return;
  }

  if(sentence_type("GPGSA")) {
     if(!parse_GPGSA())
       reject("Parse error",buffer);
     return;
  }

  if(sentence_type("GPRMC")) {
     if(!parse_GPRMC())
       reject("Parse error",buffer);
     return;
  }

  if(sentence_type("GPGSV")) {
     if(!parse_GPGSV())
       reject("Parse error",buffer);
     return;
  }

  if(sentence_type("GPVTG")) {
     if(!parse_GPVTG())
       reject("Parse error",buffer);
     return;
  }
  reject("Unknown sentence", buffer);
}
/****************************************************************************/
static int is_hex_char(char c) {
  if( c >= '0' && c <= '9') return 1;
  if( c >= 'A' && c <= 'F') return 1;
  return 0;
}
/****************************************************************************/
static int is_NMEA_char(char c) {
  if( c >= '0' && c <= '9') return 1;
  if( c >= 'A' && c <= 'Z') return 1;
  if( c == ',')             return 1;
  if( c == '.')             return 1;
  return 0;
}
/****************************************************************************/
void gps_add_char(int c) {
  switch(state) {
    case state_wait_for_nl:
      if(c != '\n') break;
      state = state_should_be_dollar;
      return;

    case state_should_be_dollar:
      if(c != '$') break;

      state = state_should_be_NMEA;
      checksum = 0;
      buffer_used = 0;
      return;

    case state_should_be_NMEA:
      if(c == '*') {
	buffer[buffer_used++] = 0;
	state = state_checksum1;
        return;
      }

      if(!is_NMEA_char(c)) break;
      /* Add to buffer */
      if(buffer_used == BUFFER_SIZE-1) {
        reject("NMEA sentence too long",NULL);
        break;
      }

      buffer[buffer_used++] = c;	
      checksum ^= c;
      return;

    case state_checksum1:
      if(!is_hex_char(c)) break;
      /* Remove from checksum */
      if(c >= '0' && c <= '9') checksum ^= (c-'0')<<4;
      if(c >= 'A' && c <= 'F') checksum ^= (c-'A'+10)<<4;
      state = state_checksum2;
      return;

    case state_checksum2:
      if(!is_hex_char(c)) break; 
      /* Remove from checksum */
      if(c >= '0' && c <= '9') checksum ^= (c-'0');
      if(c >= 'A' && c <= 'F') checksum ^= (c-'A'+10);
      state = state_should_be_nl;
      return;

    case state_should_be_nl:
      if(c == '\r') return;
      if(c != '\n') break;

      if(checksum != 0) {
	 reject("Invalid checksum",buffer);
	 return;
      }
      parse_data();
      synced = 1;
      state  = state_should_be_dollar;
      return;
  }
  state  = state_wait_for_nl;
  synced = 0;
}

/****************************************************************************/
gps_GPGGA_callback_func  gps_GPGGA_callback_set(gps_GPGGA_callback_func cb) {
  gps_GPGGA_callback_func rtn = GPGGA_callback;
  GPGGA_callback = cb;
  return rtn;
}

/****************************************************************************/
gps_GPRMC_callback_func  gps_GPRMC_callback_set(gps_GPRMC_callback_func cb) {
  gps_GPRMC_callback_func rtn = GPRMC_callback;
  GPRMC_callback = cb;
  return rtn;
}

/****************************************************************************/
gps_GPVTG_callback_func  gps_GPVTG_callback_set(gps_GPVTG_callback_func cb) {
  gps_GPVTG_callback_func rtn = GPVTG_callback;
  GPVTG_callback = cb;
  return rtn;
}

/****************************************************************************/
gps_GPGLL_callback_func  gps_GPGLL_callback_set(gps_GPGLL_callback_func cb) {
  gps_GPGLL_callback_func rtn = GPGLL_callback;
  GPGLL_callback = cb;
  return rtn;
}

/****************************************************************************/
gps_reject_callback_func gps_reject_callback_set(gps_reject_callback_func cb){
  gps_reject_callback_func rtn = reject_callback;
  reject_callback = cb;
  return rtn;
}

/****************************************************************************/
gps_no_fix_callback_func gps_no_fix_callback_set(gps_no_fix_callback_func cb){
  gps_no_fix_callback_func rtn = no_fix_callback;
  no_fix_callback = cb;
  return rtn;
}

/****************************************************************************/
