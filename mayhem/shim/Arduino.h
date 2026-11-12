// Host-side stand-in for the Arduino core: just enough for the USB Host Shield library to compile
// and run its descriptor/report parsers on a PC. No hardware is touched; output is discarded.
#ifndef MAYHEM_SHIM_ARDUINO_H
#define MAYHEM_SHIM_ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <inttypes.h>

typedef bool boolean;
typedef uint8_t byte;
typedef unsigned int word;

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#define LSBFIRST 0
#define MSBFIRST 1
#define PI 3.1415926535897932384626433832795
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (s)
#define pgm_read_ptr(addr) (*(void * const *)(addr))
#define pgm_read_pointer(addr) pgm_read_ptr(addr)
#define strlen_P(s) strlen(s)
#define strcpy_P(d, s) strcpy((d), (s))
#define sprintf_P sprintf

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bitRead(v, b) (((v) >> (b)) & 0x01)
#define _BV(b) (1UL << (b))

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t) {}
inline int digitalRead(uint8_t) { return 0; }
inline void attachInterrupt(uint8_t, void (*)(void), int) {}
inline void detachInterrupt(uint8_t) {}
inline int digitalPinToInterrupt(int p) { return p; }
inline void delay(unsigned long) {}
inline void delayMicroseconds(unsigned int) {}
inline void yield(void) {}
inline unsigned long millis(void) { return 0; }
inline unsigned long micros(void) { return 0; }
inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

class Print {
public:
        virtual ~Print() {}
        virtual size_t write(uint8_t) { return 1; }
        virtual size_t write(const uint8_t *buf, size_t n) {
                size_t k = 0;
                while (k < n && write(buf[k]))
                        k++;
                return k;
        }
        size_t write(const char *s) { return s ? write((const uint8_t *)s, strlen(s)) : 0; }
        size_t print(const char *s) { return write(s); }
        size_t print(char c) { return write((uint8_t)c); }
        size_t print(int n, int base = DEC) { return print((long)n, base); }
        size_t print(unsigned int n, int base = DEC) { return print((unsigned long)n, base); }
        size_t print(unsigned char n, int base = DEC) { return print((unsigned long)n, base); }
        size_t print(long n, int base = DEC) {
                if (base == DEC && n < 0)
                        return write('-') + printNum((unsigned long)(-n), base);
                return printNum((unsigned long)n, base);
        }
        size_t print(unsigned long n, int base = DEC) { return printNum(n, base); }
        size_t print(double d, int digits = 2) {
                char b[48];
                int n = snprintf(b, sizeof b, "%.*f", digits, d);
                return write((const uint8_t *)b, n < 0 ? 0 : (size_t)n);
        }
        size_t println(void) { return write("\r\n"); }
        template <class T> size_t println(T v) { size_t n = print(v); return n + println(); }
        template <class T> size_t println(T v, int base) { size_t n = print(v, base); return n + println(); }
private:
        size_t printNum(unsigned long n, int base) {
                char b[8 * sizeof(long) + 1];
                char *p = &b[sizeof b - 1];
                *p = 0;
                if (base < 2)
                        base = 10;
                do {
                        unsigned long q = n / base;
                        char c = (char)(n - base * q);
                        *--p = c < 10 ? '0' + c : 'A' + c - 10;
                        n = q;
                } while (n);
                return write(p);
        }
};

class String {
public:
        String() {}
        String(const char *) {}
        String(int) {}
        String(unsigned int) {}
        String(long) {}
        String &operator+=(const String &) { return *this; }
};

class Stream : public Print {
public:
        virtual int available() = 0;
        virtual int read() = 0;
        virtual int peek() = 0;
        virtual void flush() = 0;
};

// Everything the library prints goes to Serial; it is discarded unless a sink is installed.
extern FILE *mayhem_serial_sink;

class HardwareSerial : public Stream {
public:
        void begin(unsigned long) {}
        int available() { return 0; }
        int read() { return -1; }
        int peek() { return -1; }
        void flush() {}
        using Print::write;
        size_t write(uint8_t c) {
                if (mayhem_serial_sink)
                        fputc(c, mayhem_serial_sink);
                return 1;
        }
        operator bool() { return true; }
};

extern HardwareSerial Serial;

#endif
