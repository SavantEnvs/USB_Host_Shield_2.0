// Feeds a USB configuration descriptor (as returned by GET_DESCRIPTOR) to the library's
// ConfigDescParser, wired to the endpoint extractors of several real drivers.
#include <Usb.h>
#include <cdcacm.h>
#include <hiduniversal.h>
#include <masstorage.h>
#include <XBOXONE.h>
#include <XBOXOLD.h>

static const size_t kMaxDescriptor = 4096;

static void Deliver(USBReadParser &p, const uint8_t *buf, size_t size, size_t chunk) {
        for (size_t off = 0; off < size; off += chunk) {
                size_t n = size - off < chunk ? size - off : chunk;
                uint16_t offset = (uint16_t)off;
                p.Parse((uint16_t)n, buf + off, offset);
        }
}

static void RunAll(const uint8_t *buf, size_t size, size_t chunk) {
        USB usb;
        ACM acm(&usb, NULL);
        HIDUniversal hid(&usb);
        BulkOnly msc(&usb);
        XBOXONE xone(&usb);
        XBOXOLD xold(&usb);

        ConfigDescParser<USB_CLASS_COM_AND_CDC_CTRL, CDC_SUBCLASS_ACM, CDC_PROTOCOL_ITU_T_V_250,
                CP_MASK_COMPARE_CLASS | CP_MASK_COMPARE_SUBCLASS> acmCtrl(&acm);
        ConfigDescParser<USB_CLASS_CDC_DATA, 0, 0, CP_MASK_COMPARE_CLASS> acmData(&acm);
        ConfigDescParser<USB_CLASS_HID, 0, 0, CP_MASK_COMPARE_CLASS> hidParser(&hid);
        ConfigDescParser<USB_CLASS_MASS_STORAGE, MASS_SUBCLASS_SCSI, MASS_PROTO_BBB,
                CP_MASK_COMPARE_CLASS | CP_MASK_COMPARE_SUBCLASS | CP_MASK_COMPARE_PROTOCOL> mscParser(&msc);
        ConfigDescParser<0, 0, 0, 0> xoneParser(&xone);
        ConfigDescParser<0, 0, 0, 0> xoldParser(&xold);

        Deliver(acmCtrl, buf, size, chunk);
        Deliver(acmData, buf, size, chunk);
        Deliver(hidParser, buf, size, chunk);
        Deliver(mscParser, buf, size, chunk);
        Deliver(xoneParser, buf, size, chunk);
        Deliver(xoldParser, buf, size, chunk);
}

// Input: [chunk size (0 = whole descriptor in one transfer)] [descriptor bytes...]
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
        if (size < 2 || size - 1 > kMaxDescriptor)
                return -1;
        size_t chunk = data[0] ? data[0] : size - 1;
        size_t len = size - 1;
        uint8_t *buf = (uint8_t *)malloc(len);
        memcpy(buf, data + 1, len);
        RunAll(buf, len, chunk);
        free(buf);
        return 0;
}
