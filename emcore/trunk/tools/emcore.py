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
from misc import Error, Logger, getfuncdoc, gethwname


class NotImplementedError(Error):
    pass

class ArgumentError(Error):
    pass

class ArgumentTypeError(Error):
    def __init__(self, expected, seen=False):
        self.expected = expected
        self.seen = seen
    def __str__(self):
        if self.seen:
            return "Expected " + str(self.expected) + " but saw " + str(self.seen)
        else:
            return "Expected " + str(self.expected) + ", but saw something else"


def usage(errormsg=None, specific=False, docstring=True):
    """
        Prints the usage information.
        It is auto generated from various places.
    """
    logger = Logger()
    cmddict = Commandline.cmddict
    doc = getfuncdoc(cmddict)
    if not specific:
        logger.log("Please provide a command and (if needed) parameters as command line arguments\n\n")
        logger.log("Available commands:\n\n")
    else:
        logger.log("\n")
    for function in sorted(doc.items()):
        function = function[0]
        if specific == False or specific == function:
            logger.log(function + " ", 2)
            for arg in doc[function]['args']:
                logger.log("<" + arg + "> ")
            if doc[function]['kwargs']:
                for kwarg, kwvalue in doc[function]['kwargs'].iteritems():
                    logger.log("[" + kwarg + " = " + str(kwvalue) + "] ")
            if doc[function]['varargs']:
                logger.log("<db1> ... <dbN>")
            if docstring and doc[function]['documentation']:
                logger.log("\n")
                logger.log(doc[function]['documentation']+"\n", 4)
            logger.log("\n")
    logger.log("\n")

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
    for attr, value in cls.__dict__.iteritems():
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
            self.emcore = libemcore.Emcore(loglevel = 2)
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
            except (ArgumentError, libemcore.ArgumentError), e:
                usage(e, specific=func)
            except (ArgumentError, libemcore.ArgumentError):
                usage("Syntax Error in function '" + func + "'", specific=func)
            except ArgumentTypeError, e:
                usage(e, specific=func)
            except NotImplementedError:
                self.logger.error("This function is not implemented yet!\n")
            except libemcore.DeviceError, e:
                self.logger.error(str(e) + "\n")
            except TypeError, e:
                # Only act on TypeErrors for the function we called, not on TypeErrors raised by another function.
                if str(e).split(" ", 1)[0] == func + "()":
                    self.logger.error(usage("Argument Error in '" + func + "': Wrong argument count", specific=func) + "\n")
                else:
                    raise
            except libemcore.usb.core.USBError:
                self.logger.error("There is a problem with the USB connection.\n")
        else:
            usage("No such command")
    
    @staticmethod
    def _bool(something):
        """
            Converts quite everything into bool.
        """
        if type(something) == bool:
            return something
        if something is None:
            return False
        elif type(something) == int or type(something) == long:
            return bool(something)
        elif type(something == str):
            truelist = ['true', '1', 't', 'y', 'yes']
            falselist = ['false', '0', 'f', 'n', 'no']
            if something.lower() in truelist:
                return True
            elif something.lower() in falselist:
                return False
        raise ArgumentTypeError("bool", "'"+str(something)+"'")

    @staticmethod
    def _hexint(something):
        """
            Converts quite everything to a hexadecimal represented integer.
            This works for default arguments too, because it returns
            None when it found that it got a NoneType object.
        """
        if type(something) == int or type(something) == long:
            return something
        elif type(something) == str:
            try:
                return int(something, 16)
            except ValueError:
                raise ArgumentTypeError("hexadecimal coded integer", "'"+str(something)+"'")
        elif something is None:
            return None
        else:
            raise ArgumentTypeError("hexadecimal coded integer", "'"+str(something)+"'")
    
    @staticmethod
    def _hex(integer):
        return "0x%x" % integer
    
    @command
    def help(self):
        """ Generates a list of functions """
        usage(None, False, False)
    
    @command
    def getinfo(self, infotype):
        """
            Get info on the running emCORE.
            <infotype> may be either of 'version', 'packetsize', 'usermemrange'.
        """
        if infotype == "version":
            hwtype = gethwname(self.emcore.lib.dev.hwtypeid)
            self.logger.info("Connected to " + \
                             libemcoredata.swtypes[self.emcore.lib.dev.swtypeid] + \
                             " v" + str(self.emcore.lib.dev.version.majorv) + \
                             "." + str(self.emcore.lib.dev.version.minorv) + \
                             "." + str(self.emcore.lib.dev.version.patchv) + \
                             " r" + str(self.emcore.lib.dev.version.revision) + \
                             " running on " + hwtype + "\n")
        
        elif infotype == "packetsize":
            self.logger.info("Maximum packet sizes: \n command out: " + str(self.emcore.lib.dev.packetsizelimit.cout) + \
                             "\n command in: " + str(self.emcore.lib.dev.packetsizelimit.cin) + \
                             "\n data in: " + str(self.emcore.lib.dev.packetsizelimit.din) + \
                             "\n data out: " + str(self.emcore.lib.dev.packetsizelimit.dout)+ "\n")
        
        elif infotype == "usermemrange":
            resp = self.emcore.getusermemrange()
            self.logger.info("The user memory range is " + \
                             self._hex(self.emcore.lib.dev.usermem.lower) + \
                             " - " + \
                             self._hex(self.emcore.lib.dev.usermem.upper - 1) + "\n")
        
        else:
            raise ArgumentTypeError("one out of 'version', 'packetsize', 'usermemrange'", infotype)
    
    @command
    def reset(self, force=False):
        """
            Resets the device"
            If [force] is 1, the reset will be forced, otherwise it will be gracefully,
            which may take some time.
        """
        force = self._bool(force)
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
        force = self._bool(force)
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
        addr = self._hexint(addr)
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        if addr is not None:
            self.logger.info("Writing file '" + filename + \
                            "' to memory at " + self._hex(addr) + "...\n")
        else:
            self.logger.info("Writing file '" + filename + " to an allocated memory region...\n")
        with f:
            if addr is not None:
                self.emcore.write(addr, f.read())
            else:
                addr = self.emcore.upload(f.read())
            size = f.tell()
        f.close()
        self.logger.info("Done uploading " + str(size) + " bytes to 0x" + self._hex(addr) + "\n")
        return addr, size
    
    @command
    def downloadfile(self, addr, size, filename):
        """
            Uploads a file to the device
            <offset>: the address to upload the file to
            <size>: the number of bytes to be read
            <filename>: the path to the file
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        try:
            f = open(filename, 'wb')
        except IOError:
            raise ArgumentError("Can not open file for write!")
        self.logger.info("Reading data from address " + self._hex(addr) + \
                         " with the size " + self._hex(size) + \
                         " to '"+filename+"'...")
        with f:
            f.write(self.emcore.read(addr, size))
        f.close()
        self.logger.info("done\n")

    @command
    def uploadint(self, addr, integer):
        """
            Uploads a single integer to the device
            <addr>: the address to upload the integer to
            <integer>: the integer to upload
        """
        addr = self._hexint(addr)
        integer = self._hexint(integer)
        if integer > 0xFFFFFFFF:
            raise ArgumentError("Specified integer too long")
        data = struct.pack("I", integer)
        self.emcore.write(addr, data)
        self.logger.info("Integer '" + self._hex(integer) + \
                         "' written successfully to " + self._hex(addr) + "\n")

    @command
    def downloadint(self, addr):
        """
            Downloads a single integer from the device and prints it to the console window
            <addr>: the address to download the integer from
        """
        addr = self._hexint(addr)
        data = self.emcore.read(addr, 4)
        integer = struct.unpack("I", data)[0]
        self.logger.info("Integer '" + self._hex(integer) + \
                         "' read from address " + self._hex(addr) + "\n")

    @command
    def i2cread(self, bus, slave, addr, size):
        """
            Reads data from an I2C device
            <bus>: the bus index
            <slave>: the slave address
            <addr>: the start address on the I2C device
            <size>: the number of bytes to read
        """
        bus = self._hexint(bus)
        slave = self._hexint(slave)
        addr = self._hexint(addr)
        size = self._hexint(size)
        data = self.emcore.i2cread(bus, slave, addr, size)
        bytes = struct.unpack("%dB" % len(data), data)
        self.logger.info("Data read from I2C:\n")
        for index, byte in enumerate(bytes):
            self.logger.info("%02X: %02X\n" % (index, byte))

    @command
    def i2cwrite(self, bus, slave, addr, *args):
        """
            Writes data to an I2C device
            <bus>: the bus index
            <slave>: the slave address
            <addr>: the start address on the I2C device
            <db1> ... <dbN>: the data in single bytes, encoded in hex,
                seperated by whitespaces, eg. 37 5A 4F EB
        """
        bus = self._hexint(bus)
        slave = self._hexint(slave)
        addr = self._hexint(addr)
        data = ""
        for arg in args:
            data += chr(self._hexint(arg))
        self.logger.info("Writing data to I2C...\n")
        self.emcore.i2cwrite(bus, slave, addr, data)
        self.logger.info("done\n")

    @command
    def console(self):
        """
            Reads data from the USB console continuously
        """
        while True:
            resp = self.emcore.usbcread()
            self.logger.log(resp.data)
            time.sleep(0.1 / resp.maxsize * (resp.maxsize - len(resp.data)))

    @command
    def writeusbconsole(self, *args):
        """
            Writes the string <db1> ... <dbN> to the USB console.
        """
        text = ""
        for word in args:
            text += word + " "
        text = text[:-1]
        self.logger.info("Writing '"+ text +"' to the usb console\n")
        self.emcore.usbcwrite(text)

    @command
    def readdevconsole(self, bitmask):
        """
            Reads data continuously from one or more of the device's consoles.
            <bitmask>: the bitmask of the consoles to read from.
        """
        bitmask = self._hexint(bitmask)
        while True:
            resp = self.emcore.cread()
            self.logger.log(resp.data)
            time.sleep(0.1 / resp.maxsize * (resp.maxsize - len(resp.data)))
    
    @command
    def writedevconsole(self, bitmask, *args):
        """
            Writes the string <db1> ... <dbN> to one or more of the device's consoles.
            <bitmask>: the bitmask of the consoles to write to
        """
        bitmask = self._hexint(bitmask)
        text = ""
        for word in args:
            text += word + " "
        text = text[:-1]
        self.logger.info("Writing '" + text + \
                         "' to the device consoles identified with " + self._hex(bitmask) + "\n")
        self.emcore.cwrite(text, bitmask)

    @command
    def flushconsolebuffers(self, bitmask):
        """
            flushes one or more of the device consoles' buffers.
            <bitmask>: the bitmask of the consoles to be flushed
        """
        bitmask = self._hexint(bitmask)
        self.logger.info("Flushing consoles identified with the bitmask " + \
                         self._hex(bitmask) + "\n")
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
            self.logger.info(thread.name+":\n", 2)
            self.logger.info("Thread id: "      + str(thread.id)+"\n", 4)
            self.logger.info("Thread type: "    + thread.type+"\n", 4)
            self.logger.info("Thread state: "   + thread.state+"\n", 4)
            self.logger.info("Block type: "     + thread.block_type+"\n", 4)
            self.logger.info("Blocked by: "     + self._hex(thread.blocked_by_ptr)+"\n", 4)
            self.logger.info("Priority: "       + str(thread.priority)+"/255\n", 4)
            self.logger.info("Current CPU load: %.1f%%\n" % ((thread.cpuload * 100) / 255.), 4)
            self.logger.info("CPU time (total): "+str(datetime.timedelta(microseconds = thread.cputime_total))+"\n", 4)
            self.logger.info("Stack address: "  + self._hex(thread.stackaddr)+"\n", 4)
            self.logger.info("Registers:\n", 4)
            for registerrange in range(4):
                self.logger.info("      ")
                for register in range(registerrange, 16, 4):
                    registerrepr = "r"+str(register)
                    self.logger.info("{0:3s}: 0x{1:08X}   ".format(registerrepr, thread.regs["r"+str(register)]))
                self.logger.info("\n")
            self.logger.info("cpsr: 0x{0:08X}".format(thread.regs.cpsr), 6)
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
    def suspendthread(self, threadid):
        """
            Suspends/resumes the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.logger.info("Suspending the thread with the threadid "+self._hex(threadid)+"\n")
        self.emcore.suspendthread(threadid)

    @command
    def resumethread(self, threadid):
        """
            Resumes the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.logger.info("Resuming the thread with the threadid "+self._hex(threadid)+"\n")
        self.emcore.resumethread(threadid)

    @command
    def killthread(self, threadid):
        """
            Kills the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.logger.info("Killing the thread with the threadid " + self._hex(threadid) + "\n")
        self.emcore.killthread(threadid)

    @command
    def createthread(self, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state):
        """
            Creates a new thread and returns its thread ID
            <namepointer>: a pointer to the thread's name
            <entrypoint>: a pointer to the entrypoint of the thread
            <stackpointer>: a pointer to the stack of the thread
            <stacksize>: the size of the thread's stack
            <type>: the thread type, vaild are: 0 => user thread, 1 => system thread
            <priority>: the priority of the thread, from 1 to 255
            <state>: the thread's initial state, valid are: 1 => ready, 0 => suspended
        """
        nameptr = self._hexint(nameptr)
        entrypoint = self._hexint(entrypoint)
        stackpointer = self._hexint(stackpointer)
        stacksize = self._hexint(stacksize)
        priority = self._hexint(priority)
        data = self.emcore.createthread(nameptr, entrypoint, stackptr, stacksize, type, priority, state)
        name = self.emcore.readstring(nameptr)
        self.logger.info("Created a thread with the threadid " + data.id + \
                         ", the name \"" + name + "\"" + \
                         ", the entrypoint at " + self._hex(entrypoint) + \
                         ", the stack at " + self._hex(stackpointer) + \
                         " with a size of " + self._hex(stacksize) + \
                         " and a priority of " + self._hex(priority) + "\n")
    
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
        self.logger.info("Executed emCORE application as thread " + self._hex(data.thread) + "\n")

    @command
    def execimage(self, addr):
        """
            Executes the emCORE application at <addr>.
        """
        addr = self._hexint(addr)
        self.logger.info("Starting emCORE app at "+self._hex(addr)+"\n")
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
        addr_flash = self._hexint(addr_flash)
        addr_mem = self._hexint(addr_mem)
        size = self._hexint(size)
        self.logger.info("Dumping boot flash addresses "+self._hex(addr_flash)+" - "+
                         hex(addr_flash+size)+" to "+self._hex(addr_mem)+" - "+self._hex(addr_mem+size)+"\n")
        self.emcore.bootflashread(addr_mem, addr_flash, size)
    
    @command
    def writebootflash(self, addr_flash, addr_mem, size, force=False):
        """
            Writes <size> bytes from memory to bootflash.
            ATTENTION: Don't call this unless you really know what you're doing!
            This may BRICK your device (unless it has a good recovery option)
            <addr_mem>: the address in memory to copy the data from
            <addr_bootflsh>: the address in bootflash to write to
            [force]: Use this flag to suppress the 5 seconds delay
        """
        addr_flash = self._hexint(addr_flash)
        addr_mem = self._hexint(addr_mem)
        size = self._hexint(size)
        force = self._bool(force)
        self.logger.warn("Writing boot flash from the memory in "+self._hex(addr_mem)+" - "+
                         hex(addr_mem+size)+" to "+self._hex(addr_flash)+" - "+self._hex(addr_flash+size)+"\n")
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
        targetaddr = self._hexint(targetaddr)
        addr, size = self.uploadfile(filename)
        self.execfirmware(targetaddr, addr, size)
    
    @command
    def execfirmware(self, targetaddr, addr, size):
        """
            Moves the firmware at <addr> with <size> to <targetaddr> and executes it
        """
        addr = self._hexint(addr)
        self.logger.info("Running firmware at "+self._hex(targetaddr)+". Bye.\n")
        self.emcore.execfirmware(targetaddr, addr, size)
    
    @command
    def aesencrypt(self, addr, size, keyindex):
        """
            Encrypts a buffer using a hardware key
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <keyindex>: the index of the key in the crypto unit
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        keyindex = self._hexint(keyindex)
        self.emcore.aesencrypt(addr, size, keyindex)
    
    @command
    def aesdecrypt(self, addr, size, keyindex):
        """
            Decrypts a buffer using a hardware key
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <keyindex>: the index of the key in the crypto unit
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        keyindex = self._hexint(keyindex)
        self.emcore.aesdecrypt(addr, size, keyindex)
    
    @command
    def hmac_sha1(self, addr, size, destination):
        """
            Generates a HMAC-SHA1 hash of the buffer
            <addr>: the starting address of the buffer
            <size>: the size of the buffer
            <destination>: the location where the key will be stored
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        destination = self._hexint(destination)
        sha1size = 0x14
        self.logger.info("Generating hmac-sha1 hash from the buffer at " + self._hex(addr) + \
                         " with the size " + self._hex(size) + " and saving it to " + \
                         self._hex(destination) + " - " + self._hex(destination+sha1size) + "...")
        self.emcore.hmac_sha1(addr, size, destination)
        self.logger.info("done\n")
        data = self.emcore.read(destination, sha1size)
        hash = ord(data)
        self.logger.info("The generated hash is "+self._hex(hash)+"\n")

    @command
    def ipodnano2g_getnandinfo(self):
        """
            Target-specific function: ipodnano2g
            Gathers some information about the NAND chip used
        """
        data = self.emcore.ipodnano2g_getnandinfo()
        self.logger.info("NAND chip type: "         + self._hex(data["type"])+"\n")
        self.logger.info("Number of banks: "        + str(data["banks"])+"\n")
        self.logger.info("Number of blocks: "       + str(data["blocks"])+"\n")
        self.logger.info("Number of user blocks: "  + str(data["userblocks"])+"\n")
        self.logger.info("Pages per block: "        + str(data["pagesperblock"])+"\n")

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
        addr = self._hexint(addr)
        start = self._hexint(start)
        count = self._hexint(count)
        doecc = self._bool(doecc)
        checkempty = self._bool(checkempty)
        self.logger.info("Reading " + self._hex(count) + " NAND pages starting at " + \
                         self._hex(start) + " to " + self._hex(addr) + "...")
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
        addr = self._hexint(addr)
        start = self._hexint(start)
        count = self._hexint(count)
        doecc = self._bool(doecc)
        self.logger.info("Writing " + self._hex(count) + " NAND pages starting at " + \
                         self._hex(start) + " from " + self._hex(addr) + "...")
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
        addr = self._hexint(addr)
        start = self._hexint(start)
        count = self._hexint(count)
        self.logger.info("Erasing " + self._hex(count) + " NAND blocks starting at " + \
                         self._hex(start) + " and logging to " + self._hex(addr) + "...")
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
        self.logger.info("Dumping NAND contents...")
        try:
            infofile = open(filenameprefix+"_info.txt", 'wb')
            datafile = open(filenameprefix+"_data.bin", 'wb')
            sparefile = open(filenameprefix+"_spare.bin", 'wb')
            statusfile = open(filenameprefix+"_status.bin", 'wb')
        except IOError:
            raise ArgumentError("Can not open file for writing!")
        infofile.write("NAND chip type: "       + self._hex(info["type"]) + "\r\n")
        infofile.write("Number of banks: "      + str(info["banks"]) + "\r\n")
        infofile.write("Number of blocks: "     + str(info["blocks"]) + "\r\n")
        infofile.write("Number of user blocks: "+ str(info["userblocks"]) + "\r\n")
        infofile.write("Pages per block: "      + str(info["pagesperblock"]) + "\r\n")
        for i in range(info["banks"] * info["blocks"] * info["pagesperblock"] / 8192):
            self.logger.info(".")
            self.emcore.ipodnano2g_nandread(0x08000000, i * 8192, 8192, 1, 1)
            datafile.write(self.emcore.read(0x08000000, 0x01000000))
            sparefile.write(self.emcore.read(0x09000000, 0x00080000))
            statusfile.write(self.emcore.read(0x09080000, 0x00008000))
        infofile.close()
        datafile.close()
        sparefile.close()
        statusfile.close()
        self.logger.info("done\n")

    @command
    def ipodnano2g_wipenand(self, filename, force=False):
        """
            Target-specific function: ipodnano2g
            Wipes the whole NAND chip and logs the result to a file
            <filename>: location of the log file
            [force]: use this flag to suppress the 5 seconds delay
        """
        self.logger.warn("Wiping the whole NAND chip!\n")
        if force == False:
            self.logger.warn("If this was not what you intended press Ctrl-C NOW")
            for i in range(10):
                self.logger.info(".")
                time.sleep(1)
            self.logger.info("\n")
        info = self.emcore.ipodnano2g_getnandinfo()
        self.logger.info("Wiping NAND contents...")
        try:
            statusfile = open(filename, 'wb')
        except IOError:
            raise ArgumentError("Can not open file for writing!")
        for i in range(info["banks"] * info["blocks"] / 64):
            self.logger.info(".")
            self.emcore.ipodnano2g_nanderase(0x08000000, i * 64, 64)
            statusfile.write(self.emcore.read(0x08000000, 0x00000100))
        statusfile.close()
        self.logger.info("done\n")

    @command
    def ipodclassic_writebbt(self, tempaddr, filename):
        """
            Target-specific function: ipodclassic
            Uploads the bad block table <filename> to
            memory at <tempaddr> and writes it to the hard disk
        """
        tempaddr = self._hexint(tempaddr)
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        self.logger.info("Writing bad block table to disk...")
        data = self.emcore.ipodclassic_writebbt(f.read(), tempaddr)
        f.close()
        self.logger.info(" done\n")

    @command
    def getvolumeinfo(self, volume):
        """
            Gathers some information about a storage volume used
            <volume>: volume id
        """
        volume = self._hexint(volume)
        data = self.emcore.storage_get_info(volume)
        self.logger.info("Sector size: "+str(data["sectorsize"])+"\n")
        self.logger.info("Number of sectors: "+str(data["numsectors"])+"\n")
        self.logger.info("Vendor: "+data["vendor"]+"\n")
        self.logger.info("Product: "+data["product"]+"\n")
        self.logger.info("Revision: "+data["revision"]+"\n")

    @command
    def readrawstorage(self, volume, sector, count, addr):
        """
            Reads <count> sectors starting at <sector> from storage <volume> to memory at <addr>.
        """
        volume = self._hexint(volume)
        sector = self._hexint(sector)
        count = self._hexint(count)
        addr = self._hexint(addr)
        self.logger.info("Reading volume %s sectors %X - %X to %08X..." % (volume, sector, sector + count - 1, addr))
        self.emcore.storage_read_sectors_md(volume, sector, count, addr)
        self.logger.info("done\n")

    @command
    def writerawstorage(self, volume, sector, count, addr):
        """
            Writes memory contents at <addr> to <count> sectors starting at <sector> on storage <volume>.
        """
        volume = self._hexint(volume)
        sector = self._hexint(sector)
        count = self._hexint(count)
        addr = self._hexint(addr)
        self.logger.info("Writing %08X to volume %s sectors %X - %X..." % (addr, volume, sector, sector + count - 1))
        self.emcore.storage_write_sectors_md(volume, sector, count, addr)
        self.logger.info("done\n")

    @command
    def readrawstoragefile(self, volume, sector, count, file, buffsize = 0x100000, buffer = None):
        """
            Reads <count> sectors starting at <sector> from storage <volume> to file <file>,
            buffering them in memory at [buffer] in chunks of [buffsize] bytes (both optional).
        """
        volume = self._hexint(volume)
        sector = self._hexint(sector)
        count = self._hexint(count)
        buffsize = self._hexint(buffsize)
        try:
            f = open(file, 'wb')
        except IOError:
            raise ArgumentError("Could not open local file for writing.")
        try:
            storageinfo = self.emcore.storage_get_info(volume)
            buffsize = min(buffsize, storageinfo.sectorsize * count)
            if buffer is None:
                buffer = self.emcore.malloc(buffsize)
                malloc = True
            else:
                buffer = self._hexint(buffer)
                malloc = False
            try:
                self.logger.info("Reading volume %s sectors %X - %X to %s..." % (volume, sector, sector + count - 1, file))
                while count > 0:
                    sectors = min(count, int(buffsize / storageinfo.sectorsize))
                    self.emcore.storage_read_sectors_md(volume, sector, sectors, buffsize, buffer)
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
        """
        volume = self._hexint(volume)
        sector = self._hexint(sector)
        count = self._hexint(count)
        buffsize = self._hexint(buffsize)
        try:
            f = open(file, 'rb')
        except IOError:
            raise ArgumentError("Could not open local file for reading.")
        try:
            storageinfo = self.emcore.storage_get_info(volume)
            buffsize = min(buffsize, storageinfo.sectorsize * count)
            if buffer is None:
                buffer = self.emcore.malloc(buffsize)
                malloc = True
            else:
                buffer = self._hexint(buffer)
                malloc = False
            try:
                self.logger.info("Writing %s to volume %s sectors %X - %X..." % (file, volume, sector, sector + count - 1))
                while count > 0:
                    sectors = min(count, int(buffsize / storageinfo.sectorsize))
                    bytes = storageinfo.sectorsize * sectors
                    data = f.read(bytes)
                    if len(data) == 0: break
                    while len(data) < bytes: data = data + f.read(bytes - len(data))
                    self.emcore.write(buffer, data)
                    self.emcore.storage_write_sectors_md(volume, sector, sectors, buffsize, buffer)
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
        self.logger.info("Creating directory " + dirname + "...")
        self.emcore.dir_create(dirname)
        self.logger.info(" done\n")

    @command
    def rmdir(self, dirname):
        """
            Removes an empty directory with the name <dirname>
        """
        self.logger.info("Removing directory " + dirname + "...")
        self.emcore.dir_remove(dirname)
        self.logger.info(" done\n")

    @command
    def rm(self, filename):
        """
            Removes a file with the name <filename>
        """
        self.logger.info("Removing file " + filename + "...")
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
        self.logger.info("Renaming " + oldname + " to " + newname + "...")
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
        buffsize = self._hexint(buffsize)
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
                    buffer = self.emcore.malloc(buffsize)
                    malloc = True
                else:
                    buffer = self._hexint(buffer)
                    malloc = False
                try:
                    self.logger.info("Downloading file " + remotename + " to " + localname + "...")
                    while size > 0:
                        bytes = self.emcore.file_read(fd, buffsize, buffer)
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
        buffsize = self._hexint(buffsize)
        handle = self.emcore.dir_open(remotepath)
        try:
            if buffer is None:
                buffer = self.emcore.malloc(buffsize)
                malloc = True
            else:
                buffer = self._hexint(buffer)
                malloc = False
            try:
                try: os.mkdir(localpath)
                except: pass
                while True:
                    try:
                        entry = self.emcore.dir_read(handle)
                        if entry.name == "." or entry.name == "..": continue
                        elif entry.attributes & 0x10:
                            self.gettree(remotepath + "/" + entry.name, localpath + "/" + entry.name, buffsize, buffer)
                        else: self.get(remotepath + "/" + entry.name, localpath + "/" + entry.name, buffsize, buffer)
                    except: break
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
        buffsize = self._hexint(buffsize)
        try:
            f = open(localname, 'rb')
        except IOError:
            raise ArgumentError("Could not open local file for reading.")
        try:
            buffsize = min(buffsize, os.path.getsize(localname))
            if buffer is None:
                buffer = self.emcore.malloc(buffsize)
                malloc = True
            else:
                buffer = self._hexint(buffer)
                malloc = False
            try:
                self.logger.info("Uploading file " + localname + " to " + remotename + "...")
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
        buffsize = self._hexint(buffsize)
        if buffer is None:
            buffer = self.emcore.malloc(buffsize)
            malloc = True
        else:
            buffer = self._hexint(buffer)
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
        self.logger.info("Directory listing of " + path + ":\n")
        while True:
            try:
                entry = self.emcore.dir_read(handle)
                if entry.attributes & 0x10: size = "DIR"
                else: size = locale.format("%d", entry.size, True).rjust(13)
                self.logger.info(entry.name.ljust(50) + " - " + size + "\n")
            except: break
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
                if entry.name == "." or entry.name == "..": continue
                elif entry.attributes & 0x10: self.find(path + "/" + entry.name)
                else: self.logger.info(path + "/" + entry.name + "\n")
            except: break
        self.emcore.dir_close(handle)
    
    @command
    def malloc(self, size):
        """ Allocates <size> bytes and returns a pointer to the allocated memory """
        size = self._hexint(size)
        self.logger.info("Allocating %d bytes of memory\n" % size)
        addr = self.emcore.malloc(size)
        self.logger.info("Allocated %d bytes of memory at 0x%x\n" % (size, addr))
    
    @command
    def memalign(self, align, size):
        """ Allocates <size> bytes aligned to <align> and returns a pointer to the allocated memory """
        align = self._hexint(align)
        size = self._hexint(size)
        self.logger.info("Allocating %d bytes of memory aligned to 0x%x\n" % (size, align))
        addr = self.emcore.memalign(align, size)
        self.logger.info("Allocated %d bytes of memory at 0x%x\n" % (size, addr))
    
    @command
    def realloc(self, ptr, size):
        """ The size of the memory block pointed to by <ptr> is changed to the <size> bytes,
            expanding or reducing the amount of memory available in the block.
            Returns a pointer to the reallocated memory.
        """
        ptr = self._hexint(ptr)
        size = self._hexint(size)
        self.logger.info("Reallocating 0x%x to have the new size %d\n" % (ptr, size))
        addr = self.emcore.realloc(ptr, size)
        self.logger.info("Reallocated memory at 0x%x to 0x%x with the new size %d\n" % (ptr, addr, size))
    
    @command
    def reownalloc(self, ptr, owner):
        """ Changes the owner of the memory allocation <ptr> to the thread struct at addr <owner> """
        ptr = self._hexint(ptr)
        owner = self._hexint(owner)
        self.logger.info("Changing owner of the memory region 0x%x to 0x%x\n" % (ptr, owner))
        self.emcore.reownalloc(ptr, owner)
        self.logger.info("Successfully changed owner of 0x%x to 0x%x\n" % (ptr, owner))
    
    @command
    def free(self, ptr):
        """ Frees the memory space pointed to by 'ptr' """
        ptr = self._hexint(ptr)
        self.logger.info("Freeing the memory region at 0x%x\n" % ptr)
        self.emcore.free(ptr)
        self.logger.info("Successfully freed the memory region at 0x%x\n" % ptr)
    
    @command
    def free_all(self):
        """ Frees all memory allocations created by the monitor thread """
        self.logger.info("Freeing all memory allocations created by the monitor thread\n")
        self.emcore.free_all()
        self.logger.info("Successfully freed all memory allocations created by the monitor thread\n")
        

if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage("No command specified")
    try:
        interface = Commandline()
        interface._parsecommand(sys.argv[1], sys.argv[2:])
    except KeyboardInterrupt:
        sys.exit()