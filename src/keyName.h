#pragma once
#include <Arduino.h>

const char buttonName[] = {
    "NONE\n"
    "A\n"
    "B\n"
    "C\n"
    "D\n"
    "E\n"
    "F\n"
    "G\n"
    "H\n"
    "I\n"
    "J\n"
    "K\n"
    "L\n"
    "M\n"
    "N\n"
    "O\n"
    "P\n"
    "Q\n"
    "R\n"
    "S\n"
    "T\n"
    "U\n"
    "V\n"
    "W\n"
    "X\n"
    "Y\n"
    "Z\n"
    "1\n"
    "2\n"
    "3\n"
    "4\n"
    "5\n"
    "6\n"
    "7\n"
    "8\n"
    "9\n"
    "0\n"
    "ENTER\n"
    "ESCAPE\n"
    "BACKSPACE\n"
    "TAB\n"
    "SPACE\n"
    "MINUS\n"
    "EQUAL\n"
    "BRACKET_LEFT\n"
    "BRACKET_RIGHT\n"
    "BACKSLASH\n"
    "SEMICOLON\n"
    "APOSTROPHE\n"
    "GRAVE\n"
    "COMMA\n"
    "PERIOD\n"
    "SLASH\n"
    "CAPS_LOCK\n"
    "F1\n"
    "F2\n"
    "F3\n"
    "F4\n"
    "F5\n"
    "F6\n"
    "F7\n"
    "F8\n"
    "F9\n"
    "F10\n"
    "F11\n"
    "F12\n"
    "PRINT_SCREEN\n"
    "SCROLL_LOCK\n"
    "PAUSE\n"
    "INSERT\n"
    "HOME\n"
    "PAGE_UP\n"
    "DELETE\n"
    "END\n"
    "PAGE_DOWN\n"
    "ARROW_RIGHT\n"
    "ARROW_LEFT\n"
    "ARROW_DOWN\n"
    "ARROW_UP\n"
    "NUM_LOCK\n"
    "KEYPAD_DIVIDE\n"
    "KEYPAD_MULTIPLY\n"
    "KEYPAD_SUBTRACT\n"
    "KEYPAD_ADD\n"
    "KEYPAD_ENTER\n"
    "KEYPAD_1\n"
    "KEYPAD_2\n"
    "KEYPAD_3\n"
    "KEYPAD_4\n"
    "KEYPAD_5\n"
    "KEYPAD_6\n"
    "KEYPAD_7\n"
    "KEYPAD_8\n"
    "KEYPAD_9\n"
    "KEYPAD_0\n"
    "KEYPAD_DECIMAL\n"
    "EUROPE_2\n"
    "KEYPAD_EQUAL\n"
    "MENU\n"
    "CUT\n"
    "COPY\n"
    "PASTE\n"
    "FIND\n"
    "MUTE\n"
    "VOLUME_UP\n"
    "VOLUME_DOWN\n"
    "LOCKING_CAPS_LOCK\n"
    "LOCKING_NUM_LOCK\n"
    "LOCKING_SCROLL_LOCK\n"
    "KEYPAD_COMMA\n"
    "KEYPAD_EQUAL_SIGN"
};

const char* buttonString[] = {
    "NONE",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "ENTER",
    "ESCAPE",
    "BACKSPACE",
    "TAB",
    "SPACE",
    "MINUS",
    "EQUAL",
    "BRACKET_LEFT",
    "BRACKET_RIGHT",
    "BACKSLASH",
    "SEMICOLON",
    "APOSTROPHE",
    "GRAVE",
    "COMMA",
    "PERIOD",
    "SLASH",
    "CAPS_LOCK",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "PRINT_SCREEN",
    "SCROLL_LOCK",
    "PAUSE",
    "INSERT",
    "HOME",
    "PAGE_UP",
    "DELETE",
    "END",
    "PAGE_DOWN",
    "ARROW_RIGHT",
    "ARROW_LEFT",
    "ARROW_DOWN",
    "ARROW_UP",
    "NUM_LOCK",
    "KEYPAD_DIVIDE",
    "KEYPAD_MULTIPLY",
    "KEYPAD_SUBTRACT",
    "KEYPAD_ADD",
    "KEYPAD_ENTER",
    "KEYPAD_1",
    "KEYPAD_2",
    "KEYPAD_3",
    "KEYPAD_4",
    "KEYPAD_5",
    "KEYPAD_6",
    "KEYPAD_7",
    "KEYPAD_8",
    "KEYPAD_9",
    "KEYPAD_0",
    "KEYPAD_DECIMAL",
    "EUROPE_2",
    "KEYPAD_EQUAL",
    "MENU",
    "CUT",
    "COPY",
    "PASTE",
    "FIND",
    "MUTE",
    "VOLUME_UP",
    "VOLUME_DOWN",
    "LOCKING_CAPS_LOCK",
    "LOCKING_NUM_LOCK",
    "LOCKING_SCROLL_LOCK",
    "KEYPAD_COMMA",
    "KEYPAD_EQUAL_SIGN"
};

const uint8_t buttonCode[] = {
    0x00, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
    0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42,
    0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
    0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52,
    0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
    0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62,
    0x63, 0x64, 0x67, 0x76, 0x7B, 0x7C, 0x7D, 0x7E,
    0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86
};

String codeToName(uint8_t code) {
  for (int i = 0; i < sizeof(buttonCode); i++) {
    if (buttonCode[i] == code) return String(buttonString[i]);
  }
  return "UNKNOWN";
};

uint8_t nameToCode(String name) {
  for (int i = 0; i < sizeof(buttonString)/sizeof(buttonString[0]); i++) {
    if (name == String(buttonString[i])) return buttonCode[i];
  }
  return 0x00;
};

// the button name is in the loooooooooooooooooooong way
// long way
// the button code is in the loooooooooooooooooooong way
// long way
// idk please don't tell another about this