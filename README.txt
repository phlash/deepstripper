DeepStripper for Akai DPS12/16 devices - the rework
===================================================
Author: Phil Ashby
Date: October 2009-

Why??
-----

The Tcl/Tk scripted version of DeepStripper has been updated several times
to include new features, but the *much more popular* Win32 binary version
hasn't, because it's an entirely different program written in C++ using an MS
specific toolchain (VC++/MFC) and is *closed source*.

I intend to re-work the Tcl/Tk version into a modern GTK-2.0 application
written in C that should easily compile for Mac/Win32/Linux on various
architectures (maybe even portable devices). This should result in a single
compatible binary that is easily installed / removed.

How?
----

Primarily developed on Linux, this code also compiles to a Win32 binary using
the mingw32 cross-compiler. The source archive currently contains all the GTK
runtime libraries required to run on Win32 platforms, these were obtained from
the official gtk archives here: http://www.gtk.org/download-windows.html
(version 2.16.6) where one can download the source code (as required in GPL).

Build dependancies on linux (Ubuntu) are: libc6-dev, libgtk2.0-dev

Current Makefile targets will create a source archive, a binary ZIP archive for
Win32, and a .deb package for Debian/Ubuntu or other linux distributions.

Phil (16/Jan/2010).
