/* OpenSprinkler Unified (AVR/RPI/BBB/LINUX) Firmware
 * Copyright (C) 2015 by Ray Wang (ray@opensprinkler.com)
 *
 * OpenSprinkler library header file
 * Feb 2015 @ OpenSprinkler.com
 *
 * This file is part of the OpenSprinkler library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */


#ifndef _OPENSPRINKLER_H
#define _OPENSPRINKLER_H

#if defined(ARDUINO) // headers for AVR
  #include "Arduino.h"
  #include <avr/eeprom.h>
  #include <Wire.h>
  #include "LiquidCrystal.h"
  #include "Time.h"
  #include "DS1307RTC.h"
  #include "EtherCard.h"
#else // headers for RPI/BBB/LINUX
    #if defined((RPI-BBB-LCD)
      #include "LCD.h"
      #define I2C_DEV_ADDR 0x20	// PUT YOUR I2C LCD BACKPACK ADDRESS HERE!
      #define I2C_NUM_BUS 1		// 0 for RPi B rev 1, 1 for pretty much all else
      #define RS_PIN 6
      #define RW_PIN 5
      #define EN_PIN 4
      #define BL_PIN 7
      #define D4_PIN 0
      #define D5_PIN 1
      #define D6_PIN 2
      #define D7_PIN 3
    #endif
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <time.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <sys/ioctl.h>
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <err.h>
#endif // end of headers

#include "defines.h"
#include "utils.h"

/** Option data structure */
struct OptionStruct{
  byte value;     // each option value is stored as a byte
  byte max;       // maximum value of the option
  const char* str;      // full name
  const char* json_str; // json name
};

/** Non-volatile data */
struct NVConData {
  uint16_t sunrise_time;  // sunrise time (in minutes)
  uint16_t sunset_time;   // sunset time (in minutes)
  uint32_t rd_stop_time;  // rain delay stop time
};

/** Volatile controller status bits */
struct ConStatus {
  byte enabled:1;           // operation enable (when set, controller operation is enabled)
  byte rain_delayed:1;      // rain delay bit (when set, rain delay is applied)
  byte rain_sensed:1;       // rain sensor bit (when set, it indicates that rain is detected)
  byte program_busy:1;      // HIGH means a program is being executed currently
  byte has_rtc:1;           // HIGH means the controller has a DS1307 RTC
  byte has_sd:1;            // HIGH means a microSD card is detected
  byte wt_received:1;       // HIGH means weather info has been received
  byte dummy:1;             // dummy bit
  byte display_board:4;     // the board that is being displayed onto the lcd
  byte network_fails:4;     // number of network fails
  byte mas:8;               // master station index
};

class OpenSprinkler {
public:

  // data members
#if defined(ARDUINO) || (RPI-BBB-LCD)
  static LiquidCrystal lcd;
#elif defined(RPI-BBB-LCD)
  static LCD lcd;
#endif

  static NVConData nvdata;
  static ConStatus status;
  static ConStatus old_status;
  static byte nboards, nstations;

  static OptionStruct options[];  // option values, max, name, and flag

  static byte station_bits[];     // station activation bits. each byte corresponds to a board (8 stations)
                                  // first byte-> master controller, second byte-> ext. board 1, and so on
  // station attributes
  static byte masop_bits[];       // station master operation bits. each byte corresponds to a board (8 stations)
  static byte ignrain_bits[];     // ignore rain bits. each byte corresponds to a board (8 stations)
  static byte actrelay_bits[];    // activate relay bits. each byte corresponds to a board (8 stations)
  static byte stndis_bits[];      // station disable bits. each byte corresponds to a board (8 stations)
  static byte rfstn_bits[];       // RF station flags. each byte corresponds to a board (8 stations)
  static byte stnseq_bits[];      // station sequential bits. each byte corresponds to a board (8 stations)

  // variables for time keeping
  static ulong rainsense_start_time;  // time when the most recent rain sensor activation was detected
  static ulong raindelay_start_time;  // time when the most recent rain delay started
  static ulong button_lasttime;       // time when button was checked
  static ulong ntpsync_lasttime;      // time when ntp sync was performed
  static ulong checkwt_lasttime;      // time when weather was checked
  static ulong network_lasttime;      // time when network was checked
  static ulong dhcpnew_lasttime;      // time when dhcp renew was performed
  static ulong external_ip;           // external ip address
  static byte  water_percent_avg;     // average water percentage over a day

  // member functions
  // -- setup
  static void reboot_dev();   // reboot the microcontroller
  static void begin();        // initialization, must call this function before calling other functions
  static byte start_network();  // initialize network with the given mac and port
#if defined(ARDUINO)
  static bool read_hardware_mac();  // read hardware mac address
#endif
  static time_t now_tz();
  // -- station names and attributes
  static void get_station_name(byte sid, char buf[]); // get station name
  static void set_station_name(byte sid, char buf[]); // set station name
  static uint16_t get_station_name_rf(byte sid, ulong *on, ulong *off); // get station name and parse into RF code
  static void update_rfstation_bits();
  static void send_rfstation_signal(byte sid, bool status);
  static void station_attrib_bits_save(int addr, byte bits[]); // save station attribute bits to nvm
  static void station_attrib_bits_load(int addr, byte bits[]); // load station attribute bits from nvm

  // -- options and data storeage
  static void nvdata_load();
  static void nvdata_save();

  static void options_setup();
  static void options_load();
  static void options_save();

  static byte password_verify(char *pw);  // verify password

  // -- controller operation
  static void enable();           // enable controller operation
  static void disable();          // disable controller operation, all stations will be closed immediately
  static void raindelay_start();  // start raindelay
  static void raindelay_stop();   // stop rain delay
  static void rainsensor_status();// update rainsensor status
  static int detect_exp();        // detect the number of expansion boards
  static byte weekday_today();    // returns index of today's weekday (Monday is 0)
  static void set_relay(byte status); // set relay on or off

  static void set_station_bit(byte sid, byte value); // set station bit of one station (sid->station index, value->0/1)
  static void clear_all_station_bits(); // clear all station bits
  static void apply_all_station_bits(); // apply all station bits (activate/deactive values)

  // -- LCD functions
#if defined(ARDUINO) || (RPI-BBB-LCD) // LCD functions for AVR/RPi/BBB
  static void lcd_print_pgm(PGM_P PROGMEM str);           // print a program memory string
  static void lcd_print_line_clear_pgm(PGM_P PROGMEM str, byte line);
  static void lcd_print_time(byte line);                  // print current time
  static void lcd_print_ip(const byte *ip, byte line);    // print ip
  static void lcd_print_mac(const byte *mac);             // print mac
  static void lcd_print_station(byte line, char c);       // print station bits of the board selected by display_board
  static void lcd_print_version(byte v);                   // print version number

  #if defined(ARDUINO) // UI functions for AVR
    // -- UI and buttons
    static byte button_read(byte waitmode); // Read button value. options for 'waitmodes' are:
                                            // BUTTON_WAIT_NONE, BUTTON_WAIT_RELEASE, BUTTON_WAIT_HOLD
                                            // return values are 'OR'ed with flags
                                            // check defines.h for details
  
    // -- UI functions --
    static void ui_set_options(int oid);    // ui for setting options (oid-> starting option index)
    static void lcd_set_brightness();
    static void lcd_set_contrast();
  #endif
#endif
private:
#if defined(ARDUINO) || (RPI-BBB-LCD) // LCD functions for AVR/RPi/BBB
  static void lcd_print_option(int i);  // print an option to the lcd
  static void lcd_print_2digit(int v);  // print a integer in 2 digits
  #if defined(ARDUINO) // UI functions for AVR
    static byte button_read_busy(byte pin_butt, byte waitmode, byte butt, byte is_holding);
  #endif
#endif // LCD functions
};

#endif  // _OPENSPRINKLER_H
