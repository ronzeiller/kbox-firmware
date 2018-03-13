/*

       SSS       A     I L      M    M      A       X   X
      S         A A    I L      MM  MM     A A       X X
        S      A   A   I L      M MM M    A   A       X
          S   AAAAAAA  I L      M    M   AAAAAAA     X X
      SSS    A       A I LLLLL  M    M  A       A   X   X

      * Project:  Sailmax-CU
      * Purpose:  propriatary format for writing logging datas
      * Author:   © Ronnie Zeiller, 2018

*/

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "SailmaxFormat.h"


/* Some private helper functions to generate hex-serialized NMEA messages */
static const char *hex = "0123456789ABCDEF";

/*============================================================================*/
/* Convert unsigned long value to d-digit decimal string in local buffer      */
/*============================================================================*/
char *u2s(unsigned long x,unsigned d){
  static char b[16];
  char *p;
  unsigned digits = 0;
  unsigned long t = x;

  do ++digits; while (t /= 10);
  // if (digits > d) d = digits; // uncomment to allow more digits than spec'd
  *(p = b + d) = '\0';
  do *--p = x % 10 + '0'; while (x /= 10);
  while (p != b) *--p = ' ';
  return b;
}

static int appendByte(char *s, uint8_t byte) {
  s[0] = hex[byte >> 4];
  s[1] = hex[byte & 0xf];
  return 2;
}

/*
static int append2Bytes(char *s, uint16_t i) {
  appendByte(s, i >> 8);
  appendByte(s + 2, i & 0xff);
  return 4;
}


static int appendWord(char *s, uint32_t i) {
  append2Bytes(s, i >> 16);
  append2Bytes(s + 4, i & 0xffff);
  return 8;
}
*/

static uint8_t nmea_compute_checksum(const char *sentence) {
  if (sentence == 0) {
    return 0;
  }

  // Skip the $ at the beginning
  int i = 1;

  int checksum = 0;
  while (sentence[i] != '*') {
    checksum ^= sentence[i];
    i++;
  }
  return checksum;
}

size_t N2kToSailmax(const tN2kMsg &msg, uint32_t timestamp, char *buffer, size_t size) {
  //size_t pcdin_sentence_length = 6+1+6+1+8+1+2+1+msg.DataLen*2+1+2 + 1;
  uint32_t number = timestamp;
  int digits = 0; do { number /= 10; digits++; } while (number != 0);
  size_t pcdin_sentence_length = 1+digits+1+6+1+2+1+msg.DataLen*2+1+2 + 1;
  if (size < pcdin_sentence_length) {
    return 0;
  }

  char *s = buffer;

  //ultoa(timestamp, s, 10 );

  // @millis,PGN,Source,Data,checksum
  //sprintf(s, "@%010lu,",timestamp );  // -> fixe 10 Stellen mit führenden Nullen!!!

  //snprintf(s, sizeof (s), "@%lu,", (unsigned long) timestamp); /* Method 1 */
  //snprintf(s, sizeof (s), "@%," PRIu32, timestamp); /* Method 2 */
  sprintf(s, "@%lu,",timestamp );

  s += digits + 2;   // zur nächsten Stelle vorrücken
  sprintf(s,"%06lu,",msg.PGN );
  // DebugSerial.printf(s );
  s += 7;

  /*
  strcpy(s, "@,");
  s += 7;
  s += appendByte(s, msg.PGN >> 16);
  s += append2Bytes(s, msg.PGN & 0xffff);
  *s++ = ',';
  s += 9;
  s += appendWord(s, timestamp);
  *s++ = ',';
  */

  s += appendByte(s, msg.Source);
  *s++ = ',';

  for (int i = 0; i < msg.DataLen; i++) {
    s += appendByte(s, msg.Data[i]);
  }

  *s++ = '*';
  s += appendByte(s, nmea_compute_checksum(buffer));
  *s = 0;
  return (size_t)(s - buffer);
}
