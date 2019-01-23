# TRS80HDD
Repo for TRS-80 Model 1 Drive emulator project


![Version 4.1 of PCB](/img/TRS80HDD_v4.1.jpg?raw=true "Version 4.1 of PCB")

Here we go again.  So after I finished the graphics/sound/communications card, I started working on a new
project.  This time I'm working on interfacing the TRS-80 to a Teensy 3.6, which is an Arduino-ish microcontroller 
board.  It's a lot faster than the Arduino Uno, and has more ports.  You can use the Arduino tool chain, which is 
nice. 

Theory of Operation
===================

To understand how this card works (in theory... still working out some bugs as of December 2018), you have to 
understand how the TRS-80 expected to interface to floppy drives.  I'm in awe of what engineers in 1977 had to 
deal with to produce a working computer.  We take so much for granted today.  First of all, the TRS-80 Model 1
couldn't interface to a floppy drive unless you purchased the Expansion System.  That expansion system is 
essentially a large daughter board and a power supply.  It connected to your TRS-80 via a ribbon cable attached 
to the expansion bus on the keyboard/CPU unit.  Inside was a fairly complex set of components.  There's a real time
clock module that keeps pinging the CPU unit 40 times a second.  There's additional DRAM in there.  There's a bunch
of glue logic that allows you to connect a parallel port printer (remember those?) and a UART for (very slow) 
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

Build Notes
===========

January 22, 2019
![It's Alive!](/img/its-alive.jpg?raw=true "It's Alive!")

Holy smokes.  [It's working.](https://www.youtube.com/watch?v=pcdh7HSC1nE)  I've got NEWDOS/80 v2 booting up, and it seems quite functional.  
Since I haven't implemented WRITE handling code, and only the most critical READ handling code, it's pretty fragile right now, but it's booting 
and responding to commands.  This is the v4 board, so if you're following along and want to make your own, v4 with current code is the first 
actually functioning configuration.  


January 16, 2019

Considerable gnashing of teeth has occurred over the past month, but progress has been made.  The v3 and v3.1 designs 
are basically a bust.  There has been some kind of race condition on the WR* side of the data bus, and figuring out what
was going on has been nontrivial.  Folks on the Teensy forum basically said that the problem was that I was using 
unrelated Teensy ports for my 8 data signals, and that digitalReadFast() wasn't fast enough when you used it that way.
So they suggested I redesign the circuit so that all the data lines reside in a single Teensy port, which I did....
and it made absolutely no difference... Luckily I didn't completely trust their diagnosis and I simultaneously created the 
v4 design.  The v4 design was intended to fix the race condition by implementing a latch on the 
data bus (more or less a classical output port design).  This did improve things (actually started getting data to the 
Teensy sometimes), but it wasn't reliable.  After a lot of hand wringing, someone on the Teensy forums suggested a slight 
change to my wait flip flop triggering circuit so that it triggered as soon as an interesting address appeared (as soon 
as the 37EX signal toggled) rather than waiting for both that *and* a RD* or WR* signal.  This made the circuit fairly 
predictable, though it still glitches about 1 out of every 1000 writes.  The design is available in schematic v4.1.
I'm thinking that perhaps if I swap out all the LS series chips with ALS series chips, I'll get just enough of a 
performance boost across the combined propogation delay that I can get the WR* side rock solid like the RD* side is.  We'll
see.

However, the good news is that I'm actually booting stuff now.

![NEWDOS screen shot](/img/NewDosScreenShot.jpg?raw=true "Screen Shot")

~~There's still *some* kind of bug in my emulation, because it gets loaded and then doesn't give me control, and keeps 
trying to reload.  I know that the problem is somehow related to the clock signal, because when I shut it off it loads and
then just sits there.  If I turn it on, it loads, and then reloads, in a loop.  So I'm guessing I need to do something 
besides just turn the clock on, but I don't know what it's expecting just yet.  It could be as simple as making sure that
I'm handling the FD1771 status register correctly when I get to the end of the boot cycle.  I do think I've got a few bugs left 
in the status register handling, and I think I might have an off-by-one problem in handling sector read commands.  So either 
one of those problems could be the source of my rebooting problem.  However, I'm really close, and that makes me super happy.~~

With some help from someone on reddit, I was able to get it booting.  It's related to the fact that I'm using JV1 disk images, 
which apparently don't encode all the data that was on the original disks.  Apparently part of what was missing was a code that 
can be returned in the status register that indicates to the operating system that what it's reading represents directory data.
Since my disk images don't have that info, I guess my code is supposed to assume track 17 is the directory data, so it was 
as simple as forcing the status register to OR 0x20 with whatever it's already doing if the track is 17.

It's still not quite working right, because I can't do a DIR command (says "device not available"), but it's booting and semi-
usable now.  Now I just need to figure out why I can't do a DIR command.  I'm beginning to wonder if I shouldn't just figure 
out how to port the applicable code from SDLTRS over to the Teensy.  Then I'd be able to support several disk formats, and 
these kinds of strange little details would be handled for me, rather than me slowly finding them one by one.  I've avoided 
trying the port exercise, because I wasn't sure if the C compilers are compatible, or if I'll end up with strange data size 
issues or endian issues.  I might make one quick stab at getting it working without the porting effort, and then if it doesn't
work I'll dive into that.

--------------------------------------------------------


December 17, 2018

Well I'm working on version 2 of this project.  I found a copy of Byte Magazine from the late 70s that included a 
schematic from Steve Ciarcia for his Disk-80 project.  I decided to abandon my home grown address decoder and 
switched to his design.  The v3 of the schematic represents that.  Upon assembly, it didn't really work, and I 
was scratching my head.  I reached out to Peter Bartlett, who is the rock star that designed the MISE and M3SE 
boards that are light years beyond what my little projects are capable of.  He took a look at my schematics and 
helped me realize that I used a 74LS241 for my data buffer, and I probably meant to use a 74LS244.  The difference 
is that 74LS241 has four bits that are controlled by a positive enable pin, and the other four are controlled by 
a negative enable pin.  So I ordered some 74LS244's and we'll see where that gets me.
