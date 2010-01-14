DeepStripper for Akai DPS12/16 devices - the rework
===================================================
Author: Phil Ashby
Date: October 2009

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
