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

// an unused idea to make script definitions easier
//#define LED_A  11
//#define LED_S  12
//#define LED_T  13
//#define led_on(x,y)  {{ x, y, 'c', 0xff,0xff,0xff}}
//#define led_off(x,y) {{ x, y, 'c', 0x00,0x00,0x00}}
// it would then be used like:
//  { led_on(500, LED_A) },
//  { led_of(0,   LED_S) },

/*
const PROGMEM char cmdline_id[] = "ASTERISK Test for Hammer";
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  {  10,  0, 'o',  0x00,0x00,0x00, "stof" },
  { 500,  0, 'c',  0xff,0xff,0xff, "f00a" }, // frame 0: all on
};
*/

const byte LED_ALL =0;
const byte LED_A = 12;
const byte LED_S = 14;
const byte LED_T = 16;
const byte LED_E = 18;
const byte LED_R = 11;
const byte LED_I = 13;
const byte LED_Z = 15;
const byte LED_K = 17;

const PROGMEM char cmdline_id[] = "ASTERISK for Hammer";
const cmdline cmdlines[] PROGMEM = { 
// dur, addr,cmd,  args 
  {  10, LED_ALL, 'o',  0x00,0x00,0x00, "stof" },
  {  10, LED_ALL, 'f',  100, 0x00,0x00, "fads" }, // set fadespeed
  { 500, LED_A,   'c',  0xff,0xff,0xff, "f00a" }, // frame 0: all on
  {   0, LED_S,   'c',  0xff,0xff,0xff, "f00b" }, // all at same time, so 0 time
  {   0, LED_T,   'c',  0xff,0xff,0xff, "f00c" },
  {   0, LED_E,   'c',  0xff,0xff,0xff, "f00d" },
  {   0, LED_R,   'c',  0xff,0xff,0xff, "f00e" },
  {   0, LED_I,   'c',  0xff,0xff,0xff, "f00f" },
  {   0, LED_Z,   'c',  0xff,0xff,0xff, "f00g" },
  {   0, LED_K,   'c',  0xff,0xff,0xff, "f00h" },
  { 500, LED_K,   'c',  0x00,0x00,0x00, "f01 " }, // frame  1: off K
  { 500, LED_Z,   'c',  0x00,0x00,0x00, "f02 " }, // frame  2: off Z
  { 500, LED_I,   'c',  0x00,0x00,0x00, "f03 " }, // frame  3: off I
  { 500, LED_R,   'c',  0x00,0x00,0x00, "f04 " }, // frame  4: off R
  { 500, LED_E,   'c',  0x00,0x00,0x00, "f05 " }, // frame  5: off E
  { 500, LED_T,   'c',  0x00,0x00,0x00, "f06 " }, // frame  6: off T
  { 500, LED_S,   'c',  0x00,0x00,0x00, "f07 " }, // frame  7: off S
  { 500, LED_A,   'c',  0x00,0x00,0x00, "f08a" }, // frame  8: off A, on E
  {   0, LED_E,   'c',  0xff,0xff,0xff, "f08b" },

  { 500, LED_E,   'c',  0x00,0x00,0x00, "f09a" }, // frame  9: off E, on K
  {   0, LED_K,   'c',  0xff,0xff,0xff, "f09b" },

  { 500, LED_K,   'c',  0x00,0x00,0x00, "f10a" }, // frame 10: off K, on R
  {   0, LED_R,   'c',  0xff,0xff,0xff, "f10b" },
  { 500, LED_R,   'c',  0x00,0x00,0x00, "f11a" }, // frame 11: off R, on Z
  {   0, LED_Z,   'c',  0xff,0xff,0xff, "f11b" },
  { 500, LED_Z,   'c',  0x00,0x00,0x00, "f12a" }, // frame 12: off Z, on T
  {   0, LED_T,   'c',  0xff,0xff,0xff, "f12b" },
  { 500, LED_ALL, 'c',  0x00,0x00,0x00, "f13 " }, // frame 13 off T (all)
  { 500, LED_I,   'c',  0xff,0xff,0xff, "f14 " }, // frame 14:  on I
  { 500, LED_I,   'c',  0x00,0x00,0x00, "f15 " }, // frame 15: off I
  { 500, LED_I,   'c',  0xff,0xff,0xff, "f16 " }, // frame 16:  on I
  { 500, LED_I,   'c',  0x00,0x00,0x00, "f17 " }, // frame 17: off I

  { 500, LED_I,   'c',  0xff,0xff,0xff, "f18 " }, // frame 18:  on I

  { 500, LED_R,   'c',  0xff,0xff,0xff, "f19a" }, // frame 19: on R,Z,K
  {   0, LED_Z,   'c',  0xff,0xff,0xff, "f19b" }, // 
  {   0, LED_K,   'c',  0xff,0xff,0xff, "f19c" }, // 

  { 500, LED_R,   'c',  0x00,0x00,0x00, "f20a" }, // frame 20: off R,Z,K
  {   0, LED_Z,   'c',  0x00,0x00,0x00, "f20b" }, // 
  {   0, LED_K,   'c',  0x00,0x00,0x00, "f20c" }, // 

  { 500, LED_R,   'c',  0xff,0xff,0xff, "f21a" }, // frame 21: on R,Z,K
  {   0, LED_Z,   'c',  0xff,0xff,0xff, "f21b" }, // 
  {   0, LED_K,   'c',  0xff,0xff,0xff, "f21c" }, // 

  { 500, LED_R,   'c',  0x00,0x00,0x00, "f22a" }, // frame 22: off R,Z,K
  {   0, LED_Z,   'c',  0x00,0x00,0x00, "f22c" }, // 
  {   0, LED_K,   'c',  0x00,0x00,0x00, "f22d" }, // 

  { 500, LED_I,   'c',  0xff,0xff,0xff, "f23 " }, // frame 23: on I

  { 500, LED_R,   'c',  0xff,0xff,0xff, "f24a" }, // frame 24: on R,Z,K
  {   0, LED_Z,   'c',  0xff,0xff,0xff, "f24b" }, // 
  {   0, LED_K,   'c',  0xff,0xff,0xff, "f24c" }, // 

  { 500, LED_ALL, 'c',  0x00,0x00,0x00, "f25 " }, // frame 25: off all

  { 500, LED_ALL, 'c',  0x00,0x00,0x00, "f26 " }, // frame 26: off all
};  


const int cmdlines_len = sizeof(cmdlines)/sizeof(cmdline);

/*
const int cmdline_len = 11;
const cmdline cmdlines[] PROGMEM = { 
  {  10, 0, 'n',  0xff,0xff,0xff},
  {  10, 0, 'n',  0x00,0x00,0x00},
  {  10, 0, 'n',  0xff,0xff,0xff},
  {  10, 0, 'n',  0x00,0x00,0x00},
  {  10, 0, 'n',  0xff,0xff,0xff},
  { 100, 0, 'n',  0x00,0x00,0x00},
  { 100, 0, 'n',  0xff,0xff,0xff},
  { 100, 0, 'n',  0x00,0x00,0x00},
  { 100, 0, 'n',  0xff,0xff,0xff},
  { 100, 0, 'n',  0x00,0x00,0x00},
  { 100, 0, 'n',  0xff,0xff,0xff},
};
*/
