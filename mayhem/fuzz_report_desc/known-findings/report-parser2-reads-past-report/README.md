# ReportDescParser2 reads past the end of the input report

`ReportDescParser2::OnInputItem` (`hidescriptorparser.cpp`, ~line 1562) walks the input report bit by bit
using the `Report Size` / `Report Count` values taken from the HID report descriptor, and dereferences
`*p` without ever comparing `p` with `pBuf + bLen`. A descriptor that declares more input bits than the
report actually holds (any device can send one) makes `UniversalReportParser::Parse` read past the end of
the report buffer it was handed.

`heap-overflow.input` makes `fuzz_report_desc-standalone` report a `heap-buffer-overflow` READ of 1 byte,
0 bytes after the 3-byte report. Input format: chunk size, report length, report bytes, descriptor bytes.

One-line fix: stop the loop in `OnInputItem` (and the `pBuf` advance in `ParseItem`) once
`p >= pBuf + bLen`.
