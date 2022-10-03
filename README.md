Introduction
============

This is a fork of the OpenSpin compiler, an open source compiler for the Spin/PASM language of the Parallax Propeller. It was ported from Chip Gracey's original x86 assembler code (see https://github.com/parallaxinc/OpenSpin). The only binary differences in the output are from the corrected handling of floating point numbers (which is now IEEE compliant).

This fork was refactored to a more modern C++ dialect.

The spin compiler was tested against approximately 2000 OBJEX Spin Files to produces binary exact results as Parallax OpenSpin compiler.

Usage
-----

``openspin.exe -u -I include-path-to-library-folder mainfile.spin``

Run openspin.exe with no arguments to get a usage description.

Downloads
---------

Prebuild static binaries can be found here: [releases][https://github.com/ThiloA/OpenSpin/releases]

* Oldest supported Windows (tested): Windows 2000 (x86)
* Oldest supported Linux (tested): Debian 5 Kernel 2.6.26 (x86)

Improvements
------------

* unused method optimization now works with objects that have more than 255 routines *before* optimization
* switched from an almost one pass compiler to a tokenizer - parser - optimizer - generator architecture
* uses exceptions instead of return values for error, continuation on errors (multiple error messages) should now be possible (not yet implemented)
* limitations that have not reason in the interpreter or propeller chip architecture are gone (e.g. number of nested blocks, depth of expressions, cases, etc.)
* additional json/html output of object for debugging purposes

Known Limitations
-----------------

Some of the following limitations will be fixed in future releases.

* The #include, #warn and #info macros are out of order (will be fixed)
* Tree view (-t Option) is currently not supported
* List of archives (-f Option) is currently not supported
* Alternative preprocessor rules (-a Option) enabled always
* Extract doc comments not supported (-d Option)
* Dump Symbols (-s) not supported, you may use html output mode

Building
--------

Run the following command to build the compiler. No external libraries aside from the C++ Standard Template Library are required.

Linux (gcc):
``g++ main.cpp -I. -O2 -o openspin``

Linux (clang):
``clang++ main.cpp -I. -O2 -o openspin``

Windows (mingw):
``mingw32-g++ main.cpp -I. -O2 -o openspin.exe``

Older compilers may need an additional -std=c++11 parameter. Other compilers have not been tested. With msvc you might get problems regarding "incbin" macro. In this case define a macro SPINCOMPILER_EXCLUDE_HTML_SUPPORT. This will drop html output support.

