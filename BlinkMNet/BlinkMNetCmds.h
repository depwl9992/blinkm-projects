//
// BlinkMNet command script for ASTERISK
//
// There are 8 panels, arranged as four Arduinos with 2 BlinkM MaxMs each.
// Each MaxM is driving white panels only, to full brightness, on all channels.
//
//
// The 8 panels spell out the word "ASTERISK". one letter per panel.
// The timing is like below.  There are 27 frames, each "frame" taking 500 msec.
//  frame          11111111112222222
//       012345678901234567890123456   New
//  Adr  ---------------------------   Adr
//  11   AAAAAAAA                      12
//  12   SSSSSSS                       14
//  13   TTTTTT      T                 16
//  14   EEEEE   E                     18
//  15   RRRR      R        R R  R     11
//  16   III           I I IIII II     13
//  17   ZZ         Z       Z Z  Z     15
//  18   K        K         K K  K     17
//       ----------------------------
//       0123456789012345678901234562
//                 111111111122222222
//

const byte fadespeed = 100;
const int msframe = 500;

const byte LED__ = 0;
const byte LED_A = 12;
const byte LED_S = 14;
const byte LED_T = 16;
const byte LED_E = 18;
const byte LED_R = 11;
const byte LED_I = 13;
const byte LED_Z = 15;
const byte LED_K = 17;


// some ease-of-use defines
#define led_on(l,d,t)           { l, 'c', 0xff,0xff,0xff, d, t }
#define led_off(l,d,t)          { l, 'c', 0x00,0x00,0x00, d, t }
#define led_colr(l,r,g,b, d,t)  { l, 'c', r,   g,   b,    d, t }


/*
// for testing / debugging
const PROGMEM char cmdline_id[] = "Testing Script";
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  led_on(  LED_A, "tst1",    0),
  led_off( LED_A, "tst2", 1500),
  led_colr(LED_T, 0xcc,0x33,0x22, "tst3", 1500),
  { LED_A, 'c',  0xff,0xff,0xff, "one ",    0 },  // t=0, both on
  { LED_S, 'c',  0xff,0xff,0xff, "two ", 1500 },  // t=0, both on
  { LED_A, 'c',  0x00,0x00,0x00, "thre", 1500 },  // 1500ms later, A off
  { LED_S, 'c',  0xff,0xff,0xff, "four",    0 },  // 1500ms later, A on, S off
  { LED_A, 'c',  0x00,0x00,0x00, "five", 1500 },
  { LED__, 'c',  0x00,0x00,0x00, "six",     0 },  // 1500ms later, all off
};
*/

const PROGMEM char cmdline_id[] = "ASTERISK for Hammer";
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  { LED__, 'o',  0x00,0x00,0x00, "strt", 0 }, // turn off built-in script 
  { LED__, 'f',  fadespeed, 0x00,0x00, "fads", 0 }, // set fadespeed
  led_on( LED_A, "f00a", 0 ),      // frame 0: all on
  led_on( LED_S, "f00b", 0 ),
  led_on( LED_T, "f00c", 0 ),
  led_on( LED_E, "f00d", 0 ),
  led_on( LED_R, "f00e", 0 ),
  led_on( LED_I, "f00f", 0 ),
  led_on( LED_Z, "f00g", 0 ),  
  led_on( LED_K, "f00h", msframe ),  // then 500ms later...
  led_off(LED_K, "f01a", msframe ),  // frame  1: turn off K & 500ms later...
  led_off(LED_Z, "f02a", msframe ),  // frame  2: turn off Z & 500ms later...
  led_off(LED_I, "f03a", msframe ),  // frame  3: turn off I & 500ms later...
  led_off(LED_R, "f04a", msframe ),  // frame  4: turn off R & 500ms later...
  led_off(LED_E, "f05a", msframe ),  // frame  5: turn off E & 500ms later...
  led_off(LED_T, "f06a", msframe ),  // frame  6: turn off T & 500ms later...
  led_off(LED_S, "f07a", msframe ),  // frame  7: turn off S & 500ms later...

  led_off(LED_A, "f08a",   0 ),      // frame  8: turn off A 
  led_on( LED_E, "f08b", msframe ),  // frame  8: turn on  E & 500ms later... 

  led_off(LED_E, "f09a",   0 ),      // frame  9: turn off E
  led_on( LED_K, "f09b", msframe ),  // frame  9: turn on  K & 500ms later...

  led_off(LED_K, "f10a",   0 ),      // frame 10: turn off K 
  led_on( LED_R, "f10b", msframe ),  // frame 10: turn on  R & 500ms later...

  led_off(LED_R, "f11a",   0 ),      // frame 11: turn off R 
  led_on( LED_S, "f11b", msframe ),  // frame 11: turn on  S & 500ms later...

  led_off(LED_S, "f12a",   0 ),      // frame 12: turn off S 
  led_on( LED_T, "f12b", msframe ),  // frame 12: turn on  T & 500ms later...

  led_off(LED__, "f13a", msframe ),  // frame 13: all off & 500 ms later...

  led_on( LED_I, "f14a", msframe ),  // frame 14: turn on  I & 500ms later...
  led_off(LED_I, "f15a", msframe ),  // frame 15: turn off I & 500ms later...

  led_on( LED_I, "f16a", msframe ),  // frame 16: turn on  I & 500ms later...
  led_off(LED_I, "f17a", msframe ),  // frame 17: turn off I & 500ms later...

  led_on( LED_I, "f18a", msframe ),  // frame 18: turn on  I & 500ms later...

  led_on( LED_R, "f19a",   0 ),      // frame 19: turn on  R,
  led_on( LED_I, "f19b",   0 ),      // frame 19: turn on  I, (yes redundant)
  led_on( LED_R, "f19c",   0 ),      // frame 19: turn on  Z,
  led_on( LED_R, "f19d", msframe ),  // frame 19: turn on  K & 500ms later...

  led_off(LED_R, "f20a",   0 ),      // frame 20: turn off R,
  led_off(LED_Z, "f20b",   0 ),      // frame 20: turn off Z,
  led_off(LED_K, "f20c", msframe ),  // frame 20: turn off K & 500ms later...

  led_on( LED_R, "f21a",   0 ),      // frame 21: turn on  R,
  led_on( LED_I, "f21b",   0 ),      // frame 21: turn on  I, (yes redundant)
  led_on( LED_R, "f21c",   0 ),      // frame 21: turn on  Z,
  led_on( LED_R, "f21d", msframe ),  // frame 21: turn on  K & 500ms later...

  led_off(LED__, "f22a", msframe ),  // frame 22 all off & 500 ms later...

  led_on( LED_I, "f23a", msframe ),  // frame 23: turn on  I & 500ms later...

  led_on( LED_R, "f24a",   0 ),      // frame 24: turn on  R,
  led_on( LED_I, "f24b",   0 ),      // frame 24: turn on  I, (yes redundant)
  led_on( LED_R, "f24c",   0 ),      // frame 24: turn on  Z,
  led_on( LED_R, "f24d", msframe ),  // frame 24: turn on  K & 500ms later...

  led_off(LED__, "f25a", msframe ),  // frame 25 all off & 500 ms later...
  led_off(LED__, "f26a", msframe ),  // frame 26 all off & 500 ms later...

};

/*
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  {  10, LED__, 'o',  0x00,0x00,0x00, "stof" },
  {  10, LED__, 'f',  100, 0x00,0x00, "fads" }, // set fadespeed
  {   0, LED_A, 'c',  0xff,0xff,0xff, "f00a" }, // frame 0: all on
  {   0, LED_S, 'c',  0xff,0xff,0xff, "f00b" }, // all at same time, so 0 time
  {   0, LED_T, 'c',  0xff,0xff,0xff, "f00c" },
  {   0, LED_E, 'c',  0xff,0xff,0xff, "f00d" },
  {   0, LED_R, 'c',  0xff,0xff,0xff, "f00e" },
  {   0, LED_I, 'c',  0xff,0xff,0xff, "f00f" },
  {   0, LED_Z, 'c',  0xff,0xff,0xff, "f00g" },
  { 500, LED_K, 'c',  0xff,0xff,0xff, "f00h" },
  { 500, LED_K, 'c',  0x00,0x00,0x00, "f01 " }, // frame  1: off K
  { 500, LED_Z, 'c',  0x00,0x00,0x00, "f02 " }, // frame  2: off Z
  { 500, LED_I, 'c',  0x00,0x00,0x00, "f03 " }, // frame  3: off I
  { 500, LED_R, 'c',  0x00,0x00,0x00, "f04 " }, // frame  4: off R
  { 500, LED_E, 'c',  0x00,0x00,0x00, "f05 " }, // frame  5: off E
  { 500, LED_T, 'c',  0x00,0x00,0x00, "f06 " }, // frame  6: off T
  { 500, LED_S, 'c',  0x00,0x00,0x00, "f07 " }, // frame  7: off S

  {   0, LED_A, 'c',  0x00,0x00,0x00, "f08a" }, // frame  8: off A, on E
  { 500, LED_E, 'c',  0xff,0xff,0xff, "f08b" },

  {   0, LED_E, 'c',  0x00,0x00,0x00, "f09a" }, // frame  9: off E, on K
  { 500, LED_K, 'c',  0xff,0xff,0xff, "f09b" },

  {   0, LED_K, 'c',  0x00,0x00,0x00, "f10a" }, // frame 10: off K, on R
  { 500, LED_R, 'c',  0xff,0xff,0xff, "f10b" },

  {   0, LED_R, 'c',  0x00,0x00,0x00, "f11a" }, // frame 11: off R, on Z
  { 500, LED_Z, 'c',  0xff,0xff,0xff, "f11b" },

  {   0, LED_Z, 'c',  0x00,0x00,0x00, "f12a" }, // frame 12: off Z, on T
  { 500, LED_T, 'c',  0xff,0xff,0xff, "f12b" },

  { 500, LED__, 'c',  0x00,0x00,0x00, "f13 " }, // frame 13 off T (all)
  { 500, LED_I, 'c',  0xff,0xff,0xff, "f14 " }, // frame 14:  on I
  { 500, LED_I, 'c',  0x00,0x00,0x00, "f15 " }, // frame 15: off I
  { 500, LED_I, 'c',  0xff,0xff,0xff, "f16 " }, // frame 16:  on I
  { 500, LED_I, 'c',  0x00,0x00,0x00, "f17 " }, // frame 17: off I

  { 500, LED_I, 'c',  0xff,0xff,0xff, "f18 " }, // frame 18:  on I

  {   0, LED_R, 'c',  0xff,0xff,0xff, "f19a" }, // frame 19: on R,Z,K
  {   0, LED_Z, 'c',  0xff,0xff,0xff, "f19b" }, // 
  { 500, LED_K, 'c',  0xff,0xff,0xff, "f19c" }, // 

  {   0, LED_R, 'c',  0x00,0x00,0x00, "f20a" }, // frame 20: off R,Z,K
  {   0, LED_Z, 'c',  0x00,0x00,0x00, "f20b" }, // 
  { 500, LED_K, 'c',  0x00,0x00,0x00, "f20c" }, // 

  {   0, LED_R, 'c',  0xff,0xff,0xff, "f21a" }, // frame 21: on R,Z,K
  {   0, LED_Z, 'c',  0xff,0xff,0xff, "f21b" }, // 
  { 500, LED_K, 'c',  0xff,0xff,0xff, "f21c" }, // 

  {   0, LED_R, 'c',  0x00,0x00,0x00, "f22a" }, // frame 22: off R,Z,K
  {   0, LED_Z, 'c',  0x00,0x00,0x00, "f22c" }, // 
  { 500, LED_K, 'c',  0x00,0x00,0x00, "f22d" }, // 

  { 500, LED_I, 'c',  0xff,0xff,0xff, "f23 " }, // frame 23: on I

  {   0, LED_R, 'c',  0xff,0xff,0xff, "f24a" }, // frame 24: on R,Z,K
  {   0, LED_Z, 'c',  0xff,0xff,0xff, "f24b" }, // 
  { 500, LED_K, 'c',  0xff,0xff,0xff, "f24c" }, // 

  { 500, LED__, 'c',  0x00,0x00,0x00, "f25 " }, // frame 25: off all

  { 500, LED__, 'c',  0x00,0x00,0x00, "f26 " }, // frame 26: off all
};  
*/


/*
const PROGMEM char cmdline_id[] = "ASTERISK Test for Hammer";
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  {  0, 'o', 0x00,0x00,0x00, "stof",   0 },
  {  0, 'c', 0xff,0xff,0xff, "f00a", 500 }, // frame 0: all on
};
*/


// DON'T TOUCH THIS LINE
const int cmdlines_len = sizeof(cmdlines)/sizeof(cmdline);

