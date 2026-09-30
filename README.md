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
bash scripts/build-test.sh
./build/linux/impressora_custom
```

If Qt was installed through the Qt installer, supply its kit directory:

```sh
bash scripts/build-test.sh --qt-prefix /path/to/Qt/6.x/gcc_64
```

## Build on Windows

Install Qt 6.2+ with Qt Quick/Quick Controls, CMake 3.21+, and a matching MSVC
or MinGW C++ toolchain. Open the shell for that compiler. For example, in an
MSVC Developer PowerShell (adjust the Qt path to your installed version):

```powershell
.\scripts\build-test.ps1 -QtPrefix "C:/Qt/6.8.3/msvc2022_64"
& "C:/Qt/6.8.3/msvc2022_64/bin/windeployqt.exe" --qmldir . build/windows/Release/impressora_custom.exe
& ./build/windows/Release/impressora_custom.exe
```

If PowerShell blocks the script, invoke it with a process-local policy override:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build-test.ps1 -QtPrefix "C:/Qt/6.8.3/msvc2022_64"
```

For MinGW, open a shell with the matching compiler on PATH and pass the matching
Qt kit with `-Generator "MinGW Makefiles"`. That generator places the executable
directly in `build/windows`; adjust the deploy/run paths accordingly.

Both scripts configure, compile, and run the tests, stopping on failure. They
work from any current directory, default to Release, and always enable tests.
Linux accepts `--config`, `--build-dir`, and `--generator`; Windows accepts
`-Configuration`, `-BuildDir`, and `-Generator`. Relative custom build paths
resolve against the current directory. Use a different build directory when
changing compiler, Qt kit, or generator. Windows adds the selected Qt kit to
PATH for testing and restores PATH afterward. Dependencies must already be
installed; the scripts do not install packages or run the printer app.

Tests are enabled by default in CMake; use `-DBUILD_TESTING=OFF` for a manual
build without Qt Test. Raw TCP printing requires no printer driver or Elgin SDK.

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
  six lines (`ESC d 6`), and requests a partial cut (`GS V 1`).
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
