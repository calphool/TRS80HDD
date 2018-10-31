# TRS80HDD
Repo for TRS-80 Model 1 Hard Drive emulator project


![Version 2 of PCB](/img/TRS80HDD_v2.jpg?raw=true "Version 2 of PCB")

Here we go again.  So after I finished the graphics/sound/communications card, I started working on a new
project.  This time I'm working on interfacing the TRS-80 to a Teensy 3.6, which is an Arduino-ish microcontroller 
board.  It's a lot faster than the Arduino Uno, and has more ports.  You can use the Arduino tool chain, which is 
nice. 

Theory of Operation
===================

To understand how this card works (in theory... still working out some bugs as of October 2018), you have to 
understand how the TRS-80 expected to interface to floppy drives.  I'm in awe of what engineers in 1977 had to 
deal with to produce a working computer.  We take so much for granted today.  First of all, the TRS-80 Model 1
couldn't interface to a floppy drive unless you purchased the Expansion System.  That expansion system is 
essentially a large daughter board and a power supply.  It connected to your TRS-80 via a ribbon cable attached 
to the expansion bus on the keyboard/CPU unit.  Inside was a fairly complex set of components.  There's a real time
clock module that keeps pinging the CPU unit 40 times a second.  There's additional DRAM in there.  There's a bunch
of glue code that allows you to connect a parallel port printer (remember those?) and a UART for (very slow) 
RS232 / modem connectivity.  Finally, there's an [FD1771](https://en.wikipedia.org/wiki/Western_Digital_FD1771) chip.
That chip is one of the first floppy disk controller chips.  It was released in 1976, and was used in various forms 
up into the 1990s.  On one side of the chip you've got an interface designed to connect to a system bus, and on the 
other side you've got an interface designed to connect to a stepper driven floppy drive.  There's glue logic in the 
Expansion System that basically memory mapped the FD1771 chip into the address space of the Z80 processor of the 
CPU unit.  You get a little keyhole interface into RAM (0x37E0 - 0x37EF) that you can read and write to, and the chip 
responds by reading/writing to the floppy by manipulating the drive spindle and the head steppers.

So, the way this emulated device works is that it has an address decoder that watches for those addresses, along with 
read and write signal lines, triggers a flip-flop that puts the Z80 processor in the CPU unit into a WAIT state, 
invokes an interrupt on the Teensy, the Teensy reads an SD card for a disk image, and emulates the intent of the 
read or write command against that disk image file.  Once the read or write is complete the Teensy kicks the flip-flop 
and the WAIT state is released.  At least, that's the theory.  I'm still working through some timing issues before 
I start working in earnest on the Teensy code.  Basically I've got everything working right now except the data bus.
The data bus seems to be doing nothing at the moment, and I haven't figured out exactly what's wrong yet.


Help/Advice
===========

If you happen to have any experience with 8-bit hardware interfacing, I would certainly appreciate an opportunity to chat.
If you have access to a decent Logic Analyzer and are willing to loan it out, that would be even more awesome, since 
I find that trying to debug this emulator with nothing more than a 4-channel oscillator ends up being a challenge.
