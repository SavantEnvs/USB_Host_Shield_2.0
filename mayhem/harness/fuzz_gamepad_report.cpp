// Feeds a controller input report to the library's game controller report parsers and then
// reads every accessor, as a sketch would on each loop().
#include <Usb.h>
#include <PS4Parser.h>
#include <PS5Parser.h>
#include <SwitchProParser.h>
#include <XBOXONESParser.h>

static const size_t kMaxReport = 255;

struct Ps4 : PS4Parser { using PS4Parser::Parse; void sendOutputReport(PS4Output *) override {} };
struct Ps5 : PS5Parser { using PS5Parser::Parse; void sendOutputReport(PS5Output *) override {} };
struct Switch : SwitchProParser { using SwitchProParser::Parse; void sendOutputReport(uint8_t *, uint8_t) override {} };
struct XboxS : XBOXONESParser { using XBOXONESParser::Parse; void sendOutputReport(uint8_t *, uint8_t) override {} };

static const AnalogHatEnum kHats[] = { LeftHatX, LeftHatY, RightHatX, RightHatY };

template <class P>
static uint32_t ReadButtons(P &p) {
        uint32_t acc = 0;
        for (int b = 0; b <= (int)CAPTURE; b++) {
                acc += (uint32_t)p.getButtonPress((ButtonEnum)b);
                acc += (uint32_t)p.getButtonClick((ButtonEnum)b);
        }
        for (size_t i = 0; i < sizeof(kHats) / sizeof(kHats[0]); i++)
                acc += (uint32_t)p.getAnalogHat(kHats[i]);
        return acc;
}

// Input: [controller: 0=PS4 1=PS5 2=Switch Pro 3=Xbox One S] [report bytes...]
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
        if (size < 2 || size - 1 > kMaxReport)
                return -1;
        size_t len = size - 1;
        uint8_t *buf = (uint8_t *)malloc(len);
        memcpy(buf, data + 1, len);
        volatile uint32_t sink = 0;

        switch (data[0]) {
        case 0: {
                Ps4 p;
                p.Parse((uint8_t)len, buf);
                p.Parse((uint8_t)len, buf);
                sink = ReadButtons(p) + p.getAnalogButton(L2) + p.getAnalogButton(R2) + p.getX(0, 0) + p.getY(0, 0) +
                        p.getX(1, 1) + p.getY(1, 1) + p.getTouchCounter(0, 0) + p.getTouchCounter(1, 1) +
                        (uint32_t)p.getAngle(Pitch) + (uint32_t)p.getAngle(Roll) + p.getSensor(gX) + p.getSensor(aZ) +
                        p.getBatteryLevel() + p.getUsbStatus() + p.getAudioStatus() + p.getMicStatus();
                break;
        }
        case 1: {
                Ps5 p;
                p.Parse((uint8_t)len, buf);
                p.Parse((uint8_t)len, buf);
                sink = ReadButtons(p) + p.getAnalogButton(L2) + p.getAnalogButton(R2) + p.getX(0) + p.getY(0) +
                        p.getX(1) + p.getY(1) + p.getTouchCounter(0) + p.getTouchCounter(1) +
                        (uint32_t)p.getAngle(Pitch) + (uint32_t)p.getAngle(Roll) + p.getSensor(gX) + p.getSensor(aZ) +
                        p.getMessageCounter();
                break;
        }
        case 2: {
                Switch p;
                p.Parse((uint8_t)len, buf);
                p.Parse((uint8_t)len, buf);
                sink = ReadButtons(p) + (uint32_t)p.getAngle(Pitch) + (uint32_t)p.getAngle(Roll) + p.getSensor(gX) +
                        p.getSensor(aZ) + p.getBatteryLevel() + p.getMessageCounter();
                break;
        }
        case 3: {
                XboxS p;
                p.Parse((uint8_t)len, buf);
                p.Parse((uint8_t)len, buf);
                sink = ReadButtons(p);
                break;
        }
        default:
                free(buf);
                return -1;
        }
        (void)sink;
        free(buf);
        return 0;
}
