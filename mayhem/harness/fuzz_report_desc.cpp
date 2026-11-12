// Feeds a HID report descriptor to the library's report descriptor parsers: the pretty-printing
// ReportDescParser and ReportDescParser2, which also decodes an input report against the descriptor.
#include <Usb.h>
#include <hidescriptorparser.h>

static const size_t kMaxDescriptor = 4096;

static void Deliver(USBReadParser &p, const uint8_t *buf, size_t size, size_t chunk) {
        for (size_t off = 0; off < size; off += chunk) {
                size_t n = size - off < chunk ? size - off : chunk;
                uint16_t offset = (uint16_t)off;
                p.Parse((uint16_t)n, buf + off, offset);
        }
}

// Input: [chunk size (0 = whole)] [input report length] [input report bytes...] [report descriptor bytes...]
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
        if (size < 3)
                return -1;
        size_t chunk = data[0];
        size_t rlen = data[1];
        if (rlen == 0 || 2 + rlen >= size || size - 2 - rlen > kMaxDescriptor)
                return -1;
        size_t dlen = size - 2 - rlen;
        if (chunk == 0)
                chunk = dlen;

        uint8_t *report = (uint8_t *)malloc(rlen);
        memcpy(report, data + 2, rlen);
        uint8_t *desc = (uint8_t *)malloc(dlen);
        memcpy(desc, data + 2 + rlen, dlen);

        ReportDescParser plain;
        Deliver(plain, desc, dlen, chunk);

        ReportDescParser2 decode((uint16_t)rlen, report);
        Deliver(decode, desc, dlen, chunk);

        free(desc);
        free(report);
        return 0;
}
