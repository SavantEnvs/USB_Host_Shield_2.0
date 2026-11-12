# A HID long item makes `ReportDescParserBase` write past its `varBuffer`

In `ReportDescParserBase::ParseItem` (`hidescriptorparser.cpp`, state 0) a long-item prefix (`0xFE`) takes
the `USBTRACE("LONG")` branch and never sets `itemSize`. The unconditional `itemSize--` that follows wraps
the `uint8_t` from 0 to 255, so state 1 sets `theBuffer.valueSize = 255` and `MultiByteValueParser::Parse`
copies up to 255 descriptor bytes into `varBuffer`, which is `sizeof(USB_CONFIGURATION_DESCRIPTOR)` bytes
(`hidescriptorparser.h:104`). Any device can send a report descriptor that starts a long item.

`stack-overflow.input` makes `fuzz_report_desc-standalone` report `stack-buffer-overflow` WRITE of 1 byte in
`MultiByteValueParser::Parse` (`parsetools.cpp:32`, called from `ParseItem` at `hidescriptorparser.cpp:1125`).
This is the same `MultiByteValueParser::Parse` frame as the `ConfigDescParser` finding, but a different
cause: here the length comes from an unset local, not from `bLength`.
Input format: chunk size, report length, report bytes, descriptor bytes.

`chunked-clobbers-parser-state.input` is the same bug in a form ASan cannot see. Delivered in 8-byte chunks,
the long item's bytes stay inside the parser object itself: `varBuffer` (9 bytes) is followed in
`ReportDescParserBase` by `itemParseState`, `itemSize`, ..., and `pfUsage` (the usage-page handler pointer,
at `this+0x48`), so ASan sees no redzone hit. The parser then keeps running on the clobbered state and jumps through the damaged
`pfUsage`: `fuzz_report_desc-standalone` stops on `AddressSanitizer: SEGV` with `pc` equal to `&pfUsage`. Mayhem's
triage names the nearest return address it can unwind, `PrintUsagePage` at `hidescriptorparser.cpp:1272:33`
(reached from `Parse:1005`). That line is an `E_Notify` of a string constant, not a table lookup, so the
`PrintUsagePage:1272` defect is this long-item bug. Removing the `0xFE` byte from the input removes the
crash, and with chunk size 0 (first byte) the same descriptor stops in `MultiByteValueParser::Parse` as above.

One-line fix: for a long item, read `bDataSize` from the following byte (or skip the item) instead of falling
through with `itemSize == 0`.
