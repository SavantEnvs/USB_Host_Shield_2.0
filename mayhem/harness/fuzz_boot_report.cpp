// Feeds a HID boot-protocol keyboard or mouse report to the library's boot report parsers.
#include <new>
#include <Usb.h>
#include <hidboot.h>

class Keyboard : public KeyboardReportParser {
public:
        uint32_t acc = 0;
protected:
        void OnKeyDown(uint8_t mod, uint8_t key) override { acc += OemToAscii(mod, key); }
        void OnKeyUp(uint8_t mod, uint8_t key) override { acc += OemToAscii(mod, key); }
};

// Input: [0=keyboard (8-byte report) 1=mouse (3+ byte report)] [report bytes...]
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
        if (size < 2 || size - 1 > 64)
                return -1;
        size_t len = size - 1;
        size_t need = data[0] == 0 ? 8 : 3;
        if (data[0] > 1 || len < need)
                return -1;
        uint8_t *buf = (uint8_t *)malloc(len);
        memcpy(buf, data + 1, len);
        if (data[0] == 0) {
                // KeyboardReportParser leaves prevState uninitialised; start from zeros so runs are deterministic.
                void *mem = calloc(1, sizeof(Keyboard));
                Keyboard &kbd = *new (mem) Keyboard();
                kbd.Parse(NULL, false, (uint8_t)len, buf);
                kbd.Parse(NULL, false, (uint8_t)len, buf);
                free(mem);
        } else {
                MouseReportParser mouse = MouseReportParser();
                mouse.Parse(NULL, false, (uint8_t)len, buf);
        }
        free(buf);
        return 0;
}
