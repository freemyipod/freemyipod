#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven, benedikt93, Farthen
#
#
#    This file is part of emCORE.
#
#    emCORE is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emCORE is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emCORE.  If not, see <http://www.gnu.org/licenses/>.
#
#

"""
    Command line interface to communicate with emCORE devices.
    Run it without arguments to see usage information.
"""

import sys
import os
import time
import struct
import locale

from functools import wraps

import libemcore
import libemcoredata
from misc import Error, ArgumentError, ArgumentTypeError, Logger, getfuncdoc, gethwname, to_bool, to_int


def usage(errormsg=None, specific=False, docstring=True):
    """
        Prints the usage information.
        It is auto generated from various places.
    """
    logger = Logger()
    cmddict = Commandline.cmddict
    doc = getfuncdoc(cmddict)
    if not specific:
        logger.write("Please provide a command and (if needed) parameters as command line arguments\n\n")
        logger.write("Available commands:\n\n")
    else:
        logger.write("\n")
    for function in sorted(doc.items()):
        function = function[0]
        if specific == False or specific == function:
            logger.write(function + " ", 2)
            for arg in doc[function]['args']:
                logger.write("<" + arg + "> ")
            if doc[function]['kwargs']:
                for kwarg, kwvalue in doc[function]['kwargs']:
                    logger.write("[" + kwarg + " = " + str(kwvalue) + "] ")
            if doc[function]['varargs']:
                logger.write("<db1> ... <dbN>")
            if docstring and doc[function]['documentation']:
                logger.write("\n")
                logger.write(doc[function]['documentation']+"\n", 4)
            logger.write("\n")
    logger.write("\n")
    
    if errormsg:
        logger.error(str(errormsg)+"\n")
    exit(2)


def command(func):
    """
        Decorator for all commands.
        The decorated function is called with (self, all, other, arguments, ...)
    """
    @wraps(func)
    def decorator(*args):
        return func(args[0], *args[1:])
    func._command = True
    decorator.func = func
    return decorator


def commandClass(cls):
    """
        Decorator for the class. Sets the self.cmddict of the class
        to all functions decorated with @command
    """
    cls.cmddict = {}
    for attr, value in cls.__dict__.items():
        if getattr(value, 'func', False):
            if getattr(value.func, '_command', False):
                cls.cmddict[value.func.__name__] = value
    return cls


@commandClass
class Commandline(object):
    """
        If you want to create a new commandline function you just need to
        create a function with the name of it in this class and decorate
        it with the decorator @command. If you don't want to call the desired
        function (wrong arguments etc) just raise ArgumentError with or
        without an error message.
    """
    def __init__(self):
        self.logger = Logger()
        try:
            self.emcore = libemcore.Emcore(logger = self.logger)
        except libemcore.DeviceNotFoundError:
            self.logger.error("No emCORE device found!\n")
            exit(1)
        self.getinfo("version")
    
    def _parsecommand(self, func, args):
        # adds self to the commandline args.
        # this is needed because the functions need access to their class.
        args.insert(0, self)
        if func in self.cmddict:
            try:
                self.cmddict[func](*args)
            except (ArgumentError, libemcore.ArgumentError) as e:
                usage(e, specific=func)
            except (ArgumentError, libemcore.ArgumentError):
                usage("Syntax Error in function '" + func + "'", specific=func)
            except ArgumentTypeError as e:
                usage(e, specific=func)
            except NotImplementedError:
                self.logger.error("This function is not implemented yet!\n")
            except libemcore.DeviceError as e:
                self.logger.error(str(e) + "\n")
            except TypeError as e:
                # Only act on TypeErrors for the function we called, not on TypeErrors raised by another function.
                if str(e).split(" ", 1)[0] == func + "()":
                    self.logger.error(usage("Argument Error in '%s': Wrong argument count" % func, specific=func))
                else:
                    raise
            except libemcore.usb.core.USBError:
                self.logger.error("There is a problem with the USB connection.\n")
        else:
            usage("No such command!", docstring = False)

    
    @command
    def help(self):
        """ Displays this help """
        usage(docstring = True)
    
    @command
    def getinfo(self, infotype):
        """
            Get info on the running emCORE.
            <infotype> may be either of 'version', 'packetsize', 'mallocpoolbounds'.
        """
        if infotype == "version":
            hwtype = gethwname(self.emcore.lib.dev.hwtypeid)
            self.logger.info("Connected to %s v%d.%d.%d r%d running on %s\n" % (
                             libemcoredata.swtypes[self.emcore.lib.dev.swtypeid],
                             self.emcore.lib.dev.version.majorv,
                             self.emcore.lib.dev.version.minorv,
                             self.emcore.lib.dev.version.patchv,
                             self.emcore.lib.dev.version.revision,
                             hwtype))
        
        elif infotype == "packetsize":
            self.logger.info("Maximum packet sizes:\n")
            self.logger.info("command out: %d\n" % self.emcore.lib.dev.packetsizelimit.cout, 4)
            self.logger.info("command in: %d\n" % self.emcore.lib.dev.packetsizelimit.cin, 4)
            self.logger.info("data out: %d\n" % self.emcore.lib.dev.packetsizelimit.dout, 4)
            self.logger.info("data in: %d\n" % self.emcore.lib.dev.packetsizelimit.din, 4)
        
        elif infotype == "mallocpoolbounds":
            resp = self.emcore.getmallocpoolbounds()
            self.logger.info("The malloc pool is 0x%X - 0x%X" % (
                             self.emcore.lib.dev.mallocpool.lower,
                             self.emcore.lib.dev.mallocpool.upper - 1))
        
        else:
            raise ArgumentTypeError("one out of 'version', 'packetsize', 'mallocpoolbounds'", infotype)
    
    @command
    def reset(self, force=False):
        """
            Resets the device"
            If [force] is 1, the reset will be forced, otherwise it will be gracefully,
            which may take some time.
        """
        force = to_bool(force)
        if force: self.logger.info("Resetting forcefully...\n")
        else: self.logger.info("Resetting...\n")
        self.emcore.reset(force)
    
    @command
    def poweroff(self, force=False):
        """
            Powers the device off
            If [force] is 1, the poweroff will be forced, otherwise it will be gracefully,
            which may take some time.
        """
        force = to_bool(force)
        if force: self.logger.info("Powering off forcefully...\n")
        else: self.logger.info("Powering off...\n")
        self.emcore.poweroff(force)
    
    @command
    def uploadfile(self, filename, addr = None):
        """
            Uploads a file to the device
            <filename>: The path to the file
            [addr]: The address to upload the file to. Allocates a chunk of memory if not given.
        """
        addr = to_int(addr)
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        if addr is not None:
            self.logger.info("Writing file '%s' to memory at 0x%X...\n" % (filename, addr))
        else:
            self.logger.info("Writing file '%s' to an allocated memory region...\n" % filename)
        with f:
            if addr is not None:
                self.emcore.write(addr, f.read())
            else:
                addr = self.emcore.upload(f.read())
            size = f.tell()
        f.close()
        self.logger.info("Done uploading %d bytes to 0x%X\n" % (size, addr))
        return addr, size
    
    @command
    def downloadfile(self, addr, size, filename):
        """
            Downloads a region of memory from the device to a file
            <addr>: the address to download the data from
            <size>: the number of bytes to be read
            <filename>: the path to the file
        """
        addr = to_int(addr)
        size = to_int(size)
        try:
            f = open(filename, 'wb')
        except IOError:
            raise ArgumentError("Can not open file for write!")
        self.logger.info("Reading data from address 0x%X with the size 0x%X to '%s'..." %
                        (addr, size, filename))
        with f:
            f.write(self.emcore.read(addr, size))
        f.close()
        self.logger.info("done\n")
    
    @command
    def hexdump(self, addr, size, width = 16, wordsize = 1, separate = 4, align = False, \
                relative = False, ascii = True, asciisep = 8, asciirep = ".", zeropad = True):
        """
            Downloads a region of memory from the device and hexdumps it
            <addr>: the address to download the data from
            <size>: the number of bytes to be dumped
            [width]: the number of words per output line
            [wordsize]: the number of bytes per word (little endian)
            [separate]: add an additional space character every [separate] words
            [align]: if true, output lines are aligned to the line size
            [relative]: if true, the addresses displayed are relative to the <addr>, not zero
            [ascii]: add ASCII representations of the bytes to the output
            [asciisep]: add an additional space character every [asciisep] ASCII characters
            [asciirep]: replacement character for non-printable ASCII characters
            [zeropad]: pad hex values with zero instead of space characters
        """
        addr = to_int(addr)
        size = to_int(size)
        width = to_int(width)
        wordsize = to_int(wordsize)
        separate = to_int(separate)
        align = to_bool(align)
        relative = to_bool(relative)
        ascii = to_bool(ascii)
        asciisep = to_int(asciisep)
        zeropad = to_bool(zeropad)
        if addr % wordsize != 0: raise ArgumentError("Address is not word aligned!")
        if size % wordsize != 0: raise ArgumentError("Size is not word aligned!")
        if align: skip = addr % (wordsize * width)
        else: skip = 0
        if relative: base = 0
        else: base = addr
        data = self.emcore.read(addr, size)
        for r in range(-skip, size, wordsize * width):
            sys.stdout.write("%08X:" % (base + r))
            for i in range(r, r + wordsize * width, wordsize):
                if i - r > 0 and (i - r) % (separate * wordsize) == 0: sys.stdout.write(" ")
                if i >= 0 and i < size:
                    w = 0
                    for b in range(wordsize):
                        w = w | (struct.unpack("B", data[i + b : i + b + 1])[0] << (8 * b))
                    sys.stdout.write(((" %%0%dX" if zeropad else " %%%dX") % (wordsize * 2)) % w)
                else: sys.stdout.write(" " * (wordsize * 2 + 1))
            if ascii:
                sys.stdout.write(" |")
                for i in range(r, r + wordsize * width):
                    if i - r > 0 and (i - r) % asciisep == 0: sys.stdout.write(" ")
                    if i >= 0 and i < size:
                        if ord(data[i : i + 1]) > 0x1f and ord(data[i : i + 1]) < 0x80:
                            sys.stdout.write(data[i : i + 1].decode("latin1"))
                        else: sys.stdout.write(asciirep)
                    else: sys.stdout.write(" ")
                sys.stdout.write("|")
            sys.stdout.write("\n")
    
    @command
    def uploadbyte(self, addr, byte):
        """
            Uploads a single byte to the device
            <addr>: the address to upload the byte to
            <byte>: the byte to upload
        """
        addr = to_int(addr)
        byte = to_int(byte)
        if byte > 0xFF:
            raise ArgumentError("Specified integer too long")
        data = struct.pack("B", byte)
        self.emcore.write(addr, data)
        self.logger.info("Byte '0x%X' written successfully to 0x%X\n" % (byte, addr))
    
    @command
    def downloadbyte(self, addr):
        """
            Downloads a single byte from the device and prints it to the console window
            <addr>: the address to download the byte from
        """
        addr = to_int(addr)
        data = self.emcore.read(addr, 1)
        byte = struct.unpack("B", data)[0]
        self.logger.info("Read '0x%X' from address 0x%X\n" % (byte, addr))
    
    @command
    def uploadint(self, addr, integer):
        """
            Uploads a single integer to the device
            <addr>: the address to upload the integer to
            <integer>: the integer to upload
        """
        addr = to_int(addr)
        integer = to_int(integer)
        if integer > 0xFFFFFFFF:
            raise ArgumentError("Specified integer too long")
        data = struct.pack("<I", integer)
        self.emcore.write(addr, data)
        self.logger.info("Integer '0x%X' written successfully to 0x%X\n" % (integer, addr))
    
    @command
    def downloadint(self, addr):
        """
            Downloads a single integer from the device and prints it to the console window
            <addr>: the address to download the integer from
        """
        addr = to_int(addr)
        data = self.emcore.read(addr, 4)
        integer = struct.unpack("<I", data)[0]
        self.logger.info("Read '0x%X' from address 0x%X\n" % (integer, addr))
    
    @command
    def i2cread(self, bus, slave, addr, size):
        """
            Reads data from an I2C device
            <bus>: the bus index
            <slave>: the slave address
            <addr>: the start address on the I2C device
            <size>: the number of bytes to read
        """
        bus = to_int(bus)
        slave = to_int(slave)
        addr = to_int(addr)
        size = to_int(size)
        data = self.emcore.i2cread(bus, slave, addr, size)
        bytes = struct.unpack("<%dB" % len(data), data)
        self.logger.info("Data read from I2C:\n")
        for index, byte in enumerate(bytes):
            self.logger.info("%02X: %02X\n" % (addr + index, byte))
    
    @command
    def i2cwrite(self, bus, slave, addr, *args):
        """
            Writes data to an I2C device
            <bus>: the bus index
            <slave>: the slave address
            <addr>: the start address on the I2C device
            <db1> ... <dbN>: the data in single bytes,
                seperated by whitespaces, eg. 37 5A 4F EB
        """
        bus = to_int(bus)
        slave = to_int(slave)
        addr = to_int(addr)
        data = b""
        for arg in args:
            data += chr(to_int(arg))
        self.logger.info("Writing data to I2C...\n")
        self.emcore.i2cwrite(bus, slave, addr, data)
        self.logger.info("done\n")
    
    @command
    def console(self):
        """
            Reads data from the USB console continuously
        """
        size = 48
        while True:
            resp = self.emcore.usbcread(size)
            self.logger.write(resp.data, target = "stdout")
            size = max(48, min(len(resp.data), size) + resp.queuesize)
            if size < 0x800:
                time.sleep(0.1 / 0x800 * (0x800 - size))
    
    @command
    def writeusbconsole(self, *args):
        """
            Writes the string <db1> ... <dbN> to the USB console.
        """
        text = ""
        for word in args:
            text += word + " "
        text = text[:-1]
        self.logger.info("Writing '%s' to the usb console\n" % text)
        self.emcore.usbcwrite(text)
    
    @command
    def readdevconsole(self, bitmask):
        """
            Reads data continuously from one or more of the device's consoles.
            <bitmask>: the bitmask of the consoles to read from.
        """
        bitmask = to_int(bitmask)
        while True:
            resp = self.emcore.cread()
            self.logger.write(resp.data)
            time.sleep(0.1 / resp.maxsize * (resp.maxsize - len(resp.data)))
    
    @command
    def writedevconsole(self, bitmask, *args):
        """
            Writes the string <db1> ... <dbN> to one or more of the device's consoles.
            <bitmask>: the bitmask of the consoles to write to
        """
        bitmask = to_int(bitmask)
        text = ""
        for word in args:
            text += word + " "
        text = text[:-1]
        self.logger.info("Writing '%s' to the device consoles identified with 0x%X\n" % (text, bitmask))
        self.emcore.cwrite(text, bitmask)
    
    @command
    def flushconsolebuffers(self, bitmask):
        """
            flushes one or more of the device consoles' buffers.
            <bitmask>: the bitmask of the consoles to be flushed
        """
        bitmask = to_int(bitmask)
        self.logger.info("Flushing consoles identified with the bitmask 0x%X\n" % bitmask)
        self.emcore.cflush(bitmask)
    
    @command
    def getprocinfo(self):
        """
            Fetches data on the currently running processes
        """
        import datetime
        threads = self.emcore.getprocinfo()
        threadload = 0
        idleload = 0
        for thread in threads:
            if thread.id != 0:
                threadload += thread.cpuload / 255.
            else:
                idleload += thread.cpuload / 255.
        coreload = 1 - (threadload + idleload)
        cpuload = threadload + coreload
        self.logger.info("Threads: %d, CPU load: %.1f%%, kernel load: %.1f%%, user load: %.1f%%\n\n"
                         % (len(threads), cpuload * 100, coreload * 100, threadload * 100))
        self.logger.info("Thread dump:\n")
        for thread in threads:
            self.logger.info("%s:\n" % thread.name, 2)
            self.logger.info("Threadstruct address: 0x%X\n" % thread.addr, 4)
            self.logger.info("Thread type: %s\n" % thread.thread_type, 4)
            self.logger.info("Thread state: %s\n" % thread.state, 4)
            if thread.block_type != "THREAD_NOT_BLOCKED":
                self.logger.info("Block type: %s\n" % thread.block_type, 4)
                if thread.block_type == "THREAD_BLOCK_MUTEX":
                    self.logger.info("Blocked by mutex: 0x%X\n" % thread.blocked_by, 6)
                    self.logger.info("Owner: %s (0x%X)\n" % (thread.blocked_by.owner.name, thread.blocked_by.owner), 8)
                elif thread.block_type == "THREAD_BLOCK_WAKEUP":
                    self.logger.info("Blocked by wakeup: 0x%X\n" % thread.blocked_by, 6)
            self.logger.info("Priority: %d/255\n" % thread.priority, 4)
            self.logger.info("Current CPU load: %.1f%%\n" % ((thread.cpuload * 100) / 255.), 4)
            self.logger.info("CPU time (total): %s\n" % datetime.timedelta(microseconds = thread.cputime_total), 4)
            self.logger.info("Stack address: 0x%X\n" % thread.stack, 4)
            self.logger.info("Registers:\n", 4)
            for registerrange in range(4):
                for register in range(registerrange, 16, 4):
                    registerrepr = "r"+str(register)
                    self.logger.info("{0:>3s}: 0x{1:08X}   ".format(registerrepr, thread.regs[register]), 5)
                self.logger.info("\n")
            self.logger.info("cpsr: 0x{0:08X}".format(thread.cpsr), 6)
            self.logger.info("\n")
    
    @command
    def lockscheduler(self):
        """
            Locks (freezes) the scheduler
        """
        self.logger.info("Will now lock scheduler\n")
        self.emcore.lockscheduler()
    
    @command
    def unlockscheduler(self):
        """
            Unlocks (unfreezes) the scheduler
        """
        self.logger.info("Will now unlock scheduler\n")
        self.emcore.unlockscheduler()
    
    @command
    def suspendthread(self, threadaddr):
        """
            Suspends the thread with the thread address <threadaddr>
        """
        threadaddr = to_int(threadaddr)
        self.logger.info("Suspending the thread with the threadaddr 0x%X\n" % threadaddr)
        self.emcore.suspendthread(threadaddr)
    
    @command
    def resumethread(self, threadaddr):
        """
            Resumes the thread with the thread address <threadaddr>
        """
        threadaddr = to_int(threadaddr)
        self.logger.info("Resuming the thread with the threadaddr 0x%X\n" % threadaddr)
        self.emcore.resumethread(threadaddr)
    
    @command
    def killthread(self, threadaddr):
        """
            Kills the thread with the thread address <threadaddr>
        """
        threadaddr = to_int(threadaddr)
        self.logger.info("Killing the thread with the threadaddr 0x%X\n" % threadaddr)
        self.emcore.killthread(threadaddr)
    
    @command
    def createthread(self, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state):
        """
            Creates a new thread and returns its thread pointer
            <namepointer>: a pointer to the thread's name
            <entrypoint>: a pointer to the entrypoint of the thread
            <stackpointer>: a pointer to the stack of the thread
            <stacksize>: the size of the thread's stack
            <threadtype>: the thread type, vaild are: 0 => user thread, 1 => system thread
            <priority>: the priority of the thread, from 1 to 255
            <state>: the thread's initial state, valid are: 1 => ready, 0 => suspended
        """
        nameptr = to_int(nameptr)
        entrypoint = to_int(entrypoint)
        stackptr = to_int(stackptr)
        stacksize = to_int(stacksize)
        priority = to_int(priority)
        data = self.emcore.createthread(nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state)
        name = self.emcore.readstring(nameptr)
        self.logger.info("Created a thread with the thread pointer 0x%X, the name \"%s\", the entrypoint at 0x%X," \
                         "the stack at 0x%X with a size of 0x%X and a priority of %d/255\n" %
                        (data.threadptr, name, entrypoint, stackptr, stacksize, priority))
    
    @command
    def run(self, filename):
        """
            Uploads the emCORE application <filename> to
            the memory and executes it
        """
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        with f:
            data = self.emcore.run(f.read())
        self.logger.info("Executed emCORE application as thread 0x%X\n" % data.thread)
    
    @command
    def execimage(self, addr):
        """
            Executes the emCORE application at <addr>.
        """
        addr = to_int(addr)
        self.logger.info("Starting emCORE app at 0x%X\n" % addr)
        self.emcore.execimage(addr)
    
    @command
    def flushcaches(self):
        """
            Flushes the CPUs data and instruction caches.
        """
        self.logger.info("Flushing CPU data and instruction caches...")
        self.emcore.flushcaches()
        self.logger.info("done\n")
    
    @command
    def readbootflash(self, addr_flash, addr_mem, size):
        """
            Reads <size> bytes from bootflash to memory.
            <addr_bootflsh>: the address in bootflash to read from
            <addr_mem>: the address in memory to copy the data to
        """
        addr_flash = to_int(addr_flash)
        addr_mem = to_int(addr_mem)
        size = to_int(size)
        self.logger.info("Dumping boot flash from 0x%X - 0x%X to 0x%X - 0x%X\n" %
                        (addr_flash, addr_flash + size, addr_mem, addr_mem + size))
        self.emcore.bootflashread(addr_mem, addr_flash, size)
    
    @command
    def writebootflash(self, addr_flash, addr_mem, size, force=False):
        """
            Writes <size> bytes from memory to bootflash.
            ATTENTION: Don't call this unless you really know what you're doing!
            This may BRICK your device (unless it has a good recovery option)
            <addr_mem>: the address in memory to copy the data from
            <addr_bootflsh>: the address in bootflash to write to
            [force]: Use this flag to suppress the 10 seconds delay
        """
        addr_flash = to_int(addr_flash)
        addr_mem = to_int(addr_mem)
        size = to_int(size)
        force = to_bool(force)
        self.logger.warn("Writing boot flash from the memory in 0x%X - 0x%X to 0x%X - 0x%X\n" %
                        (addr_mem, addr_mem + size, addr_flash, addr_flash + size))
        if force == False:
            self.logger.warn("If this was not what you intended press Ctrl-C NOW")
            for i in range(10):
                self.logger.info(".")
                time.sleep(1)
            self.logger.info("\n")
        self.emcore.bootflashwrite(addr_mem, addr_flash, size)
    
    @command
    def runfirmware(self, targetaddr, filename):
        """
            Uploads the firmware in <filename>
            to an allocated buffer and executes it at <targetaddr>.
        """
        targetaddr = to_int(targetaddr)
        addr, size = self.uploadfile(filename)
        self.execfirmware(targetaddr, addr, size)
    
    @command
    def execfirmware(self, targetaddr, addr, size):
        """
            Moves the firmware at <addr> with <size> to <targetaddr> and executes it
        """
        targetaddr = to_int(targetaddr)
        addr = to_int(addr)
        size = to_int(size)
        self.logger.info("Running firmware at 0x%X. Bye.\n" % targetaddr)
        self.emcore.execfirmware(targetaddr, addr, size)
    
    @command
    def aesencrypt(self, addr, size, keyindex):
        """
            Encrypts a buffer using a hardware key
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <keyindex>: the index of the key in the crypto unit
        """
        addr = to_int(addr)
        size = to_int(size)
        keyindex = to_int(keyindex)
        self.emcore.aesencrypt(addr, size, keyindex)
    
    @command
    def aesdecrypt(self, addr, size, keyindex):
        """
            Decrypts a buffer using a hardware key
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <keyindex>: the index of the key in the crypto unit
        """
        addr = to_int(addr)
        size = to_int(size)
        keyindex = to_int(keyindex)
        self.emcore.aesdecrypt(addr, size, keyindex)
    
    @command
    def hmac_sha1(self, addr, size, destination):
        """
            Generates a HMAC-SHA1 hash of the buffer
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <destination>: the location where the key will be stored
        """
        addr = to_int(addr)
        size = to_int(size)
        destination = to_int(destination)
        sha1size = 0x14
        self.logger.info("Generating hmac-sha1 hash from the buffer at 0x%X with the size 0x%X and saving it to 0x%X - 0x%X\n" %
                        (addr, size, destination, destination + sha1size))
        self.emcore.hmac_sha1(addr, size, destination)
        self.logger.info("done\n")
        data = self.emcore.read(destination, sha1size)
        hash = ord(data)
        self.logger.info("The generated hash is 0x%X\n" % hash)
    
    @command
    def ipodnano2g_getnandinfo(self):
        """
            Target-specific function: ipodnano2g
            Gathers some information about the NAND chip used
        """
        data = self.emcore.ipodnano2g_getnandinfo()
        self.logger.info("NAND chip type: 0x%X\n" % data["type"])
        self.logger.info("Number of banks: %d\n" % data["banks"])
        self.logger.info("Number of blocks: %d\n" % data["blocks"])
        self.logger.info("Number of user blocks: %d\n" % data["userblocks"])
        self.logger.info("Pages per block: %d\n" % data["pagesperblock"])
    
    @command
    def ipodnano2g_nandread(self, addr, start, count, doecc=True, checkempty=True):
        """
            Target-specific function: ipodnano2g
            Reads data from the NAND chip into memory
            <addr>: the memory location where the data is written to
            <start>: start block
            <count>: block count
            [doecc]: use ecc error correction data
            [checkempty]: set statusflags if pages are empty
        """
        addr = to_int(addr)
        start = to_int(start)
        count = to_int(count)
        doecc = to_bool(doecc)
        checkempty = to_bool(checkempty)
        self.logger.info("Reading 0x%X NAND pages starting at 0x%X to memory at 0x%X..." %
                        (count, start, addr))
        self.emcore.ipodnano2g_nandread(addr, start, count, doecc, checkempty)
        self.logger.info("done\n")
    
    @command
    def ipodnano2g_nandwrite(self, addr, start, count, doecc=True):
        """
            Target-specific function: ipodnano2g
            Writes data to the NAND chip
            <addr>: the memory location where the data is read from
            <start>: start block
            <count>: block count
            [doecc]: create ecc error correction data
        """
        addr = to_int(addr)
        start = to_int(start)
        count = to_int(count)
        doecc = to_bool(doecc)
        self.logger.info("Writing 0x%X NAND pages starting at 0x%X from memory at 0x%X..." %
                        (count, start, addr))
        self.emcore.ipodnano2g_nandwrite(addr, start, count, doecc)
        self.logger.info("done\n")
    
    @command
    def ipodnano2g_nanderase(self, addr, start, count):
        """
            Target-specific function: ipodnano2g
            Erases blocks on the NAND chip and stores the results to memory
            <addr>: the memory location where the results are stored
            <start>: start block
            <count>: block count
        """
        addr = to_int(addr)
        start = to_int(start)
        count = to_int(count)
        self.logger.info("Erasing 0x%X NAND pages starting at 0x%X and logging to 0x%X..." %
                        (count, start, addr))
        self.emcore.ipodnano2g_nanderase(addr, start, count)
        self.logger.info("done\n")
    
    @command
    def ipodnano2g_dumpnand(self, filenameprefix):
        """
            Target-specific function: ipodnano2g
            Dumps the whole NAND chip to four files
            <filenameprefix>: prefix of the files that will be created
        """
        info = self.emcore.ipodnano2g_getnandinfo()
        try:
            buf = self.emcore.memalign(0x10, 0x1088000)
            self.logger.info("Dumping NAND contents...")
            try:
                infofile = open(filenameprefix+"_info.txt", 'wb')
                datafile = open(filenameprefix+"_data.bin", 'wb')
                sparefile = open(filenameprefix+"_spare.bin", 'wb')
                statusfile = open(filenameprefix+"_status.bin", 'wb')
            except IOError:
                raise ArgumentError("Can not open file for writing!")
            infofile.write("NAND chip type: 0x%X\r\n" % info["type"])
            infofile.write("Number of banks: %d\r\n" % info["banks"])
            infofile.write("Number of blocks: %d\r\n" % info["blocks"])
            infofile.write("Number of user blocks: %d\r\n" % info["userblocks"])
            infofile.write("Pages per block: %d\r\n" % info["pagesperblock"])
            for i in range(info["banks"] * info["blocks"] * info["pagesperblock"] / 8192):
                self.logger.info(".")
                self.emcore.ipodnano2g_nandread(buf, i * 8192, 8192, 1, 1)
                datafile.write(self.emcore.read(buf, 0x01000000))
                sparefile.write(self.emcore.read(buf + 0x01000000, 0x00080000))
                statusfile.write(self.emcore.read(buf + 0x01080000, 0x00008000))
            infofile.close()
            datafile.close()
            sparefile.close()
            statusfile.close()
            self.logger.info("done\n")
        finally:
            self.emcore.free(buf)
    
    @command
    def ipodnano2g_restorenand(self, filenameprefix, force=False):
        """
            Target-specific function: ipodnano2g
            Restores the whole NAND chip from <filenameprefix>_data.bin and <filenameprefix>_spare.bin
            [force]: use this flag to suppress the 10 seconds delay
        """
        self.logger.warn("Flashing NAND image %s!\n" % filenameprefix)
        if force == False:
            self.logger.warn("If this was not what you intended press Ctrl-C NOW")
            for i in range(10):
                self.logger.info(".")
                time.sleep(1)
            self.logger.info("\n\n")
        info = self.emcore.ipodnano2g_getnandinfo()
        ppb = info["pagesperblock"]
        banks = info["banks"]
        blocks = info["blocks"]
        try:
            if (os.path.getsize(filenameprefix+"_data.bin") != blocks * banks * ppb * 2048):
                raise ArgumentError("Data file size does not match flash size!")
            if (os.path.getsize(filenameprefix+"_spare.bin") != blocks * banks * ppb * 64):
                raise ArgumentError("Spare file size does not match flash size!")
            datafile = open(filenameprefix+"_data.bin", 'rb')
            sparefile = open(filenameprefix+"_spare.bin", 'rb')
        except IOError:
            raise ArgumentError("Can not open input files!")
        try:
            buf = self.emcore.memalign(0x10, 0x844)
            for block in range(info["blocks"]):
                for bank in range(info["banks"]):
                    self.logger.info("\r    Erasing block %d bank %d         " % (block, bank))
                    self.emcore.ipodnano2g_nanderase(buf, block * banks + bank, 1)
                    rc = struct.unpack("<I", self.emcore.read(buf, 4))[0]
                    if rc != 0: self.logger.info("\rBlock %d bank %d erase failed with RC %08X\n" % (block, bank, rc))
                for page in range(ppb):
                    for bank in range(banks):
                        data = datafile.read(2048) + sparefile.read(64)
                        if data == "\xff" * 2112: continue
                        self.emcore.write(buf, data)
                        self.logger.info("\rProgramming block %d page %d bank %d" % (block, page, bank))
                        self.emcore.ipodnano2g_nandwrite(buf, ((block * ppb) + page) * banks + bank, 1, 0)
                        rc = struct.unpack("<I", self.emcore.read(buf + 2112, 4))[0]
                        if rc != 0: self.logger.info("\rBlock %d bank %d page %d programming failed with RC %08X\n" % (block, bank, page, rc))
        finally:
            self.emcore.free(buf)
        datafile.close()
        sparefile.close()
        self.logger.info("\ndone\n")
    
    @command
    def ipodnano2g_wipenand(self, filename, force=False):
        """
            Target-specific function: ipodnano2g
            Wipes the whole NAND chip and logs the result to a file
            <filename>: location of the log file
            [force]: use this flag to suppress the 10 seconds delay
        """
        self.logger.warn("Wiping the whole NAND chip!\n")
        if force == False:
            self.logger.warn("If this was not what you intended press Ctrl-C NOW")
            for i in range(10):
                self.logger.info(".")
                time.sleep(1)
            self.logger.info("\n")
        try:
            buf = self.emcore.malloc(0x100)
            info = self.emcore.ipodnano2g_getnandinfo()
            self.logger.info("Wiping NAND contents...")
            try:
                statusfile = open(filename, 'wb')
            except IOError:
                raise ArgumentError("Can not open file for writing!")
            for i in range(info["banks"] * info["blocks"] / 64):
                self.logger.info(".")
                self.emcore.ipodnano2g_nanderase(buf, i * 64, 64)
                statusfile.write(self.emcore.read(buf, 0x00000100))
            statusfile.close()
            self.logger.info("done\n")
        finally:
            self.emcore.free(buf)
    
    @command
    def ipodclassic_readbbt(self, filename, tempaddr = None):
        """
            Target-specific function: ipodclassic
            Reads the bad block table from the hard disk to memory at <tempaddr>
            (or an allocated block if not given) and writes it to <filename>
        """
        tempaddr = to_int(tempaddr)
        try:
            f = open(filename, 'wb')
        except IOError:
            raise ArgumentError("File not writable.")
        self.logger.info("Reading bad block table from disk...")
        f.write(self.emcore.ipodclassic_readbbt(tempaddr))
        f.close()
        self.logger.info(" done\n")
    
    @command
    def ipodclassic_writebbt(self, filename, tempaddr = None):
        """
            Target-specific function: ipodclassic
            Uploads the bad block table <filename> to memory at <tempaddr>
            (or an allocated block if not given) and writes it to the hard disk
        """
        tempaddr = to_int(tempaddr)
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        self.logger.info("Writing bad block table to disk...")
        data = self.emcore.ipodclassic_writebbt(f.read(), tempaddr)
        f.close()
        self.logger.info(" done\n")
    
    @command
    def ipodclassic_disablebbt(self):
        """
            Target-specific function: ipodclassic
            Disables the hard disk bad block table, if present
        """
        self.logger.info("Disabling hard disk BBT...")
        data = self.emcore.ipodclassic_disablebbt()
        self.logger.info(" done\n")
    
    @command
    def ipodclassic_reloadbbt(self):
        """
            Target-specific function: ipodclassic
            Reloads the hard disk bad block table, if present
        """
        self.logger.info("Reloading hard disk BBT...")
        data = self.emcore.ipodclassic_reloadbbt()
        self.logger.info(" done\n")
    
    @command
    def getvolumeinfo(self, volume):
        """
            Gathers some information about a storage volume used
            <volume>: volume id
        """
        volume = to_int(volume)
        data = self.emcore.storage_get_info(volume)
        self.logger.info("Sector size: %d\n" % data["sectorsize"])
        self.logger.info("Number of sectors: %d\n" % data["numsectors"])
        self.logger.info("Vendor: %s\n" % data["vendor"])
        self.logger.info("Product: %s\n" % data["product"])
        self.logger.info("Revision: %s\n" % data["revision"])
    
    @command
    def readrawstorage(self, volume, sector, count, addr):
        """
            Reads <count> sectors starting at <sector> from storage <volume> to memory at <addr>.
        """
        volume = to_int(volume)
        sector = to_int(sector)
        count = to_int(count)
        addr = to_int(addr)
        self.logger.info("Reading volume %s sectors %X - %X to %08X..." % (volume, sector, sector + count - 1, addr))
        self.emcore.storage_read_sectors_md(volume, sector, count, addr)
        self.logger.info("done\n")
    
    @command
    def writerawstorage(self, volume, sector, count, addr):
        """
            Writes memory contents at <addr> to <count> sectors starting at <sector> on storage <volume>.
        """
        volume = to_int(volume)
        sector = to_int(sector)
        count = to_int(count)
        addr = to_int(addr)
        self.logger.info("Writing %08X to volume %s sectors %X - %X..." % (addr, volume, sector, sector + count - 1))
        self.emcore.storage_write_sectors_md(volume, sector, count, addr)
        self.logger.info("done\n")
    
    @command
    def readrawstoragefile(self, volume, sector, count, file, buffsize = 0x100000, buffer = None):
        """
            Reads <count> sectors starting at <sector> from storage <volume> to file <file>,
            buffering them in memory at [buffer] in chunks of [buffsize] bytes (both optional).
        """
        volume = to_int(volume)
        sector = to_int(sector)
        count = to_int(count)
        buffsize = to_int(buffsize)
        try:
            f = open(file, 'wb')
        except IOError:
            raise ArgumentError("Could not open local file for writing.")
        try:
            storageinfo = self.emcore.storage_get_info(volume)
            buffsize = min(buffsize, storageinfo.sectorsize * count)
            if buffer is None:
                buffer = self.emcore.memalign(0x10, buffsize)
                malloc = True
            else:
                buffer = to_int(buffer)
                malloc = False
            try:
                self.logger.info("Reading volume %s sectors %X - %X to %s..." % (volume, sector, sector + count - 1, file))
                while count > 0:
                    sectors = min(count, int(buffsize / storageinfo.sectorsize))
                    self.emcore.storage_read_sectors_md(volume, sector, sectors, buffer)
                    f.write(self.emcore.read(buffer, storageinfo.sectorsize * sectors))
                    sector = sector + sectors
                    count = count - sectors
            finally:
                if malloc == True:
                    self.emcore.free(buffer)
        finally:
            f.close()
        self.logger.info("done\n")
    
    @command
    def writerawstoragefile(self, volume, sector, count, file, buffsize = 0x100000, buffer = None):
        """
            Writes contents of <file> to <count> sectors starting at <sector> on storage <volume>,
            buffering them in memory at [buffer] in chunks of [buffsize] bytes (both optional).
            If <count> is -1, the number of sectors will be determined from the file size.
        """
        volume = to_int(volume)
        sector = to_int(sector)
        count = to_int(count)
        buffsize = to_int(buffsize)
        try:
            f = open(file, 'rb')
        except IOError:
            raise ArgumentError("Could not open local file for reading.")
        try:
            storageinfo = self.emcore.storage_get_info(volume)
            if count == -1: count = int((os.path.getsize(file) + storageinfo.sectorsize - 1) / storageinfo.sectorsize)
            buffsize = min(buffsize, storageinfo.sectorsize * count)
            if buffer is None:
                buffer = self.emcore.memalign(0x10, buffsize)
                malloc = True
            else:
                buffer = to_int(buffer)
                malloc = False
            try:
                self.logger.info("Writing %s to volume %s sectors %X - %X..." % (file, volume, sector, sector + count - 1))
                while count > 0:
                    sectors = min(count, int(buffsize / storageinfo.sectorsize))
                    bytes = storageinfo.sectorsize * sectors
                    data = b""
                    while len(data) < bytes:
                       new = f.read(bytes - len(data))
                       data = data + new
                       if len(new) == 0: break
                    self.emcore.write(buffer, data)
                    self.emcore.storage_write_sectors_md(volume, sector, sectors, buffer)
                    sector = sector + sectors
                    count = count - sectors
            finally:
                if malloc == True:
                    self.emcore.free(buffer)
        finally:
            f.close()
        self.logger.info("done\n")
    
    @command
    def mkdir(self, dirname):
        """
            Creates a directory with the name <dirname>
        """
        self.logger.info("Creating directory %s..." % dirname)
        self.emcore.dir_create(dirname)
        self.logger.info(" done\n")
    
    @command
    def rmdir(self, dirname):
        """
            Removes an empty directory with the name <dirname>
        """
        self.logger.info("Removing directory %s..." % dirname)
        self.emcore.dir_remove(dirname)
        self.logger.info(" done\n")
    
    @command
    def rm(self, filename):
        """
            Removes a file with the name <filename>
        """
        self.logger.info("Removing file %s..." % filename)
        self.emcore.file_unlink(filename)
        self.logger.info(" done\n")
    
    @command
    def rmtree(self, path):
        """
            Recursively removes a folder
            <path>: the folder to be removed
        """
        handle = self.emcore.dir_open(path)
        while True:
            try:
                entry = self.emcore.dir_read(handle)
                if entry.name == "." or entry.name == "..": continue
                elif entry.attributes & 0x10:
                    self.rmtree(path + "/" + entry.name)
                else: self.rm(path + "/" + entry.name)
            except: break
        self.emcore.dir_close(handle)
        self.rmdir(path)
    
    @command
    def mv(self, oldname, newname):
        """
            Renames or moves file or directory <oldname> to <newname>
        """
        self.logger.info("Renaming %s to %s..." % (oldname, newname))
        self.emcore.file_rename(oldname, newname)
        self.logger.info(" done\n")
    
    @command
    def get(self, remotename, localname, buffsize = 0x10000, buffer = None):
        """
            Downloads a file
            <remotename>: filename on the device
            <localname>: filename on the computer
            [buffsize]: buffer size (optional)
            [buffer]: buffer address (optional)
        """
        buffsize = to_int(buffsize)
        try:
            f = open(localname, 'wb')
        except IOError:
            raise ArgumentError("Could not open local file for writing.")
        try:
            fd = self.emcore.file_open(remotename, 0)
            try:
                size = self.emcore.file_size(fd)
                buffsize = min(buffsize, size)
                if buffer is None:
                    buffer = self.emcore.memalign(0x10, buffsize)
                    malloc = True
                else:
                    buffer = to_int(buffer)
                    malloc = False
                try:
                    self.logger.info("Downloading file %s to %s..." % (remotename, localname))
                    while size > 0:
                        bytes = self.emcore.file_read(fd, buffsize, buffer).rc
                        f.write(self.emcore.read(buffer, bytes))
                        size = size - bytes
                finally:
                    if malloc == True:
                        self.emcore.free(buffer)
            finally:
                self.emcore.file_close(fd)
        finally:
            f.close()
        self.logger.info(" done\n")
    
    @command
    def gettree(self, remotepath, localpath, buffsize = 0x10000, buffer = None):
        """
            Downloads a directory tree
            <remotepath>: path on the device
            <localpath>: path on the computer
            [buffsize]: buffer size (optional)
            [buffer]: buffer address (optional)
        """
        buffsize = to_int(buffsize)
        handle = self.emcore.dir_open(remotepath)
        try:
            if buffer is None:
                buffer = self.emcore.memalign(0x10, buffsize)
                malloc = True
            else:
                buffer = to_int(buffer)
                malloc = False
            try:
                try: os.mkdir(localpath)
                except: pass
                while True:
                    try:
                        entry = self.emcore.dir_read(handle)
                    except: break
                    if entry.name == "." or entry.name == "..": continue
                    elif entry.attributes & 0x10:
                        self.gettree(remotepath + "/" + entry.name, localpath + "/" + entry.name, buffsize, buffer)
                    else: self.get(remotepath + "/" + entry.name, localpath + "/" + entry.name, buffsize, buffer)
            finally:
                if malloc == True:
                    self.emcore.free(buffer)
        finally:
            self.emcore.dir_close(handle)
    
    @command
    def put(self, localname, remotename, buffsize = 0x10000, buffer = None):
        """
            Uploads a file
            <localname>: filename on the computer
            <remotename>: filename on the device
            [buffsize]: buffer size (optional)
            [buffer]: buffer address (optional)
        """
        buffsize = to_int(buffsize)
        try:
            f = open(localname, 'rb')
        except IOError:
            raise ArgumentError("Could not open local file for reading.")
        try:
            buffsize = min(buffsize, os.path.getsize(localname))
            if buffer is None:
                buffer = self.emcore.memalign(0x10, buffsize)
                malloc = True
            else:
                buffer = to_int(buffer)
                malloc = False
            try:
                self.logger.info("Uploading file %s to %s..." % (localname, remotename))
                fd = self.emcore.file_open(remotename, 0x15)
                try:
                    while True:
                        data = f.read(buffsize)
                        if len(data) == 0: break
                        self.emcore.write(buffer, data)
                        bytes = 0
                        while bytes < len(data):
                            bytes = bytes + self.emcore.file_write(fd, len(data) - bytes, buffer + bytes)
                finally:
                    self.emcore.file_close(fd)
            finally:
                if malloc == True:
                    self.emcore.free(buffer)
        finally:
            f.close()
        self.logger.info(" done\n")
    
    @command
    def puttree(self, localpath, remotepath, buffsize = 0x10000, buffer = None):
        """
            Uploads a directory tree
            <localpath>: path on the computer
            <remotepath>: path on the device
            [buffsize]: buffer size (optional)
            [buffer]: buffer address (optional)
        """
        buffsize = to_int(buffsize)
        if buffer is None:
            buffer = self.emcore.memalign(0x10, buffsize)
            malloc = True
        else:
            buffer = to_int(buffer)
            malloc = False
        try:
            try: self.mkdir(remotepath)
            except: self.logger.info(" failed\n")
            pathlen = len(localpath)
            for d in os.walk(localpath):
                prefix = remotepath + "/" + d[0].replace("\\", "/")[pathlen:] + "/"
                for dir in d[1]:
                    if dir != ".svn":
                        try: self.mkdir(prefix + dir)
                        except: self.logger.info(" failed\n")
                for f in d[2]:
                    if prefix.find("/.svn/") == -1:
                        self.put(d[0] + "/" + f, prefix + f, buffsize, buffer)
        finally:
            if malloc == True:
                self.emcore.free(buffer)
    
    @command
    def ls(self, path = "/"):
        """
            Lists all files in the specified path
            [path]: the path which is listed
        """
        handle = self.emcore.dir_open(path)
        self.logger.info("Directory listing of %s:\n" % path)
        while True:
            try:
                entry = self.emcore.dir_read(handle)
            except: break
            if entry.attributes & 0x10: size = "DIR"
            else: size = locale.format("%d", entry.size, True).rjust(13)
            self.logger.info(entry.name.ljust(50) + " - " + size + "\n")
        self.emcore.dir_close(handle)
    
    @command
    def find(self, path = "/"):
        """
            Lists all files in the specified path, recursively
            [path]: the path which is listed
        """
        handle = self.emcore.dir_open(path)
        self.logger.info(path + "/\n")
        while True:
            try:
                entry = self.emcore.dir_read(handle)
            except: break
            if entry.name == "." or entry.name == "..": continue
            elif entry.attributes & 0x10: self.find(path + "/" + entry.name)
            else: self.logger.info(path + "/" + entry.name + "\n")
        self.emcore.dir_close(handle)
    
    @command
    def malloc(self, size):
        """ Allocates <size> bytes and returns a pointer to the allocated memory """
        size = to_int(size)
        self.logger.info("Allocating %d bytes of memory\n" % size)
        addr = self.emcore.malloc(size)
        self.logger.info("Allocated %d bytes of memory at 0x%X\n" % (size, addr))
    
    @command
    def memalign(self, align, size):
        """ Allocates <size> bytes aligned to <align> and returns a pointer to the allocated memory """
        align = to_int(align)
        size = to_int(size)
        self.logger.info("Allocating %d bytes of memory aligned to 0x%X\n" % (size, align))
        addr = self.emcore.memalign(align, size)
        self.logger.info("Allocated %d bytes of memory at 0x%X\n" % (size, addr))
    
    @command
    def realloc(self, ptr, size):
        """ The size of the memory block pointed to by <ptr> is changed to the <size> bytes,
            expanding or reducing the amount of memory available in the block.
            Returns a pointer to the reallocated memory.
        """
        ptr = to_int(ptr)
        size = to_int(size)
        self.logger.info("Reallocating 0x%X to have the new size %d\n" % (ptr, size))
        addr = self.emcore.realloc(ptr, size)
        self.logger.info("Reallocated memory at 0x%X to 0x%X with the new size %d\n" % (ptr, addr, size))
    
    @command
    def reownalloc(self, ptr, owner):
        """ Changes the owner of the memory allocation <ptr> to the thread struct at addr <owner> """
        ptr = to_int(ptr)
        owner = to_int(owner)
        self.logger.info("Changing owner of the memory region 0x%X to 0x%X\n" % (ptr, owner))
        self.emcore.reownalloc(ptr, owner)
        self.logger.info("Successfully changed owner of 0x%X to 0x%X\n" % (ptr, owner))
    
    @command
    def free(self, ptr):
        """ Frees the memory space pointed to by 'ptr' """
        ptr = to_int(ptr)
        self.logger.info("Freeing the memory region at 0x%X\n" % ptr)
        self.emcore.free(ptr)
        self.logger.info("Successfully freed the memory region at 0x%X\n" % ptr)
    
    @command
    def free_all(self):
        """ Frees all memory allocations created by the monitor thread """
        self.logger.info("Freeing all memory allocations created by the monitor thread\n")
        self.emcore.free_all()
        self.logger.info("Successfully freed all memory allocations created by the monitor thread\n")
    
    @command
    def rtcread(self):
        """ Reads the real time clock on the device """
        import datetime
        self.logger.info("Reading the clock\n")
        dt = self.emcore.rtcread()
        self.logger.info("Successfully read the clock: %s\n" % (dt.ctime()))
            
    @command
    def rtcwrite(self):
        """ Sets the real time clock on the device to the current local time """
        import datetime
        dt = datetime.datetime.now()
        self.logger.info("Setting the clock to: %s\n" % (dt.ctime()))
        self.emcore.rtcwrite(dt)
        self.logger.info("Successfully set the clock\n")
        

if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage("No command specified", docstring = False)
    try:
        interface = Commandline()
        interface._parsecommand(sys.argv[1], sys.argv[2:])
    except KeyboardInterrupt:
        sys.exit()
