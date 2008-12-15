#ifndef KEYS_H
#define KEYS_H

#if 0
/* If the "special" keyboard is available use this strange set of keys. */
#define KEY_CLEAR case 'c': case 'C':
#define KEY_SPIN  case 'j': case 'J':
#define KEY_STOP  case ' ':
#define KEY_STOP  case '?':
#define KEY_PAUSE case 'h': case 'H':
#define KEY_PRINT case '1': case '!':
#define KEY_PHOTO case 'g': case 'G':
#else

/* More sensible keys */
#define KEY_CLEAR case 'x': case 'X':
#define KEY_SPIN  case ' ':
#define KEY_START case 'r': case 'R':
#define KEY_STOP  case 's': case 'S':
#define KEY_PAUSE case 'p': case 'P':
//#define KEY_PRINT case 'w': case 'W':
//#define KEY_PHOTO case 'l': case 'L':
#endif

#endif

