# Usage-name lookup tables are indexed one past their end

`output_if_between(v, l, h, ...)` (`macros.h`) uses `VALUE_BETWEEN`, which is *exclusive* at both ends, and
indexes the title table with `v - (l + 1)`. Three call sites in `ReportDescParserBase` pass a range one entry
longer than the table behind it, so the highest value in the range loads a `const char *` past the end of
the array. A device only has to put that usage page or usage in its HID report descriptor.

- `generic-desktop-usage-9.input`: Usage Page Generic Desktop, Usage `0x09`. `PrintGenericDesktopPageUsage`
  (`hidescriptorparser.cpp:1300`) reads `genDesktopTitles0[8]`, but the table has 8 entries (usages 1..8).
- `usage-page-0x10.input`: Usage Page `0x10`. `PrintUsagePage` (`hidescriptorparser.cpp:1258`, reached from
  `ParseItem:1167`) matches `output_if_between(page, 0x00, 0x11, ...)`, so pages 0x01..0x10 index
  `usagePageTitles0`, but that table has 15 entries (`hidescriptorparser.cpp:20`) and page `0x10` reads
  `usagePageTitles0[15]`. The table is also missing the entry for page `0x0E`, so PID and Unicode are
  labelled one page early.
- `game-controls-usage-0x39.input`: Usage Page Game Controls, Usage `0x39`. `PrintGameControlsPageUsage`
  (`hidescriptorparser.cpp:1342`) reads one entry past `gameTitles1`.

Each input makes `fuzz_report_desc-standalone` stop on an ASan `global-buffer-overflow` (an 8-byte read of a
`const char *` past the table) at the quoted line.
Input format: chunk size, report length, report bytes, descriptor bytes.

One-line fix: shorten the range bound by one (`0x0a` -> `0x09`, `0x3a` -> `0x39`, `0x11` -> `0x10`) or add the
missing title to each table.
