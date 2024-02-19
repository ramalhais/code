// NeXT keycodes:
// https://github.com/tmk/tmk_keyboard/blob/master/converter/next_usb/unimap_trans.h
// https://web.archive.org/web/20150608141822/http://www.68k.org/~degs/nextkeyboard.html

#include <class/hid/hid.h>

#ifndef _NEXT_KEYCODES_H_
#define _NEXT_KEYCODES_H_

uint16_t ascii2next[256];
uint16_t hid2next[256];

// Modifiers
#define KD_CNTL 0x01
#define KD_LSHIFT 0x02
#define KD_RSHIFT 0x04
#define KD_LCOMM 0x08
#define KD_RCOMM 0x10
#define KD_LALT 0x20
#define KD_RALT 0x40

uint8_t hid2next_modifiers(uint8_t hid_modifiers, uint8_t hid_rcomm) {
  uint8_t next_modifiers = 0;
  uint8_t rcomm = KEYBOARD_MODIFIER_RIGHTGUI;
  uint8_t cntl = KEYBOARD_MODIFIER_LEFTCTRL;
  if (hid_rcomm) {
    rcomm = hid_rcomm;
    if (rcomm != KEYBOARD_MODIFIER_RIGHTCTRL) {
      cntl |= KEYBOARD_MODIFIER_RIGHTCTRL;
    }
  }
  next_modifiers =
    (bool)(hid_modifiers&cntl)        |
    (bool)(hid_modifiers&KEYBOARD_MODIFIER_LEFTSHIFT)   <<1 |
    (bool)(hid_modifiers&KEYBOARD_MODIFIER_RIGHTSHIFT)  <<2 |
    (bool)(hid_modifiers&KEYBOARD_MODIFIER_LEFTGUI)     <<3 |
    (bool)(hid_modifiers&rcomm)                         <<4 |
    (bool)(hid_modifiers&KEYBOARD_MODIFIER_LEFTALT)     <<5 |
    (bool)(hid_modifiers&KEYBOARD_MODIFIER_RIGHTALT)    <<6 ;

  return next_modifiers;
}

// Keys
#define NEXT_KEY_NO_KEY 0
#define NEXT_KEY_BACKSPACE 0x1B
#define NEXT_KEY_HORIZONTAL_TAB 0x41
#define NEXT_KEY_ENTER 0x2A
#define NEXT_KEY_ESCAPE 0x49
#define NEXT_KEY_SPACE 0x38
#define NEXT_KEY_EXCLAMATION 0x4A|KD_LSHIFT<<8
#define NEXT_KEY_DOUBLE_QUOTES 0x2B|KD_LSHIFT<<8
#define NEXT_KEY_NUMBER_SIGN 0x4C|KD_LSHIFT<<8// hash
#define NEXT_KEY_DOLLAR 0x4D|KD_LSHIFT<<8
#define NEXT_KEY_PERCENT 0x50|KD_LSHIFT<<8
#define NEXT_KEY_AMPERSAND 0x4E|KD_LSHIFT<<8
#define NEXT_KEY_SINGLE_QUOTE 0x2B
#define NEXT_KEY_LEFT_PARENTHESIS 0x1F|KD_LSHIFT<<8
#define NEXT_KEY_RIGHT_PARENTHESIS 0x20|KD_LSHIFT<<8
#define NEXT_KEY_ASTERISK 0x1E|KD_LSHIFT<<8
#define NEXT_KEY_PLUS 0x1C|KD_LSHIFT<<8
#define NEXT_KEY_COMMA 0x2E|KD_LSHIFT<<8
#define NEXT_KEY_MINUS 0x1D
#define NEXT_KEY_PERIOD 0x2F
#define NEXT_KEY_SLASH 0x30
#define NEXT_KEY_ZERO 0x20
#define NEXT_KEY_ONE 0x4A
#define NEXT_KEY_TWO 0x4B
#define NEXT_KEY_THREE 0x4C
#define NEXT_KEY_FOUR 0x4D
#define NEXT_KEY_FIVE 0x50
#define NEXT_KEY_SIX 0x4F
#define NEXT_KEY_SEVEN 0x4E
#define NEXT_KEY_EIGHT 0x1E
#define NEXT_KEY_NINE 0x1F
#define NEXT_KEY_COLON 0x2C|KD_LSHIFT<<8
#define NEXT_KEY_SEMICOLON 0x2C
#define NEXT_KEY_LESS_THAN 0x2E|KD_LSHIFT<<8
#define NEXT_KEY_EQUALS 0x1C
#define NEXT_KEY_GREATER_THAN 0x2F|KD_LSHIFT<<8
#define NEXT_KEY_QUESTION_MARK 0x30|KD_LSHIFT<<8
#define NEXT_KEY_AT_SIGN 0x4B|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_A 0x39|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_B 0x35|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_C 0x33|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_D 0x3B|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_E 0x44|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_F 0x3C|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_G 0x3D|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_H 0x40|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_I 0x06|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_J 0x3F|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_K 0x3E|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_L 0x2D|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_M 0x36|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_N 0x37|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_O 0x07|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_P 0x08|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Q 0x42|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_R 0x45|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_S 0x3A|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_T 0x48|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_U 0x46|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_V 0x34|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_W 0x43|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_X 0x32|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Y 0x47|KD_LSHIFT<<8
#define NEXT_KEY_UPPERCASE_Z 0x31|KD_LSHIFT<<8
#define NEXT_KEY_OPENING_BRACKET 0x05
#define NEXT_KEY_BACKSLASH 0x28|KD_LSHIFT<<8
#define NEXT_KEY_CLOSING_BRACKET 0x04
#define NEXT_KEY_CARET_CIRCUMFLEX 0x4F|KD_LSHIFT<<8
#define NEXT_KEY_UNDERSCORE 0x1D|KD_LSHIFT<<8
#define NEXT_KEY_GRAVE_ACCENT 0x26
#define NEXT_KEY_LOWERCASE_A 0x39
#define NEXT_KEY_LOWERCASE_B 0x35
#define NEXT_KEY_LOWERCASE_C 0x33
#define NEXT_KEY_LOWERCASE_D 0x3B
#define NEXT_KEY_LOWERCASE_E 0x44
#define NEXT_KEY_LOWERCASE_F 0x3C
#define NEXT_KEY_LOWERCASE_G 0x3D
#define NEXT_KEY_LOWERCASE_H 0x40
#define NEXT_KEY_LOWERCASE_I 0x06
#define NEXT_KEY_LOWERCASE_J 0x3F
#define NEXT_KEY_LOWERCASE_K 0x3E
#define NEXT_KEY_LOWERCASE_L 0x2D
#define NEXT_KEY_LOWERCASE_M 0x36
#define NEXT_KEY_LOWERCASE_N 0x37
#define NEXT_KEY_LOWERCASE_O 0x07
#define NEXT_KEY_LOWERCASE_P 0x08
#define NEXT_KEY_LOWERCASE_Q 0x42
#define NEXT_KEY_LOWERCASE_R 0x45
#define NEXT_KEY_LOWERCASE_S 0x3A
#define NEXT_KEY_LOWERCASE_T 0x48
#define NEXT_KEY_LOWERCASE_U 0x46
#define NEXT_KEY_LOWERCASE_V 0x34
#define NEXT_KEY_LOWERCASE_W 0x43
#define NEXT_KEY_LOWERCASE_X 0x32
#define NEXT_KEY_LOWERCASE_Y 0x47
#define NEXT_KEY_LOWERCASE_Z 0x31
#define NEXT_KEY_OPENING_BRACE 0x05|KD_LSHIFT<<8
#define NEXT_KEY_VERTICAL_BAR 0x27|KD_LSHIFT<<8
#define NEXT_KEY_CLOSING_BRACE 0x04|KD_LSHIFT<<8
#define NEXT_KEY_TILDE 0x26|KD_LSHIFT<<8
#define NEXT_KEY_DELETE 0x1B|KD_LSHIFT<<8 // FIXME: Made it up
#define NEXT_KEY_ARROW_RIGHT 0x10
#define NEXT_KEY_ARROW_LEFT 0x09
#define NEXT_KEY_ARROW_DOWN 0x0F
#define NEXT_KEY_ARROW_UP 0x16


void ascii2next_init() {
  ascii2next[8] = NEXT_KEY_BACKSPACE;
  ascii2next[9] = NEXT_KEY_HORIZONTAL_TAB;
  ascii2next[10] = NEXT_KEY_ENTER;
  ascii2next[27] = NEXT_KEY_ESCAPE;
  ascii2next[' '] = NEXT_KEY_SPACE;
  ascii2next['!'] = NEXT_KEY_EXCLAMATION;
  ascii2next['"'] = NEXT_KEY_DOUBLE_QUOTES;
  ascii2next['#'] = NEXT_KEY_NUMBER_SIGN;
  ascii2next['$'] = NEXT_KEY_DOLLAR;
  ascii2next['%'] = NEXT_KEY_PERCENT;
  ascii2next['&'] = NEXT_KEY_AMPERSAND;
  ascii2next['\''] = NEXT_KEY_SINGLE_QUOTE;
  ascii2next['('] = NEXT_KEY_LEFT_PARENTHESIS;
  ascii2next[')'] = NEXT_KEY_RIGHT_PARENTHESIS;
  ascii2next['*'] = NEXT_KEY_ASTERISK;
  ascii2next['+'] = NEXT_KEY_PLUS;
  ascii2next[','] = NEXT_KEY_COMMA;
  ascii2next['-'] = NEXT_KEY_MINUS;
  ascii2next['.'] = NEXT_KEY_PERIOD;
  ascii2next['/'] = NEXT_KEY_SLASH;
  ascii2next['0'] = NEXT_KEY_ZERO;
  ascii2next['1'] = NEXT_KEY_ONE;
  ascii2next['2'] = NEXT_KEY_TWO;
  ascii2next['3'] = NEXT_KEY_THREE;
  ascii2next['4'] = NEXT_KEY_FOUR;
  ascii2next['5'] = NEXT_KEY_FIVE;
  ascii2next['6'] = NEXT_KEY_SIX;
  ascii2next['7'] = NEXT_KEY_SEVEN;
  ascii2next['8'] = NEXT_KEY_EIGHT;
  ascii2next['9'] = NEXT_KEY_NINE;
  ascii2next[':'] = NEXT_KEY_COLON;
  ascii2next[';'] = NEXT_KEY_SEMICOLON;
  ascii2next['<'] = NEXT_KEY_LESS_THAN;
  ascii2next['='] = NEXT_KEY_EQUALS;
  ascii2next['>'] = NEXT_KEY_GREATER_THAN;
  ascii2next['?'] = NEXT_KEY_QUESTION_MARK;
  ascii2next['@'] = NEXT_KEY_AT_SIGN;
  ascii2next['A'] = NEXT_KEY_UPPERCASE_A;
  ascii2next['B'] = NEXT_KEY_UPPERCASE_B;
  ascii2next['C'] = NEXT_KEY_UPPERCASE_C;
  ascii2next['D'] = NEXT_KEY_UPPERCASE_D;
  ascii2next['E'] = NEXT_KEY_UPPERCASE_E;
  ascii2next['F'] = NEXT_KEY_UPPERCASE_F;
  ascii2next['G'] = NEXT_KEY_UPPERCASE_G;
  ascii2next['H'] = NEXT_KEY_UPPERCASE_H;
  ascii2next['I'] = NEXT_KEY_UPPERCASE_I;
  ascii2next['J'] = NEXT_KEY_UPPERCASE_J;
  ascii2next['K'] = NEXT_KEY_UPPERCASE_K;
  ascii2next['L'] = NEXT_KEY_UPPERCASE_L;
  ascii2next['M'] = NEXT_KEY_UPPERCASE_M;
  ascii2next['N'] = NEXT_KEY_UPPERCASE_N;
  ascii2next['O'] = NEXT_KEY_UPPERCASE_O;
  ascii2next['P'] = NEXT_KEY_UPPERCASE_P;
  ascii2next['Q'] = NEXT_KEY_UPPERCASE_Q;
  ascii2next['R'] = NEXT_KEY_UPPERCASE_R;
  ascii2next['S'] = NEXT_KEY_UPPERCASE_S;
  ascii2next['T'] = NEXT_KEY_UPPERCASE_T;
  ascii2next['U'] = NEXT_KEY_UPPERCASE_U;
  ascii2next['V'] = NEXT_KEY_UPPERCASE_V;
  ascii2next['W'] = NEXT_KEY_UPPERCASE_W;
  ascii2next['X'] = NEXT_KEY_UPPERCASE_X;
  ascii2next['Y'] = NEXT_KEY_UPPERCASE_Y;
  ascii2next['Z'] = NEXT_KEY_UPPERCASE_Z;
  ascii2next['['] = NEXT_KEY_OPENING_BRACKET;
  ascii2next['\\'] = NEXT_KEY_BACKSLASH;
  ascii2next[']'] = NEXT_KEY_CLOSING_BRACKET;
  ascii2next['^'] = NEXT_KEY_CARET_CIRCUMFLEX;
  ascii2next['_'] = NEXT_KEY_UNDERSCORE;
  ascii2next['`'] = NEXT_KEY_GRAVE_ACCENT;
  ascii2next['a'] = NEXT_KEY_LOWERCASE_A;
  ascii2next['b'] = NEXT_KEY_LOWERCASE_B;
  ascii2next['c'] = NEXT_KEY_LOWERCASE_C;
  ascii2next['d'] = NEXT_KEY_LOWERCASE_D;
  ascii2next['e'] = NEXT_KEY_LOWERCASE_E;
  ascii2next['f'] = NEXT_KEY_LOWERCASE_F;
  ascii2next['g'] = NEXT_KEY_LOWERCASE_G;
  ascii2next['h'] = NEXT_KEY_LOWERCASE_H;
  ascii2next['i'] = NEXT_KEY_LOWERCASE_I;
  ascii2next['j'] = NEXT_KEY_LOWERCASE_J;
  ascii2next['k'] = NEXT_KEY_LOWERCASE_K;
  ascii2next['l'] = NEXT_KEY_LOWERCASE_L;
  ascii2next['m'] = NEXT_KEY_LOWERCASE_M;
  ascii2next['n'] = NEXT_KEY_LOWERCASE_N;
  ascii2next['o'] = NEXT_KEY_LOWERCASE_O;
  ascii2next['p'] = NEXT_KEY_LOWERCASE_P;
  ascii2next['q'] = NEXT_KEY_LOWERCASE_Q;
  ascii2next['r'] = NEXT_KEY_LOWERCASE_R;
  ascii2next['s'] = NEXT_KEY_LOWERCASE_S;
  ascii2next['t'] = NEXT_KEY_LOWERCASE_T;
  ascii2next['u'] = NEXT_KEY_LOWERCASE_U;
  ascii2next['v'] = NEXT_KEY_LOWERCASE_V;
  ascii2next['w'] = NEXT_KEY_LOWERCASE_W;
  ascii2next['x'] = NEXT_KEY_LOWERCASE_X;
  ascii2next['y'] = NEXT_KEY_LOWERCASE_Y;
  ascii2next['z'] = NEXT_KEY_LOWERCASE_Z;
  ascii2next['{'] = NEXT_KEY_OPENING_BRACE;
  ascii2next['|'] = NEXT_KEY_VERTICAL_BAR;
  ascii2next['}'] = NEXT_KEY_CLOSING_BRACE;
  ascii2next['~'] = NEXT_KEY_TILDE;
  ascii2next[0x7F] = NEXT_KEY_DELETE;
}

void hid2next_init() {
  hid2next[HID_KEY_BACKSPACE] = NEXT_KEY_BACKSPACE;
  hid2next[HID_KEY_TAB] = NEXT_KEY_HORIZONTAL_TAB;
  hid2next[HID_KEY_ENTER] = NEXT_KEY_ENTER;
  hid2next[HID_KEY_ESCAPE] = NEXT_KEY_ESCAPE;
  hid2next[HID_KEY_SPACE] = NEXT_KEY_SPACE;
  hid2next[HID_KEY_APOSTROPHE] = NEXT_KEY_SINGLE_QUOTE;
  hid2next[HID_KEY_COMMA] = NEXT_KEY_COMMA;
  hid2next[HID_KEY_MINUS] = NEXT_KEY_MINUS;
  hid2next[HID_KEY_PERIOD] = NEXT_KEY_PERIOD;
  hid2next[HID_KEY_SLASH] = NEXT_KEY_SLASH;
  hid2next[HID_KEY_0] = NEXT_KEY_ZERO;
  hid2next[HID_KEY_1] = NEXT_KEY_ONE;
  hid2next[HID_KEY_2] = NEXT_KEY_TWO;
  hid2next[HID_KEY_3] = NEXT_KEY_THREE;
  hid2next[HID_KEY_4] = NEXT_KEY_FOUR;
  hid2next[HID_KEY_5] = NEXT_KEY_FIVE;
  hid2next[HID_KEY_6] = NEXT_KEY_SIX;
  hid2next[HID_KEY_7] = NEXT_KEY_SEVEN;
  hid2next[HID_KEY_8] = NEXT_KEY_EIGHT;
  hid2next[HID_KEY_9] = NEXT_KEY_NINE;
  hid2next[HID_KEY_SEMICOLON] = NEXT_KEY_SEMICOLON;
  hid2next[HID_KEY_EQUAL] = NEXT_KEY_EQUALS;
  hid2next[HID_KEY_BRACKET_LEFT] = NEXT_KEY_OPENING_BRACKET;
  hid2next[HID_KEY_BACKSLASH] = NEXT_KEY_BACKSLASH;
  hid2next[HID_KEY_BRACKET_RIGHT] = NEXT_KEY_CLOSING_BRACKET;
  hid2next[HID_KEY_GRAVE] = NEXT_KEY_GRAVE_ACCENT;
  hid2next[HID_KEY_A] = NEXT_KEY_LOWERCASE_A;
  hid2next[HID_KEY_B] = NEXT_KEY_LOWERCASE_B;
  hid2next[HID_KEY_C] = NEXT_KEY_LOWERCASE_C;
  hid2next[HID_KEY_D] = NEXT_KEY_LOWERCASE_D;
  hid2next[HID_KEY_E] = NEXT_KEY_LOWERCASE_E;
  hid2next[HID_KEY_F] = NEXT_KEY_LOWERCASE_F;
  hid2next[HID_KEY_G] = NEXT_KEY_LOWERCASE_G;
  hid2next[HID_KEY_H] = NEXT_KEY_LOWERCASE_H;
  hid2next[HID_KEY_I] = NEXT_KEY_LOWERCASE_I;
  hid2next[HID_KEY_J] = NEXT_KEY_LOWERCASE_J;
  hid2next[HID_KEY_K] = NEXT_KEY_LOWERCASE_K;
  hid2next[HID_KEY_L] = NEXT_KEY_LOWERCASE_L;
  hid2next[HID_KEY_M] = NEXT_KEY_LOWERCASE_M;
  hid2next[HID_KEY_N] = NEXT_KEY_LOWERCASE_N;
  hid2next[HID_KEY_O] = NEXT_KEY_LOWERCASE_O;
  hid2next[HID_KEY_P] = NEXT_KEY_LOWERCASE_P;
  hid2next[HID_KEY_Q] = NEXT_KEY_LOWERCASE_Q;
  hid2next[HID_KEY_R] = NEXT_KEY_LOWERCASE_R;
  hid2next[HID_KEY_S] = NEXT_KEY_LOWERCASE_S;
  hid2next[HID_KEY_T] = NEXT_KEY_LOWERCASE_T;
  hid2next[HID_KEY_U] = NEXT_KEY_LOWERCASE_U;
  hid2next[HID_KEY_V] = NEXT_KEY_LOWERCASE_V;
  hid2next[HID_KEY_W] = NEXT_KEY_LOWERCASE_W;
  hid2next[HID_KEY_X] = NEXT_KEY_LOWERCASE_X;
  hid2next[HID_KEY_Y] = NEXT_KEY_LOWERCASE_Y;
  hid2next[HID_KEY_Z] = NEXT_KEY_LOWERCASE_Z;
  hid2next[HID_KEY_DELETE] = NEXT_KEY_DELETE;
  hid2next[HID_KEY_ARROW_RIGHT] = NEXT_KEY_ARROW_RIGHT;
  hid2next[HID_KEY_ARROW_LEFT] = NEXT_KEY_ARROW_LEFT;
  hid2next[HID_KEY_ARROW_DOWN] = NEXT_KEY_ARROW_DOWN;
  hid2next[HID_KEY_ARROW_UP] = NEXT_KEY_ARROW_UP;
}

void next_keycodes_init() {
  ascii2next_init();
  hid2next_init();
}
#endif