#!/usr/bin/env python
#
#
#    Copyright 2010 TheSeven, benedikt93, Farthen
#
#
#    This file is part of emBIOS.
#
#    emBIOS is free software: you can redistribute it and/or
#    modify it under the terms of the GNU General Public License as
#    published by the Free Software Foundation, either version 2 of the
#    License, or (at your option) any later version.
#
#    emBIOS is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
#
#

import sys
import os
import inspect
import re
import time

from functools import wraps

import libembios
from libembios import Error
import libembiosdata

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


def usage(errormsg=None, specific=False):
    """
        Prints the usage information.
        It is auto generated from various places.
    """
    logger = Logger()
    cmddict= Commandline.cmddict
    doc = {}
    # This sorts the output of various internal functions
    # and puts everything in easy readable form
    for function in cmddict:
        function = cmddict[function].func
        docinfo = {}
        name = function.__name__
        args = inspect.getargspec(function)[0]
        docinfo['varargs'] = False
        if inspect.getargspec(function)[1]:
            docinfo['varargs'] = True
        kwargvalues = inspect.getargspec(function)[3]
        kwargs = {}
        if args:
            if kwargvalues:
                argnum = len(args) - len(kwargvalues)
                kwargnum = len(kwargvalues)
                kwargs = dict(zip(args[argnum:], kwargvalues))
            else:
                argnum = len(args)
        else:
            argnum = 0
        docinfo['args'] = args[1:argnum]
        docinfo['kwargs'] = kwargs
        if function.__doc__:
            # strip unneccessary whitespace
            docinfo['documentation'] = re.sub(r'\n        ', '\n', function.__doc__)
        else:
            docinfo['documentation'] = None
        doc[name] = docinfo

    if not specific:
        logger.log("Please provide a command and (if needed) parameters as command line arguments\n\n")
        logger.log("Available commands:\n\n")
    else:
        logger.log("\n")
    for function in sorted(doc.items()):
        function = function[0]
        if specific == False or specific == function:
            logger.log("  " + function + " ")
            for arg in doc[function]['args']:
                logger.log("<" + arg + "> ")
            if doc[function]['kwargs']:
                for kwarg in doc[function]['kwargs']:
                    logger.log("[" + kwarg + "] ")
            if doc[function]['varargs']:
                logger.log("<db1> ... <dbN>")
            if doc[function]['documentation']:
                logger.log(doc[function]['documentation']+"\n")
    
    logger.log("\n")

    if errormsg:
        logger.error(str(errormsg)+"\n")
    exit(2)


class Logger(object):
    """
        Simple stdout logger.
        Loglevel 4 is most verbose, Loglevel 0 only say something if there is an error.
    """
    def __init__(self):
        # Possible values: 0 (only errors), 1 (warnings), 2 (info, recommended for production use), 3 and more (debug)
        self.loglevel = 3
        
    def log(self, text):
        sys.stdout.write(text)
    
    def debug(self, text):
        if self.loglevel >= 3:
            self.log(text)
    
    def info(self, text):
        if self.loglevel >= 2:
            self.log(text)
    
    def warning(self, text):
        if self.loglevel >= 1:
            self.log("WARNING: " + text)
    
    def error(self, text):
        self.log("ERROR: " + text)


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
        without an error message or raise ArgumentCountError
    """
    def __init__(self):
        self.logger = Logger()
        try:
            self.embios = libembios.Embios()
        except libembios.DeviceNotFoundError:
            self.logger.error("No emBIOS device found!")
            exit(1)
        self.getinfo("version")
        
    def _parsecommand(self, func, args):
        # adds self to the commandline args.
        # this is needed because the functions need access to their class.
        args.insert(0, self)
        if func in self.cmddict:
            try:
                self.cmddict[func](*args)
            except ArgumentError, e:
                usage(e, specific=func)
            except ArgumentError:
                usage("Syntax Error in function '" + func + "'", specific=func)
            except ArgumentTypeError, e:
                usage(e, specific=func)
            except NotImplementedError:
                self.logger.error("This function is not implemented yet!")
            except libembios.DeviceError, e:
                self.logger.error(str(e))
            except ValueError:
                usage(specific=func)
            except TypeError, e:
                if str(e).split(" ", 1)[0] == func + "()":
                    self.logger.error(usage("Argument Error in '" + func + "': Wrong argument count", specific=func))
                else:
                    raise
            except libembios.usb.core.USBError:
                self.logger.error("Problem with USB connection.")
        else:
            usage("No such command")
    
    @staticmethod
    def _bool(something):
        """
            Converts quite everything into bool.
        """
        if type(something) == bool:
            return something
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
        elif type(something) == NoneType:
            return None
        else:
            raise ArgumentTypeError("hexadecimal coded integer", "'"+str(something)+"'")

    @staticmethod
    def _strcheck(string, values):
        if string in values:
            return string
        else:
            expected = ""
            for item in values:
                expected += "'" + item + "', "
            expected = expected[:-2]
            raise ArgumentTypeError("one out of " + expected, "'" + string + "'")
    
    @staticmethod
    def _hex(integer):
        return "0x%x" % integer
    
    @command
    def getinfo(self, infotype):
        """
            Get info on the running emBIOS.
            <infotype> may be either of 'version', 'packetsize', 'usermemrange'.
        """
        if infotype == "version":
            resp = self.embios.getversioninfo()
            self.logger.info("Connected to "+libembiosdata.swtypes[resp.swtypeid] + " v" + str(resp.majorv) + "." + str(resp.minorv) +
                             "." + str(resp.patchv) + " r" + str(resp.revision) + " running on " + libembiosdata.hwtypes[resp.hwtypeid] + "\n")
        elif infotype == "packetsize":
            resp = self.embios.getpacketsizeinfo()
            self.logger.info("Maximum packet sizes: "+str(resp))
        elif infotype == "usermemrange":
            resp = self.embios.getusermemrange()
            self.logger.info("The user memory range is "+self._hex(resp.lower)+" - "+self._hex(resp.upper-1))
        else:
            raise ArgumentTypeError("one out of 'version', 'packetsize', 'usermemrange'", infotype)
    
    @command
    def reset(self, force=False):
        """
            Resets the device"
            If <force> is 1, the reset will be forced, otherwise it will be gracefully,
            which may take some time.
        """
        force = self._bool(force)
        if force: self.logger.info("Resetting forcefully...\n")
        else: self.logger.info("Resetting...\n")
        self.embios.reset(force)
    
    @command
    def poweroff(self, force=False):
        """
            Powers the device off
            If <force> is 1, the poweroff will be forced, otherwise it will be gracefully,
            which may take some time.
        """
        force = self._bool(force)
        if force: self.logger.info("Resetting forcefully...\n")
        else: self.logger.info("Resetting...\n")
        self.embios.reset(force)
    
    @command
    def uploadfile(self, addr, filename):
        """
            Uploads a file to the device
            <offset>: the address to upload the file to
            <filename>: the path to the file
        """
        addr = self._hexint(addr)
        try:
            f = open(filename, 'rb')
        except IOError:
            raise ArgumentError("File not readable. Does it exist?")
        self.logger.info("Writing file '"+filename+"' to memory at "+self._hex(addr)+"...")
        with f:
            self.embios.write(addr, f.read())
        f.close()
        self.logger.info("done\n")
    
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
        self.logger.info("Reading data from address "+self._hex(addr)+" with the size "+self._hex(size)+" to '"+filename+"'...")
        with f:
            f.write(self.embios.read(addr, size))
        f.close()
        self.logger.info("done\n")

    @command
    def uploadint(self, addr, integer):
        """
            Uploads a single integer to the device
            <offset>: the address to upload the integer to
            <data>: the integer to upload
        """
        addr = self._hexint(addr)
        integer = self._hexint(integer)
        if integer > 0xFFFFFFFF:
            raise ArgumentError("Specified integer too long")
        data = chr(integer)
        self.embios.write(addr, data)
        self.logger.info("Integer '"+self._hex(integer)+"' written successfully to "+self._hex(addr))

    @command
    def downloadint(self, addr):
        """
            Downloads a single integer from the device and prints it to the console window
            <offset>: the address to download the integer from
        """
        addr = self._hexint(addr)
        data = self.embios.read(addr, 1)
        integer = ord(data)
        self.logger.info("Integer '"+self._hex(integer)+"' read from address "+self._hex(addr))

    @command
    def i2cread(self, bus, slave, addr, size):
        """
            Reads data from an I2C device
            <bus> the bus index
            <slave> the slave address
            <addr> the start address on the I2C device
            <size> the number of bytes to read
        """
        bus = self._hexint(bus)
        slave = self._hexint(slave)
        addr = self._hexint(addr)
        size = self._hexint(size)
        self.embios.i2cread(bus, slave, addr, size)

    @command
    def i2cwrite(self, bus, slave, addr, *args):
        """
            Writes data to an I2C device
            <bus> the bus index
            <slave> the slave address
            <addr> the start address on the I2C device
            <db1> ... <dbN> the data in single bytes, encoded in hex,
                seperated by whitespaces, eg. 37 56 45 12
        """
        bus = self._hexint(bus)
        slave = self._hexint(slave)
        addr = self._hexint(addr)
        data = ""
        for arg in args:
            data += chr(self._hexint(arg))
        self.embios.i2cwrite(bus, slave, addr, data)

    @command
    def console(self):
        """
            Reads data from the USB console continuously
        """
        while True:
            resp = self.embios.usbcread()
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
        self.logger.info("Writing '"+text+"' to the usb console\n")
        self.embios.usbcwrite(text)

    @command
    def readdevconsole(self, bitmask):
        """
            Reads data continuously from one or more of the device's consoles.
            <bitmask>: the bitmask of the consoles to read from.
        """
        bitmask = self._hexint(bitmask)
        while True:
            resp = self.embios.cread()
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
        self.logger.info("Writing '"+text+"' to the device consoles identified with "+self._hex(bitmask)+"\n")
        self.embios.cwrite(text, bitmask)

    @command
    def flushconsolebuffers(self, bitmask):
        """
            flushes one or more of the device consoles' buffers.
            <bitmask>: the bitmask of the consoles to be flushed
        """
        bitmask = self._hexint(bitmask)
        raise NotImplementedError

    @command
    def getprocinfo(self):
        """
            Fetches data on the currently running processes
        """
        import datetime
        threads = self.embios.getprocinfo()
        self.logger.info("The device has "+str(len(threads))+" running threads:\n\n")
        for thread in threads:
            self.logger.info("  "+thread.name+":\n")
            self.logger.info("    Thread id: "+str(thread.id)+"\n")
            self.logger.info("    Thread type: "+thread.type+"\n")
            self.logger.info("    Thread state: "+thread.state+"\n")
            self.logger.info("    Priority: "+str(thread.priority)+"/256\n")
            self.logger.info("    CPU time (total): "+str(datetime.timedelta(microseconds=thread.cputime_total))+"\n")
            self.logger.info("    Stack address: "+self._hex(thread.stackaddr)+"\n")
            self.logger.info("    Registers:\n")
            for registerrange in range(4):
                self.logger.info("      ")
                for register in range(registerrange, 16, 4):
                    registerrepr = "r"+str(register)
                    self.logger.info("{:3s}: 0x{:08X}    ".format(registerrepr, thread.regs["r"+str(register)]))
                self.logger.info("\n")
            self.logger.info("      cpsr: 0x{:08X}".format(thread.regs.cpsr))
            self.logger.info("\n")
    
    @command
    def lockscheduler(self):
        """
            Locks (freezes) the scheduler
        """
        self.logger.info("Will now lock scheduler\n")
        self.embios.lockscheduler()
    
    @command
    def unlockscheduler(self):
        """
            Unlocks (unfreezes) the scheduler
        """
        self.logger.info("Will now unlock scheduler\n")
        self.embios.unlockscheduler()
    
    @command
    def suspendthread(self, threadid):
        """
            Suspends/resumes the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.embios.resumethread(threadid)

    @command
    def resumethread(self, threadid):
        """
            Resumes the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.embios.resumethread(threadid)

    @command
    def killthread(self, threadid):
        """
            Kills the thread with thread ID <threadid>
        """
        threadid = self._hexint(threadid)
        self.embios.killthread(threadid)

    @command
    def createthread(self, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state):
        """
            Creates a new thread and returns its thread ID
            <namepointer> a pointer to the thread's name
            <entrypoint> a pointer to the entrypoint of the thread
            <stackpointer> a pointer to the stack of the thread
            <stacksize> the size of the thread's stack
            <type> the thread type, vaild are: 0 => user thread, 1 => system thread
            <priority> the priority of the thread, from 1 to 255
            <state> the thread's initial state, valid are: 1 => ready, 0 => suspended
        """
        nameptr = self._hexint(nameptr)
        entrypoint = self._hexint(entrypoint)
        stackpointer = self._hexint(stackpointer)
        stacksize = self._hexint(stacksize)
        priority = self._hexint(priority)
        self.embios.createthread(nameptr, entrypoint, stackptr, stacksize, type, priority, state)
    
    @command
    def run(self, filename):
        """
            Uploads the emBIOS application <filename> to
            the beginning of the user memory and executes it
        """
        #self.execimage(addr)
        raise NotImplementedError

    @command
    def execimage(self, addr):
        """
            Executes the emBIOS application at <addr>.
        """
        addr = self._hexint(addr)
        self.logger.info("Starting emBIOS app at "+self._hex(addr)+"\n")
        self.embios.execimage(addr)
    
    @command
    def flushcaches(self):
        """
            Flushes the CPUs data and instruction caches.
        """
        self.logger.info("Flushing CPU data and instruction caches...")
        self.embios.flushcaches()
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
        self.embios.bootflashread(addr_flash, addr_mem, size)
    
    @command
    def writebootflash(self, addr_flash, addr_mem, size, force=False):
        """
            Writes <size> bytes from memory to bootflash.
            ATTENTION: Don't call this unless you really know what you're doing!
            This may BRICK your device (unless it has a good recovery option)
            <addr_mem>: the address in memory to copy the data from
            <addr_bootflsh>: the address in bootflash to write to
            <force>: Use this flag to suppress the 5 seconds delay
        """
        addr_flash = self._hexint(addr_flash)
        addr_mem = self._hexint(addr_mem)
        size = self._hexint(size)
        force = self._bool(force)
        self.logger.info("Writing boot flash from the memory in "+self._hex(addr_mem)+" - "+
                         hex(addr_mem+size)+" to "+self._hex(addr_flash)+" - "+self._hex(addr_flash+size)+"\n")
        if force == False:
            self.logger.info("If this was not what you intended press Ctrl-C NOW")
            for i in range(10):
                self.logger.info(".")
                time.sleep(1)
            self.logger.info("\n")
        self.embios.bootflashwrite(addr_flash, addr_mem, size)
    
    @command
    def runfirmware(self, addr, filename):
        """
            Uploads the firmware in 'filename' to the beginning of the
            user memory and executes it
        """
        addr = self._hexint(addr)
        self.uploadfile(addr, filename)
        self.execfirmware(addr)
    
    @command
    def execfirmware(self, addr):
        """
            Executes the firmware at addr
        """
        addr = self._hexint(addr)
        self.logger.info("Running firmware at "+self._hex(addr)+". Bye.")
        self.embios.execfirmware(addr)
    
    @command
    def aesencrypt(self, addr, size, keyindex):
        """
            Encrypts a buffer using a hardware key
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        keyindex = self._hexint(keyindex)
        self.embios.aesencrypt(addr, size, keyindex)
    
    @command
    def aesdecrypt(self, addr, size, keyindex):
        """
            Decrypts a buffer using a hardware key
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        keyindex = self._hexint(keyindex)
        self.embios.aesdecrypt(addr, size, keyindex)
    
    @command
    def hmac_sha1(self, addr, size, destination):
        """
            Generates a HMAC-SHA1 hash of the buffer and saves it to 'destination'
        """
        addr = self._hexint(addr)
        size = self._hexint(size)
        destination = self._hexint(destination)
        sha1size = 0x14
        self.logger.info("Generating hmac-sha1 hash from the buffer at "+self._hex(addr)+" with the size "+self._hex(size)+
                         " and saving it to "+self._hex(destination)+" - "+self._hex(destination+sha1size)+"...")
        self.embios.hmac_sha1(addr, size, destination)
        self.logger.info("done\n")
        data = self.embios.read(destination, sha1size)
        hash = ord(data)
        self.logger.info("The generated hash is "+self._hex(hash))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        usage("No command specified")
    interface = Commandline()
    interface._parsecommand(sys.argv[1], sys.argv[2:])