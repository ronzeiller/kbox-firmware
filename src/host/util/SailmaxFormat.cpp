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
#include <Seasmart.h>


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

/*
 * Attemts to read n bytes in hexadecimal from input string to value.
 * Returns true if successful, false otherwise.
 */
static bool readNHexByte(const char *s, unsigned int n, uint32_t &value) {
  if (strlen(s) < 2*n) {
    return -1;
  }
  for (unsigned int i = 0; i < 2*n; i++) {
    if (!isxdigit(s[i])) {
      return -1;
    }
  }

  char sNumber[2*n + 1];
  strncpy(sNumber, s, sizeof(sNumber));
  sNumber[sizeof(sNumber) - 1] = 0;

  value = strtol(sNumber, 0, 16);
  return true;
}

size_t N2kToSailmax(const tN2kMsg &msg, uint32_t timestamp, char *buffer, size_t size) {
  //size_t pcdin_sentence_length = 6+1+6+1+8+1+2+1+msg.DataLen*2+1+2 + 1;
  uint32_t number = timestamp;
  int digits = 0; do { number /= 10; digits++; } while (number != 0);
  // @millis,PGN,Source,Data*checksum
  // @22643312,128267,23,DB28010000A0F6FF*28
  size_t pcdin_sentence_length = 1+digits+1+6+1+2+1+msg.DataLen*2+1+2 + 1;
  if (size < pcdin_sentence_length) {
    return 0;
  }

  char *s = buffer;
  sprintf(s, "@%lu,",timestamp );

  s += digits + 2;
  sprintf(s,"%06lu,",msg.PGN );
  // DebugSerial.printf(s );
  s += 7;

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

// read format: @millis,PGN,Source,Data*checksum
// like: @22643312,128267,23,DB28010000A0F6FF*28
bool SailmaxToN2k(const char *buffer, uint32_t &timestamp, tN2kMsg &msg) {

  msg.Clear();
  msg.Destination = 0xFF;

  const char *s = buffer;
  int i = 0;

  // check if line starts with @
  if (*s == '@') {
    i = 1;

    timestamp = 0;
    while (s[i] != ',') {
      timestamp = timestamp * 10 + ((int)s[i] - 48);
      i++;
    }
    msg.MsgTime = timestamp;

    i ++; // skip the comma
    // the next 6 chars are the PGN (N2k page number)
    msg.PGN = 0;
    while (s[i] != ',') {
      msg.PGN = msg.PGN * 10 + ((int)s[i] - 48);
      i++;
    }
    i ++; // skip the comma
    s += i;

    // next comes Source
    uint32_t source;
    if (!readNHexByte(s, 1, source)) {
      return false;
    }
    msg.Source = source;

    s += 3;
    int dataLen = 0;
    while (s[dataLen] != 0 && s[dataLen] != '*') {
      dataLen++;
      //i++;
    }
    if (dataLen % 2 != 0) {
      return false;
    }
    dataLen /= 2;

    msg.DataLen = dataLen;
    if (msg.DataLen > msg.MaxDataLen) {
      return false;
    }
    //Serial.printf("DataLen: %d\n", msg.DataLen);

    for (int j = 0; j < dataLen; j++) {
      uint32_t byte;
      if (!readNHexByte(s, 1, byte)) {
        return false;
      }
      msg.Data[j] = byte;
      s += 2;
    }

    // Skip the terminating '*' which marks beginning of checksum
    s += 1;
    uint32_t checksum;
    //Serial.printf("checksum s: %s\n", s);
    if (!readNHexByte(s, 1, checksum)) {
      Serial.printf("readNHexByte nicht ok \n");
      return false;
    }

    if (checksum != nmea_compute_checksum(buffer)) {
      Serial.printf("nmea_compute_checksum nicht ok \n");
      return false;
    }

    return true;
  } else {
    return false;
  }
}
