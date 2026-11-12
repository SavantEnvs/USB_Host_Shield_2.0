// Known-answer probe: runs the library's parsers on fixed inputs and prints what they compute.
// Built with the project's normal flags (no sanitizers) and dynamically linked, so mayhem/test.sh
// can compare its output with golden values. Usage: kat_probe <config|reportdesc|ps4|keyboard>
#include <new>
#include <Usb.h>
#include <PS4Parser.h>
#include <hidboot.h>
#include <hidescriptorparser.h>

// USB HID boot keyboard: configuration descriptor with one interface and one interrupt-IN endpoint.
static const uint8_t kKeyboardConfig[] = {
        0x09, 0x02, 0x22, 0x00, 0x01, 0x01, 0x00, 0xA0, 0x32,
        0x09, 0x04, 0x00, 0x00, 0x01, 0x03, 0x01, 0x01, 0x00,
        0x09, 0x21, 0x11, 0x01, 0x00, 0x01, 0x22, 0x3F, 0x00,
        0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x0A,
};

// Standard HID boot mouse report descriptor.
static const uint8_t kMouseReportDesc[] = {
        0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x03,
        0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05, 0x81, 0x01,
        0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06,
        0xC0, 0xC0,
};

struct Recorder : UsbConfigXtracter {
        void EndpointXtract(uint8_t conf, uint8_t iface, uint8_t alt, uint8_t proto, const USB_ENDPOINT_DESCRIPTOR *ep) override {
                printf("endpoint conf=%u iface=%u alt=%u proto=%u addr=0x%02x attr=0x%02x mps=%u interval=%u\n",
                       conf, iface, alt, proto, ep->bEndpointAddress, ep->bmAttributes, ep->wMaxPacketSize, ep->bInterval);
        }
};

struct Keyboard : KeyboardReportParser {
        void OnKeyDown(uint8_t mod, uint8_t key) override {
                printf("down mod=0x%02x key=0x%02x ascii=%c\n", mod, key, OemToAscii(mod, key));
        }
        void OnKeyUp(uint8_t mod, uint8_t key) override {
                printf("up mod=0x%02x key=0x%02x\n", mod, key);
        }
};

struct Ps4 : PS4Parser {
        using PS4Parser::Parse;
        void sendOutputReport(PS4Output *) override {}
};

int main(int argc, char **argv) {
        if (argc != 2)
                return 2;
        const char *what = argv[1];
        if (!strcmp(what, "config")) {
                Recorder rec;
                ConfigDescParser<USB_CLASS_HID, HID_BOOT_INTF_SUBCLASS, USB_HID_PROTOCOL_KEYBOARD, CP_MASK_COMPARE_ALL> whole(&rec);
                uint16_t off = 0;
                whole.Parse(sizeof kKeyboardConfig, kKeyboardConfig, off);
                ConfigDescParser<USB_CLASS_HID, HID_BOOT_INTF_SUBCLASS, USB_HID_PROTOCOL_KEYBOARD, CP_MASK_COMPARE_ALL> chunked(&rec);
                for (size_t o = 0; o < sizeof kKeyboardConfig; o += 8) {
                        size_t n = sizeof kKeyboardConfig - o < 8 ? sizeof kKeyboardConfig - o : 8;
                        off = (uint16_t)o;
                        chunked.Parse((uint16_t)n, kKeyboardConfig + o, off);
                }
                ConfigDescParser<USB_CLASS_MASS_STORAGE, 0, 0, CP_MASK_COMPARE_CLASS> other(&rec);
                off = 0;
                other.Parse(sizeof kKeyboardConfig, kKeyboardConfig, off);
                return 0;
        }
        if (!strcmp(what, "reportdesc")) {
                mayhem_serial_sink = stdout;
                ReportDescParser p;
                uint16_t off = 0;
                p.Parse(sizeof kMouseReportDesc, kMouseReportDesc, off);
                printf("\n");
                return 0;
        }
        if (!strcmp(what, "ps4")) {
                uint8_t r[64] = {0};
                r[0] = 0x01;
                r[1] = 0x11; r[2] = 0x22; r[3] = 0x33; r[4] = 0x44;
                r[5] = 0x22; r[6] = 0x09; r[7] = 0x01;
                r[8] = 0x55; r[9] = 0x66;
                Ps4 p;
                p.Parse(sizeof r, r);
                printf("hats %u %u %u %u\n", p.getAnalogHat(LeftHatX), p.getAnalogHat(LeftHatY), p.getAnalogHat(RightHatX), p.getAnalogHat(RightHatY));
                printf("triggers %u %u\n", p.getAnalogButton(L2), p.getAnalogButton(R2));
                printf("pressed cross=%d circle=%d l1=%d r2=%d ps=%d right=%d up=%d\n", p.getButtonPress(CROSS), p.getButtonPress(CIRCLE),
                       p.getButtonPress(L1), p.getButtonPress(R2), p.getButtonPress(PS), p.getButtonPress(RIGHT), p.getButtonPress(UP));
                printf("click cross=%d then=%d\n", p.getButtonClick(CROSS), p.getButtonClick(CROSS));
                return 0;
        }
        if (!strcmp(what, "keyboard")) {
                // KeyboardReportParser leaves prevState uninitialised; start it from zeros like a fresh sketch would.
                void *mem = calloc(1, sizeof(Keyboard));
                Keyboard &kbd = *new (mem) Keyboard();
                uint8_t shiftA[8] = {0x02, 0, 0x04, 0, 0, 0, 0, 0};
                uint8_t b[8] = {0, 0, 0x05, 0, 0, 0, 0, 0};
                uint8_t digit[8] = {0, 0, 0x1e, 0x21, 0, 0, 0, 0};
                kbd.Parse(NULL, false, 8, shiftA);
                kbd.Parse(NULL, false, 8, b);
                kbd.Parse(NULL, false, 8, digit);
                free(mem);
                return 0;
        }
        return 2;
}
