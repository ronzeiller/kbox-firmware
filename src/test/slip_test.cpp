/*
  The MIT License

  Copyright (c) 2016 Thomas Sarlandie thomas@sarlandie.net

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


#include "KBoxTest.h"
#include "common/comms/SlipStream.h"

struct rxBuffer {
  size_t len;
  size_t index;
  const uint8_t *data;
};

class StreamMock : public Stream {
  private:
    // For mock purposes, we have a number of rxBuffers, initially, only
    // the first one will be delivered, then the next one, etc.
    unsigned int countBuffers = 0;
    unsigned int currentBuffer = 0;
    struct rxBuffer *rxBuffers = 0;

    struct rxBuffer *getCurrentBuffer() {
      if (currentBuffer < countBuffers) {
        return &rxBuffers[currentBuffer];
      }
      else {
        return 0;
      }
    };

  public:
    StreamMock() {};

    StreamMock(int count, struct rxBuffer *b) : countBuffers(count), rxBuffers(b) {
      for (unsigned int i = 0; i < countBuffers; i++) {
        rxBuffers[i].index = 0;
      }
    };

    void advanceToNextBuffer() {
      currentBuffer++;
    };

    int available() {
      struct rxBuffer *b = getCurrentBuffer();

      if (b) {
        return b->len - b->index;
      }
      else {
        return 0;
      }
    };

    int read() {
      struct rxBuffer *b = getCurrentBuffer();

      if (b && b->index < b->len) {
        return b->data[b->index++];
      }
      else {
        return -1;
      }
    };

    int peek() {
      struct rxBuffer *b = getCurrentBuffer();

      if (b && b->index < b->len) {
        return b->data[b->index];
      }
      else {
        return -1;
      }
    };

    size_t write(uint8_t b) {
      return 1;
    };

    void flush() {
      // nop
    };
};

uint8_t ptr[100];

TEST_CASE("reading an empty stream") {
  StreamMock emptyStream(0, 0);
  SlipStream slip(emptyStream, 100);

  THEN("available should be 0") {
    REQUIRE( slip.available() == 0 );
  }

  THEN("read should return 0") {
    uint8_t ptr[100];
    REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 0 );
  }

  THEN("peek should return 0") {
    uint8_t *ptr;
    REQUIRE( slip.peekFrame(&ptr) == 0 );
  }
}

TEST_CASE("reading EOM bytes repeatedly") {
  struct rxBuffer b;
  b.len = 10;
  b.data = (const uint8_t*)"\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0\xc0";

  StreamMock eomBytesStream(1, &b);
  SlipStream slip(eomBytesStream, 100);

  THEN("available should be 0") {
    REQUIRE( slip.available() == 0 );
  }
}

TEST_CASE("reading simple message") {
  uint8_t ptr[100];

  struct rxBuffer b;
  b.len = 12;
  b.data = (const uint8_t*)"\xc0""1234567890\xc0";

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  THEN("should be able to peek 10 bytes") {
    uint8_t *ptr;
    REQUIRE( slip.available() == 10 );
    REQUIRE( slip.peekFrame(&ptr) == 10 );
    REQUIRE( memcmp(ptr, b.data+1, 10) == 0 );

    THEN("packet should still be available after peek") {
      REQUIRE( slip.available() == 10 );

      THEN("packet should be discarded after a readFrame(0,0)") {
        slip.readFrame(0, 0);
        REQUIRE( slip.available() == 0 );
      }
    }
  }

  THEN("should be able to read 10 bytes") {
    REQUIRE( slip.available() == 10 );
    REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 10);
    REQUIRE( memcmp(ptr, b.data+1, 10) == 0 );

    THEN("message should be discarded after calling readFrame") {
      REQUIRE( slip.available() == 0 );
    }
  }

}

TEST_CASE("reading message with end of frame header") {
  uint8_t ptr[100];

  struct rxBuffer b;
  b.len = 13;
  b.data = (const uint8_t*)"\xc0\xc0""0123456789\xc0";

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  THEN("should be able to peek 10 bytes") {
    uint8_t *p;
    REQUIRE( slip.available() == 10 );
    REQUIRE( slip.peekFrame(&p) == 10);
    REQUIRE( memcmp(p, b.data+2, 10) == 0 );

    THEN("packet should still be available after peek") {
      REQUIRE( slip.available() == 10 );
    }
  }

  THEN("should be able to read 10 bytes") {
    REQUIRE( slip.available() == 10 );
    REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 10);
    REQUIRE( memcmp(ptr, b.data + 2, 10) == 0 );
  }
}

TEST_CASE("reading simple message with escaping") {
  struct rxBuffer b;
  b.len = 11;
  b.data = (const uint8_t*)"\xc0""1:\xdb\xdc 2:\xdb\xdd\xc0";
  const uint8_t *expected = (const uint8_t*)"1:\xc0 2:\xdb";

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  REQUIRE( slip.available() == 7 );
  REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 7);
  REQUIRE( memcmp(ptr, expected, 7) == 0 );
}

TEST_CASE("reading two messages in two read calls") {
  struct rxBuffer rxb[2];
  rxb[0].len = 11;
  rxb[0].data = (const uint8_t*)"\xc0""123456789\xc0";
  rxb[1].len = 4;
  rxb[1].data = (const uint8_t*)"abc\xc0";

  StreamMock bytesStream(2, rxb);
  SlipStream slip(bytesStream, 100);

  REQUIRE( slip.available() == 9 );
  REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 9);
  REQUIRE( memcmp(ptr, rxb[0].data + 1, 9) == 0 );

  bytesStream.advanceToNextBuffer();

  REQUIRE( slip.available() == 3 );
  REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 3 );
  REQUIRE( memcmp(ptr, rxb[1].data, 3) == 0 );
}

TEST_CASE("messages split across multiple read-calls") {
  struct rxBuffer rxb[2];
  rxb[0].len = 10;
  rxb[0].data = (const uint8_t*)"\xc0""123456789";
  rxb[1].len = 4;
  rxb[1].data = (const uint8_t*)"abc\xc0";

  StreamMock bytesStream(2, rxb);
  SlipStream slip(bytesStream, 100);

  REQUIRE( slip.available() == 0 );
  REQUIRE( rxb[0].index == rxb[0].len );

  bytesStream.advanceToNextBuffer();

  REQUIRE( slip.available() == 12 );
  REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 12);
  REQUIRE( memcmp(ptr, rxb[0].data + 1, 9) == 0 );
  REQUIRE( memcmp(ptr + 9, rxb[1].data, 3) == 0 );
}

TEST_CASE("an invalid escape sequence is used") {
  struct rxBuffer b;
  b.len = 4;
  b.data = (const uint8_t*)"\xc0\xdb\x42\xc0";

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  REQUIRE( slip.available() == 0 );
  REQUIRE( slip.invalidFrameErrors() == 1 );

  REQUIRE( slip.available() == 0 );
}

TEST_CASE("a message is bigger than the mtu") {
  struct rxBuffer b;
  b.len = 200;
  uint8_t *data = (uint8_t*) malloc(200);
  b.data = data;

  data[0] = 0xc0;
  for (int i = 1; i < 199; i++) {
    data[i] = 0x42;
  }
  data[199] = 0xc0;

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  REQUIRE( slip.available() == 0 );
  REQUIRE( slip.invalidFrameErrors() == 1 );

  REQUIRE( slip.available() == 0 );
}

TEST_CASE("do not share crap sent before first message separator") {
  uint8_t ptr[100];

  struct rxBuffer b;
  b.len = 16;
  b.data = (const uint8_t*)"crap\xc0""0123456789\xc0";

  StreamMock bytesStream(1, &b);
  SlipStream slip(bytesStream, 100);

  THEN("should be able to read 10 bytes") {
    REQUIRE( slip.available() == 10 );
    REQUIRE( slip.readFrame(ptr, sizeof(ptr)) == 10);
    REQUIRE( memcmp(ptr, "0123456789", 10) == 0 );
  }
}

