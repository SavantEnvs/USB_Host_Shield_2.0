# ConfigDescParser writes past its 16-byte `varBuffer` on an oversized descriptor

`confdescparser.h` (`ConfigDescParser::ParseDescriptor`, states 3 and 4) sets
`theBuffer.valueSize = dscrLen - 2` from the descriptor's own `bLength` byte and then runs
`MultiByteValueParser::Parse` for configuration, interface and endpoint descriptors. `varBuffer` is only
16 bytes, so a device that reports `bLength` > 18 for one of these descriptors (or 0/1, where `dscrLen - 2`
wraps to 254/255) makes the parser write up to 253 bytes past the buffer: first the parser's own state
fields (`stateParseDescr`, `dscrLen`, ...), then the stack. Every driver that enumerates through
`ConfigDescParser` (CDC ACM, HID, mass storage, Xbox, ...) is exposed to what a hostile USB device sends.

- `stack-overflow.input`: `fuzz_config_desc-standalone` reports `stack-buffer-overflow` WRITE in
  `MultiByteValueParser::Parse` (`parsetools.cpp:32`).
- `hang.input`: the corrupted state sends `Parse` into an endless loop (libFuzzer timeout).

Input format: first byte is the chunk size (0 = whole descriptor), the rest is the descriptor.

One-line fix: clamp the length to the buffer, e.g. reject the descriptor (or skip it with `theSkipper`) when
`dscrLen < 2 || dscrLen - 2 > sizeof(varBuffer)` before setting `valueSize`.
