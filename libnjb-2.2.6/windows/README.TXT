Using libnjb and sample programs under Windows/Win32
----------------------------------------------------

Nominally, if you have already installed Creatives drivers on your 
machine, you should install the libusb-win32 filter driver for use
with libnjb on Windows. The filter driver is a simple installer that
can be found at http://libusb-win32.sourceforge.net/

The current version (as of writing) is version 0.1.10, but any later
version will probably work just as fine.

Plug in your device, exit any Creative applications and run some 
sample programs to test that the library works. Start with the program
program "handshake.exe", then test "tracks.exe" and whatever you want
to try out.

If you only want to run the sample programs, this is all you need to
do.

You can then try to compile your own programs against libnjb if you 
like. You will need the GNU MinGW toolchain. The details are explained
below.


Compilation of programs under Windows/Win32
-------------------------------------------

1. libnjb must be compiled with the MinGW Windows compiler, see
   http://www.mingw.org/ install both MinGW and MSYS.

2. libnjb needs libusb-win32, see http://libusb-win32.sourceforge.net
   Depending on what you have on your machine, you have to do
   different things:

   A: If you already have Creative/Notmad software on this machine,
      Install the libusb-win32 filter binary from libusb-win32. An
      installer is available on the file download page for 
      libusb-win32

   B: If you have no Creative/Notmad or otherwise official drivers
      on your machine, you need to install the full driver. Do this
      by downloading the libusb-win32 device binary from libusb-win32.
      Then combine this with the .inf file found in the libnjb source
      tree "windows" directory to create a driver package. Then 
      choose this .inf file, right-click and install, or select it
      when you plug in your device and you're asked for a driver.

3. To compile libnjb you need the libusb-win32 device binary, because
   this binary includes the library itself and the usb.h file. Unzip
   the libusb-win32 driver binary and copy the libusb.a file to 
   something like C:\mingw\lib and usb.h to something like
   C:\mingw\include.

4. Unzip the libnjb sources and run MSYS. Enter the libnjb source dir
   and type "./configure" followed by "make". libnjb should build.
   You can immediately run the samples and verify that it works.

5. Then install libnjb and compile your own programs... By default
   libnjb installs into the local tree (mostly c:\mingw\usr\local)
   which confuse parts of MinGW so perhaps you're better off by
   installing to /usr with "./configure --prefix=/usr".

6. If you make an installer for your Windows program you might want
   to do something that checks for Creatives driver, and if they're
   present will install the filter driver, else installs the device
   driver.
