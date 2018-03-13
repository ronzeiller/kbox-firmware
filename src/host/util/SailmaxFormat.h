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
#ifndef _SailmaxFormat_h_
#define _SailmaxFormat_h_

#include <N2kMsg.h>

/**
 *  Converts a tN2kMsg into a proprietary Sailmax-Sentence used for logging and
 *  internal communication between external modules and the Sailmax-CU
 *
 *  If the buffer is not long enough, this function returns 0 and does not do
 *  anything.
 *
 *  If the buffer is long enough, this function returns the number of bytes
 *  written including the terminating \0 (but this function does not add the
 *  Return/LineFeed \r\n).
 */
size_t N2kToSailmax(const tN2kMsg &msg, uint32_t timestamp, char *buffer, size_t size);

/**
  * TODO: überlegen, wie da die Funktion ausschauen soll und was da genau gebraucht wird
  * Einstweilen ist hier eine Funktion vorbereitet, die aus dem Sailmax Format einen
  * N2k Satz machen soll.
  *
  * eigene Funktionen für einzelne Werte folgen
 */
 // TODO: Logging Rückumwandlungsfunktion schreiben
bool SailmaxToN2k(const char *buffer, uint32_t &timestamp, tN2kMsg &msg);

#endif
