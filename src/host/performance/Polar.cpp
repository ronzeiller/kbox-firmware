/*
     __  __     ______     ______     __  __
    /\ \/ /    /\  == \   /\  __ \   /\_\_\_\
    \ \  _"-.  \ \  __<   \ \ \/\ \  \/_/\_\/_
     \ \_\ \_\  \ \_____\  \ \_____\   /\_\/\_\
       \/_/\/_/   \/_____/   \/_____/   \/_/\/_/

  The MIT License
  Copyright (c) 2018 Ronnie Zeiller ronnie@zeiller.eu
  Copyright (c) 2018 Thomas Sarlandie thomas@sarlandie.net

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

// Performance class calculates values and gives them back to keep the class
// independent from the calling function

#include <KBoxLogging.h>
#include <KBoxHardware.h>
#include "host/config/KBoxConfig.h"
#include "Polar.h"
#include "common/signalk/SKUnits.h"

Polar::Polar(PerformanceConfig &config) : _config(config) {
  if (KBox.isSdCardUsable()) {
    _cardReady = true;
  }
}

bool Polar::readPolarDataFile(){

  if (!_cardReady) {
    return false;
  }

  //static const char *polarDataFileName = "polarData.pol";
  String polarDataFileName = _config.polarDataFileName;

  DEBUG("Try to load polar datas from file %s on SD-Card", polarDataFileName.c_str());
  if (KBox.getSdFat().exists(polarDataFileName.c_str())) {
    //File polarDataFile = KBox.getSdFat().open(polarDataFileName);
    DEBUG("Polar data file found");
  } else {
    return false;
  }

  ifstream sdin(polarDataFileName.c_str());
  const int line_buffer_size = 120;
  char buffer[line_buffer_size];
  //int line_number = 0;
  //int index = 0;
  // TR
  //String wdirstr, wsp;
  //bool first = true;
  //int mode = -1, row = -1, sep = -1;
  //String WS, WSS;
  // \TR
  //static uint8_t WS[WINDSPEED];
  //static uint8_t WSS[WINDSPEED];

  while (sdin.getline(buffer, line_buffer_size, '\n') || sdin.gcount()) {
    int count = sdin.gcount();
    if (sdin.fail()) {
      DEBUG("Partial long line");
      sdin.clear(sdin.rdstate() & ~ios_base::failbit);
    } else if (sdin.eof()) {
      DEBUG("Partial final line");  // sdin.fail() is false
    } else {
      count--;  // Don’t include newline in count
      //DEBUG("Line %i", ++line_number);
    }
    DEBUG("%i chars %s",count, buffer);
/*
    // 1. run, check which type of polar datas
    if (first){
      // WS = wxStringTokenize(str, _T(";,\t "));

      char * pch;
      printf ("Splitting buffer \"%s\" into tokens:\n",buffer);
      pch = strtok (buffer,"\t\n");
      while (pch != NULL) {
        DEBUG("%s",pch);
        WS[index] = *pch;
        DEBUG("%s",WS[index]);
        index++;
        pch = strtok (NULL, " ,.-");
      }
*/
/*
      WS[0] = WS[0].toUpperCase();
      if (WS[0].Find("TWA\\TWS") != -1 || WS[0].Find("TWA/TWS") != -1 || WS[0].Find("TWA") != -1){
        mode = 1;
        sep = 1;
      }
      else if (WS[0].IsNumber()) {
        mode = 2;
        sep = 1;
        //x = wxAtoi(WS[0]);
        //col = (x + 1) / 2 - 1;
        col = atoi(WS[0]);

        for (i = 1; i < (int)WS.GetCount(); i += 2){
          //x = wxAtoi(WS[i]);
          //row = (x + 2) / 5 - 1;
          row = atoi(WS[i]);
          s = WS[i + 1];

          if (col > WINDSPEED - 1) break;
          if (s == "0") || s == ("0.00") || s == ("0.0") || s == ("0.000")){
            continue;
          }
          if (col < WINDSPEED + 1) {
            setValue(s, row, col);
          }
        }
      }
*/
//    } // end if first



} // end while get line

  // BEGIN TR
/*

          else if (!WS[0].IsNumber()){
            continue;
          }

          if (sep == -1){
            wxMessageBox(_("Format in this file not recognised"));
            return;
          }

          first = false;
          if (mode != 0)
            continue;
        }
        if (mode == 1) // Formats OCPN/QTVlm/MAXSea/CVS
        {
          WSS = wxStringTokenize(str, _T(";,\t "));
          if (WSS[0] == _T("0") && mode == 1)
          {
            row++; continue;
          }
          else if (row == -1)
            row++;
          row = wxAtoi(WSS[0]);
          for (i = 1; i < (int)WSS.GetCount(); i++)
          {
            s = WSS[i];
            if (col > WINDSPEED - 1) break;
            if (s == _T("0") || s == _T("0.00") || s == _T("0.0") || s == _T("0.000")){
              continue;
            }
            col = wxAtoi(WS[i]);
            setValue(s, row, col);
          }
        }

        if (mode == 2) // Format Expedition
        {
          WS = wxStringTokenize(str, _T(";,\t "));
          //x = wxAtoi(WS[0]);
          //col = (x + 1) / 2 - 1;
          col = wxAtoi(WS[0]);

          for (i = 1; i < (int)WS.GetCount(); i += 2)
          {
            //x = wxAtoi(WS[i]);
            //row = (x + 2) / 5 - 1;
            row = wxAtoi(WS[i]);
            s = WS[i + 1];
            if (col > WINDSPEED - 1) break;
            if (s == _T("0") || s == _T("0.00") || s == _T("0.0") || s == _T("0.000"))
            {
              continue;
            }
            //if (col < 21)
            if (col < WINDSPEED + 1){
              setValue(s, row, col);
            }
          }
        }
      }


        // END TR
*/

DEBUG("Try to load polar datas from file %s on SD-Card", polarDataFileName.c_str());
if (KBox.getSdFat().exists(polarDataFileName.c_str())) {

  DEBUG("Polar data file found");
} else {
  return false;
}

  if (File polarDataFile = KBox.getSdFat().open(polarDataFileName)) {
  const int ROW_DIM = 20;
  const int COL_DIM = 200;


    // Array for data.
    int array[ROW_DIM][COL_DIM];
    int i = 0;     // First array index.
    int j = 0;     // Second array index
    size_t n;      // Length of returned field with delimiter.
    char str[20];  // Must hold longest field with delimiter and zero byte.
    char *ptr;     // Test for valid field.
    int max_row = 0;
    int max_col = 0;

    // Read the file and store the data.

    for (i = 0; i < ROW_DIM; i++) {
      for (j = 0; j < COL_DIM; j++) {
        n = readField(&polarDataFile, str, sizeof(str), "\t\n");
        if (n == 0) {
          // no more lines
          continue;
        }
        DEBUG(str);
        array[i][j] = strtol(str, &ptr, 10);
        if (j > max_col) max_col = j;
        if( i > max_row) max_row = i;
        if (ptr == str) {
          DEBUG("bad number");
          return false;
        }
        while (*ptr == ' ') {
          ptr++;
        }
        if (*ptr == '\n' || *ptr == '\0') {
          // next row
          continue;
        }
      }
      // Allow missing endl at eof.
      if (str[n-1] != '\n' && polarDataFile.available()) {
        DEBUG("missing endl");
        return false;
      }

    }

    DEBUG("i= %i / j= %i",max_row,max_col);

    // Print the array.
    for (i = 0; i < max_row; i++) {
      for (j = 0; j < max_col; j++) {
        if (j) {
          Serial.print(' ');
        }
        Serial.print(array[i][j]);
      }
      Serial.println();
    }
    Serial.println("Done");
    polarDataFile.close();
    return true;
  } else {
    DEBUG("Something went wrong, Polar data file not found");
    return false;
  }

  return false;
}

size_t Polar::readField(File* file, char* str, size_t size, const char* delim) {
  char ch;
  size_t n = 0;
  while ((n + 1) < size && file->read(&ch, 1) == 1) {
    // Delete CR.
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (strchr(delim, ch)) {
        break;
    }
  }
  str[n] = '\0';
  return n;
}
