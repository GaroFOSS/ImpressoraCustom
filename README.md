# Impressora Custom

A small Qt 6 / C++ application with a QML interface for an Elgin i8 at
**192.168.0.113:9100**. Enter text, choose Small / Medium / Big and
Left / Center / Right, then click **Print**. Each job advances the paper and
requests a partial cut.

## Build on Linux

Install CMake 3.21+, a C++17 compiler, and Qt 6.2+ with Quick, Quick Controls 2,
Network, and Test. On Debian/Ubuntu:

```sh
sudo apt-get update
sudo apt-get install build-essential cmake qt6-base-dev qt6-declarative-dev \
    qml6-module-qtqml qml6-module-qtqml-models \
    qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-window qml6-module-qtquick-templates qml6-module-qtqml-workerscript
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/impressora_custom
```

If Qt was installed through the Qt installer, supply its kit directory with
`-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64` when configuring.

## Build on Windows

Install Qt 6.2+ with Qt Quick/Quick Controls, CMake 3.21+, and a matching MSVC
or MinGW C++ toolchain. Open the shell for that compiler. For example, in an
MSVC Developer PowerShell (adjust the Qt path to your installed version):

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
& "C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe" --qmldir . build/Release/impressora_custom.exe
& ./build/Release/impressora_custom.exe
```

Tests are enabled by default; use `-DBUILD_TESTING=OFF` if building without
Qt Test. Raw TCP printing requires no printer driver or Elgin SDK.

## Printer behavior

- Small is normal Font A; Medium and Big enlarge width and height by 2× and 3×.
- Size and alignment apply to the entire job. The printer wraps long lines
  according to its configured print width.
- Text is converted to CP850, including Portuguese accents such as é, ã, and ç.
  Decomposed accents are normalized. CRLF and CR become LF; existing spacing
  and line breaks are preserved. Unsupported characters (including emoji and
  tabs) and embedded control commands produce an error before connecting.
- Every job initializes the printer, exits double-byte mode (`FS .`), selects
  CP850 (`ESC t 2`), applies formatting, prints, restores normal size, feeds
  three lines (`ESC d 3`), and requests a partial cut (`GS V 1`).
- **Sent to printer** means all bytes were transmitted by the socket. It does
  not confirm paper availability or completion of the physical print/cut.
- The app remains responsive while connecting/sending. A job times out after
  10 seconds. Errors never trigger automatic retries; text stays in the box.
  If a connection fails partway through, check the paper before retrying,
  because the printer may already have received part of the job.
- The printer endpoint is fixed in the default `PrinterController` constructor.

## Hardware acceptance

On the actual i8, print `ação, café, maçã, São Paulo` in all nine combinations
of size and alignment, plus a multiline example. Check accents, alignment,
size, feed distance, and the partial cut.

`ESC t 2` uses the standard ESC/POS CP850 table numbering. Its mapping on the
installed i8 firmware still requires physical verification; the code page
selector is isolated in `src/escpos.cpp` if your firmware uses another number.
The cutter must be enabled in the printer configuration.

References: [Elgin i8 specifications](https://d2u2qhufg0q9tn.cloudfront.net/assets/arquivos/manual_2a2228fd-72b1-4b99-8b66-8177650aaef5_Lamina_Impressora_i8__Elgin.pdf),
[ESC/POS character tables](https://download4.epson.biz/sec_pubs/pos/reference_en/escpos/esc_lt.html),
[character sizes](https://download4.epson.biz/sec_pubs/pos/reference_en/escpos/gs_exclamation.html),
and [cut commands](https://download4.epson.biz/sec_pubs/pos/reference_en/escpos/gs_cv.html).

The automated tests use localhost TCP receivers and never contact the real printer.
