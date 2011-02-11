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
    emCORE client library.
    Provides functions to communicate with emCORE devices via the USB bus.
"""

import sys
import struct
import ctypes
import usb.core
import base64

from libemcoredata import *
from misc import Logger, Bunch, Error, ArgumentError, gethwname
from functools import wraps

class DeviceNotFoundError(Error):
    pass

class DeviceError(Error):
    pass

class SendError(Error):
    pass

class ReceiveError(Error):
    pass
    

def command(timeout = None, target = None):
    """
        Decorator for all commands.
        It adds the "timeout" variable to all commands.
        It also provides the possibility to set the timeout directly in the decorator.
        It also includes some dirty hacks to not learn from.
    """
    time = timeout # dirty hack because otherwise it would raise a scoping problem.
                   # The reason is probably because I suck but I can't find any good explanation of this.
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            self = args[0] # little cheat as it expects self being always the first argument
            # precommand stuff
            if target is not None:
                if self.lib.dev.hwtypeid != target:
                    raise DeviceError("Wrong device for target-specific command. Expected \'%s\' but got \'%s\'" %
                                     (gethwname(target), gethwname(self.lib.dev.hwtypeid)))
            timeout = None
            if "timeout" in kwargs.keys():
                timeout = kwargs['timeout']
            elif time is not None:
                timeout = time
            if timeout is not None:
                oldtimeout = self.lib.dev.timeout
                self.lib.dev.timeout = timeout
            # function call
            ret = func(*args)
            # postcommand stuff
            if timeout is not None:
                self.lib.dev.timeout = oldtimeout
            return ret
        func._command = True
        wrapper.func = func
        return wrapper
    return decorator


class Emcore(object):
    """
        Class for all emcore functions.
        They all get the "@command()" decorator.
        This decorator has a timeout variable that can be set to change the
        device timeout for the duration of the function.
        It also adds a "timeout" argument to every function to access this
        feature from external. So DON'T EVER use a parameter called 'timeout'
        in your commands. Variables are ok.
        
        If you want to enable logging please pass a misc.Logger object to the
        constructor.
    """
    def __init__(self, logger = Logger(loglevel = -1)):
        self.logger = logger
        self.logger.debug("Initializing Emcore object\n")
        self.lib = Lib(self.logger)
        
        self.getversioninfo()
        if self.lib.dev.swtypeid != 2:
            if self.lib.dev.swtypeid == 1:
                raise DeviceError("Connected to emBIOS. emBIOS is not supported by libemcore")
            else:
                raise DeviceError("Connected to unknown software type. Exiting")
        
        self.getpacketsizeinfo()
        self.getusermemrange()
    
    @staticmethod
    def _alignsplit(addr, size, blksize, align):
        if size <= blksize: return (size, 0, 0)
        end = addr + size
        if addr & (align - 1):
            bodyaddr = (addr + min(size, blksize)) & ~(align - 1)
        else: bodyaddr = addr
        headsize = bodyaddr - addr
        if (size - headsize) & (align - 1):
            tailaddr = (end - min(end - bodyaddr, blksize) + align - 1) & ~(align - 1)
        else: tailaddr = end
        tailsize = end - tailaddr
        return (headsize, tailaddr - bodyaddr, tailsize)
    
    @command()
    def _readmem(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device.
        """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 4, addr, size, 0), "III%ds" % size, (None, None, None, "data"))
        return resp.data
    
    @command()
    def _writemem(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device.
        """
        return self.lib.monitorcommand(struct.pack("<IIII%ds" % len(data), 5, addr, len(data), 0, data), "III", (None, None, None))
    
    @command()
    def _readdma(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. This uses DMA and the data in endpoint.
        """
        self.lib.monitorcommand(struct.pack("<IIII", 6, addr, size, 0), "III", (None, None, None))
        return struct.unpack("<%ds" % size, self.lib.dev.din(size))[0]
    
    @command()
    def _writedma(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. This uses DMA and the data out endpoint.
        """
        self.lib.monitorcommand(struct.pack("<IIII", 7, addr, len(data), 0), "III", (None, None, None))
        return self.lib.dev.dout(data)
    
    @command()
    def getversioninfo(self):
        """ This returns the emCORE version and device information. """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 1, 0, 0, 0), "IBBBBI", ("revision", "majorv", "minorv", "patchv", "swtypeid", "hwtypeid"))
        self.lib.dev.version.revision = resp.revision
        self.lib.dev.version.majorv = resp.majorv
        self.lib.dev.version.minorv = resp.minorv
        self.lib.dev.version.patchv = resp.patchv
        self.logger.debug("Device Software Type ID = 0x%X\n" % resp.swtypeid)
        self.lib.dev.swtypeid = resp.swtypeid
        self.logger.debug("Device Hardware Type ID = 0x%X\n" % resp.hwtypeid)
        self.lib.dev.hwtypeid = resp.hwtypeid
        return resp
    
    @command()
    def getpacketsizeinfo(self):
        """ This returns the emCORE max packet size information.
            It also sets the properties of the device object accordingly.
        """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 1, 1, 0, 0), "HHII", ("coutmax", "cinmax", "doutmax", "dinmax"))
        self.logger.debug("Device cout packet size limit = %d\n" % resp.coutmax)
        self.lib.dev.packetsizelimit.cout = resp.coutmax
        self.logger.debug("Device cin packet size limit = %d\n" % resp.cinmax)
        self.lib.dev.packetsizelimit.cin = resp.cinmax
        self.logger.debug("Device din packet size limit = %d\n" % resp.doutmax)
        self.lib.dev.packetsizelimit.din = resp.dinmax
        self.logger.debug("Device dout packet size limit = %d\n" % resp.dinmax)
        self.lib.dev.packetsizelimit.dout = resp.doutmax
        return resp
    
    @command()
    def getusermemrange(self):
        """ This returns the memory range the user has access to. """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 1, 2, 0, 0), "III", ("lower", "upper", None))
        self.logger.debug("Device user memory = 0x%X - 0x%X\n" % (resp.lower, resp.upper))
        self.lib.dev.usermem.lower = resp.lower
        self.lib.dev.usermem.upper = resp.upper
        return resp
    
    @command()
    def reset(self, force=False):
        """ Reboot the device """
        if force:
            return self.lib.monitorcommand(struct.pack("<IIII", 2, 0, 0, 0))
        else:
            return self.lib.monitorcommand(struct.pack("<IIII", 2, 1, 0, 0), "III", (None, None, None))
    
    @command()
    def poweroff(self, force=False):
        """ Powers the device off. """
        if force:
            return self.lib.monitorcommand(struct.pack("<IIII", 3, 0, 0, 0))
        else:
            return self.lib.monitorcommand(struct.pack("<IIII", 3, 1, 0, 0), "III", (None, None, None))
    
    @command()
    def read(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. This cares about too long packages
            and decides whether to use DMA or not.
        """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        din_maxsize = self.lib.dev.packetsizelimit.din
        data = b""
        (headsize, bodysize, tailsize) = self._alignsplit(addr, size, cin_maxsize, 16)
        self.logger.debug("Downloading %d bytes from 0x%X, split as (%d/%d/%d)\n" % (size, addr, headsize, bodysize, tailsize))
        if headsize != 0:
            data += self._readmem(addr, headsize)
            addr += headsize
        while bodysize > 0:
            if bodysize >= 2 * cin_maxsize:
                readsize = min(bodysize, din_maxsize)
                data += self._readdma(addr, readsize)
            else:
                readsize = min(bodysize, cin_maxsize)
                data += self._readmem(addr, readsize)
            addr += readsize
            bodysize -= readsize
        if tailsize != 0:
            data += self._readmem(addr, tailsize)
        return data
    
    @command()
    def write(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. This cares about too long packages
            and decides whether to use DMA or not.
        """
        cout_maxsize = self.lib.dev.packetsizelimit.cout - self.lib.headersize
        dout_maxsize = self.lib.dev.packetsizelimit.dout
        (headsize, bodysize, tailsize) = self._alignsplit(addr, len(data), cout_maxsize, 16)
        self.logger.debug("Uploading %d bytes to 0x%X, split as (%d/%d/%d)\n" % (len(data), addr, headsize, bodysize, tailsize))
        offset = 0
        if headsize != 0:
            self._writemem(addr, data[offset:offset+headsize])
            offset += headsize
            addr += headsize
        while bodysize > 0:
            if bodysize >= 2 * cout_maxsize:
                writesize = min(bodysize, dout_maxsize)
                self._writedma(addr, data[offset:offset+writesize])
            else:
                writesize = min(bodysize, cout_maxsize)
                self._writemem(addr, data[offset:offset+writesize])
            offset += writesize
            addr += writesize
            bodysize -= writesize
        if tailsize != 0:
            self._writemem(addr, data[offset:offset+tailsize])
        return data
    
    @command()
    def upload(self, data):
        """ Allocates memory of the size of 'data' and uploads 'data' to that memory region.
            Returns the address where 'data' is stored
        """
        addr = self.malloc(len(data))
        self.write(addr, data)
        return addr
    
    @command()
    def readstring(self, addr, maxlength = 256):
        """ Reads a zero terminated string from memory 
            Reads only a maximum of 'maxlength' chars.
        """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        string = ""
        while (len(string) < maxlength or maxlength < 0):
            data = self._readmem(addr, min(maxlength - len(string), cin_maxsize))
            length = data.find(b"\0")
            if length >= 0:
                string += data[:length].decode("latin_1")
                break
            else:
                string += data.decode("latin_1")
            addr += cin_maxsize
        return string
    
    @command()
    def i2cread(self, index, slaveaddr, startaddr, size):
        """ Reads data from an i2c slave """
        data = b""
        for i in range(size):
            resp = self.lib.monitorcommand(struct.pack("<IBBBBII", 8, index, slaveaddr, startaddr + i, 1, 0, 0), "III1s", (None, None, None, "data"))
            data += resp.data
        return data
    
    @command()
    def i2cwrite(self, index, slaveaddr, startaddr, data):
        """ Writes data to an i2c slave """
        size = len(data)
        if size > 256 or size < 1:
            raise ArgumentError("Size must be a number between 1 and 256")
        if size == 256:
            size = 0
        return self.lib.monitorcommand(struct.pack("<IBBBBII%ds" % size, 9, index, slaveaddr, startaddr, size, 0, 0, data), "III", (None, None, None))
    
    @command()
    def usbcread(self):
        """ Reads one packet with the maximal cin size from the console """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        resp = self.lib.monitorcommand(struct.pack("<IIII", 10, cin_maxsize, 0, 0), "III%ds" % cin_maxsize, ("validsize", "buffersize", "queuesize", "data"))
        resp.data = resp.data[:resp.validsize].decode("latin_1")
        resp.maxsize = cin_maxsize
        return resp
    
    @command()
    def usbcwrite(self, data):
        """ Writes data to the USB console """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        size = len(data)
        while len(data) > 0:
            writesize = min(cin_maxsize, len(data))
            resp = self.lib.monitorcommand(struct.pack("<IIII%ds" % writesize, 11, writesize, 0, 0, data[:writesize]), "III", ("validsize", "buffersize", "freesize"))
            data = data[resp.validsize:]
        return size
    
    @command()
    def cread(self, bitmask=0x1):
        """ Reads one packet with the maximal cin size from the device consoles
            identified with the specified bitmask
        """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        resp = self.lib.monitorcommand(struct.pack("<IIII", 13, bitmask, cin_maxsize, 0), "III%ds" % cin_maxsize, ("size", None, None))
        resp.data = resp.data[size:]
        resp.maxsize = cin_maxsize
        return resp
    
    @command()
    def cwrite(self, data, bitmask=0x1):
        """ Writes data to the device consoles 
            identified with the specified bitmask.
        """
        cin_maxsize = self.lib.dev.packetsizelimit.cin - self.lib.headersize
        size = len(data)
        while len(data) > 0:
            writesize = min(cin_maxsize, len(data))
            resp = self.lib.monitorcommand(struct.pack("<IIII%ds" % writesize, 12, bitmask, writesize, 0, data[:writesize]), "III", (None, None, None))
            data = data[writesize:]
        return size
    
    @command()
    def cflush(self, bitmask):
        """ Flushes the consoles specified with 'bitmask' """
        return self.lib.monitorcommand(struct.pack("<IIII", 14, bitmask, 0, 0), "III", (None, None, None))
    
    @command()
    def getprocinfo(self):
        """ Gets current state of the scheduler """
        schedulerstate = self.lockscheduler()
        resp = self.lib.monitorcommand(struct.pack("<IIII", 15, 0, 0, 0), "III", ("structver", "structptr", None))
        if resp.structver != 2:
            raise DeviceError("Unsupported thread struct version!")
        
        threads = []
        structptr = resp.structptr
        id = 0
        while structptr != 0:
            threadstruct = scheduler_thread()
            self.logger.debug("Reading thread struct of thread at 0x%X\n" % structptr)
            threaddata = self.read(structptr, ctypes.sizeof(scheduler_thread))
            threadstruct._from_string(threaddata)
            threadstruct = threadstruct._to_bunch()
            threadstruct.id = id # only for the purpose of detecting the idle thread as it is always the first one
            threadstruct.addr = structptr
            threadstruct.name = self.readstring(threadstruct.name)
            threadstruct.state = thread_state(threadstruct.state)
            threads.append(threadstruct)
            id += 1
            structptr = threadstruct.thread_next
        self.lockscheduler(schedulerstate)
        return threads
    
    @command()
    def lockscheduler(self, freeze=True):
        """ Freezes/Unfreezes the scheduler """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 16, 1 if freeze else 0, 0, 0), "III", ("before", None, None))
        return True if resp.before == 1 else False
    
    @command()
    def unlockscheduler(self):
        """ Unfreezes the scheduler """
        return self.lib.monitorcommand(struct.pack("<IIII", 16, 0, 0, 0), "III", ("before", None, None))
    
    @command()
    def suspendthread(self, id, suspend=True):
        """ Suspends the thread with the specified id """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 17, 1 if suspend else 0, id, 0), "III", ("before", None, None))
        return True if resp.before == 1 else False
    
    @command()
    def resumethread(self, id):
        """ Resumes the thread with the specified id """
        return self.lib.monitorcommand(struct.pack("<IIII", 17, 0, id, 0), "III", ("before", None, None))
    
    @command()
    def killthread(self, id):
        """ Kills the thread with the specified id """
        return self.lib.monitorcommand(struct.pack("<IIII", 18, id, 0, 0), "III", ("before", None, None))
    
    @command()
    def createthread(self, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state):
        """ Creates a thread with the specified attributes """
        if threadtype == "user":
            threadtype = 0
        elif threadtype == "system":
            threadtype = 1
        else:
            raise ArgumentError("Threadtype must be either 'system' or 'user'")
        if priority > 256 or priority < 0:
            raise ArgumentError("Priority must be a number between 0 and 256")
        if state == "ready":
            state = 0
        elif state == "suspended":
            state = 1
        else:
            raise ArgumentError("State must be either 'ready' or 'suspended'")
        resp = self.lib.monitorcommand(struct.pack("<IIIIIIII", 19, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state), "III", ("threadptr", None, None))
        if resp.threadptr < 0:
            raise DeviceError("The device returned the error code %d" % resp.threadptr)
        return resp
    
    @command()
    def flushcaches(self):
        """ Flushes the CPU instruction and data cache """
        return self.lib.monitorcommand(struct.pack("<IIII", 20, 0, 0, 0), "III", (None, None, None))
    
    @command()
    def execimage(self, addr):
        """ Runs the emCORE app at 'addr' """
        return self.lib.monitorcommand(struct.pack("<IIII", 21, addr, 0, 0), "III", ("thread", None, None))
    
    @command()
    def run(self, app):
        """ Uploads and runs the emCORE app in the string 'app' """
        if app[:8].decode("ascii") != "emCOexec":
            raise ArgumentError("The specified app is not an emCORE application")
        baseaddr = self.malloc(len(app))
        self.write(baseaddr, app)
        result = self.execimage(baseaddr)
        return Bunch(thread=result.thread)
    
    @command(timeout = 5000)
    def bootflashread(self, memaddr, flashaddr, size):
        """ Copies the data in the bootflash at 'flashaddr' of the specified size
            to the memory at addr 'memaddr'
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 22, memaddr, flashaddr, size), "III", (None, None, None))
    
    @command(timeout = 30000)
    def bootflashwrite(self, memaddr, flashaddr, size):
        """ Copies the data in the memory at 'memaddr' of the specified size
            to the boot flash at addr 'flashaddr'
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 23, memaddr, flashaddr, size), "III", (None, None, None))
    
    @command()
    def execfirmware(self, targetaddr, addr, size):
        """ Moves the firmware at 'addr' with size 'size' to 'targetaddr' and passes all control to it. """
        self.logger.debug("Moving firmware at 0x%X with the size %d to 0x%X and executing it\n" % (addr, size, targetaddr))
        return self.lib.monitorcommand(struct.pack("<IIII", 24, targetaddr, addr, size))
    
    @command(timeout = 30000)
    def aesencrypt(self, addr, size, keyindex):
        """ Encrypts the buffer at 'addr' with the specified size
            with the hardware AES key index 'keyindex'
        """
        return self.lib.monitorcommand(struct.pack("<IBBHII", 25, 1, 0, keyindex, addr, size), "III", (None, None, None))
    
    @command(timeout = 30000)
    def aesdecrypt(self, addr, size, keyindex):
        """ Decrypts the buffer at 'addr' with the specified size
            with the hardware AES key index 'keyindex'
        """
        return self.lib.monitorcommand(struct.pack("<IBBHII", 25, 0, 0, keyindex, addr, size), "III", (None, None, None))
    
    @command(timeout = 30000)
    def hmac_sha1(self, addr, size, destination):
        """ Generates a HMAC-SHA1 hash of the buffer and saves it to 'destination' """
        return self.lib.monitorcommand(struct.pack("<IIII", 26, addr, size, destination), "III", (None, None, None))

    @command(target = 0x47324e49)
    def ipodnano2g_getnandinfo(self):
        """ Target-specific function: ipodnano2g
            Gathers some information about the NAND chip used
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 0xffff0001, 0, 0, 0), "IHHHH", ("type", "pagesperblock", "banks", "userblocks", "blocks"))
    
    @command(timeout = 30000, target = 0x47324e49)
    def ipodnano2g_nandread(self, addr, start, count, doecc, checkempty):
        """ Target-specific function: ipodnano2g
            Reads data from the NAND chip into memory
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 0xffff0002, addr | (0x80000000 if doecc else 0) | (0x40000000 if checkempty else 0), start, count), "III", (None, None, None))
    
    @command(timeout = 30000, target = 0x47324e49)
    def ipodnano2g_nandwrite(self, addr, start, count, doecc):
        """ Target-specific function: ipodnano2g
            Writes data to the NAND chip
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 0xffff0003, addr | (0x80000000 if doecc else 0), start, count), "III", (None, None, None))
    
    @command(timeout = 30000, target = 0x47324e49)
    def ipodnano2g_nanderase(self, addr, start, count):
        """ Target-specific function: ipodnano2g
            Erases blocks on the NAND chip and stores the results to memory
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 0xffff0004, addr, start, count), "III", (None, None, None))
    
    @command(target = 0x4c435049)
    def ipodclassic_gethddinfo(self):
        """ Target-specific function: ipodclassic
            Gather information about the hard disk drive
        """
        return self.lib.monitorcommand(struct.pack("<IIII", 0xffff0001, 0, 0, 0), "IQQII", ("identifyptr", "totalsectors", "virtualsectors", "bbtptr", "bbtsize"))
    
    @command(timeout = 30000, target = 0x4c435049)
    def ipodclassic_hddaccess(self, type, sector, count, addr):
        """ Target-specific function: ipodclassic
            Access the hard disk, type = 0 (read) / 1 (write)
        """
        rc = self.lib.monitorcommand(struct.pack("<IIQIIII", 0xffff0002, type, sector, count, addr, 0, 0), "III", ("rc", None, None))
        if (rc > 0x80000000):
            raise DeviceError("HDD access (type=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (type, sector, count, addr, rc))
    
    @command(target = 0x4c435049)
    def ipodclassic_writebbt(self, bbt, tempaddr):
        """ Target-specific function: ipodclassic
            Write hard drive bad block table
        """
        try:
            bbtheader = struct.unpack("<8s2024sQII512I", bbt[:4096])
        except struct.error:
            raise ArgumentError("The specified file is not an emCORE hard disk BBT")
        if bbtheader[0] != "emBIbbth":
            raise ArgumentError("The specified file is not an emCORE hard disk BBT")
        virtualsectors = bbtheader[2]
        bbtsectors = bbtheader[3]
        self.write(tempaddr, bbt)
        sector = 0
        count = 1
        offset = 0
        for i in range(bbtsectors):
            if bbtheader[4][i] == sector + count:
                count = count + 1
            else:
                self.ipodclassic_hddaccess(1, sector, count, tempaddr + offset)
                offset = offset + count * 4096
                sector = bbtheader[4][i]
                count = 1
        self.ipodclassic_hddaccess(1, sector, count, tempaddr + offset)
    
    @command()
    def storage_get_info(self, volume):
        """ Get information about a storage device """
        self.logger.debug("Getting storage information\n")
        result = self.lib.monitorcommand(struct.pack("<IIII", 27, volume, 0, 0), "IIIIIIII", ("version", None, None, "sectorsize", "numsectors", "vendorptr", "productptr", "revisionptr"))
        if result.version != 1:
            raise ValueError("Unknown version of storage_info struct: %d" % result.version)
        result.vendor = self.readstring(result.vendorptr)
        result.product = self.readstring(result.productptr)
        result.revision = self.readstring(result.revisionptr)
        self.logger.debug("Got storage information:\n")
        self.logger.debug("Vendor: %s\n" % result.vendor)
        self.logger.debug("Product: %s\n" % result.product)
        self.logger.debug("Revision: %s\n" % result.revision)
        self.logger.debug("Sector size: %d\n" % result.sectorsize)
        self.logger.debug("Number of sectors: %d\n" % result.numsectors)
        return result
    
    @command(timeout = 50000)
    def storage_read_sectors_md(self, volume, sector, count, addr):
        """ Read sectors from as storage device """
        self.logger.debug("Reading %d sectors from disk at volume %d, sector %d to memory at 0x%X\n" % (count, volume, sector, addr))
        result = self.lib.monitorcommand(struct.pack("<IIQIIII", 28, volume, sector, count, addr, 0, 0), "III", ("rc", None, None))
        self.logger.debug("Read sectors, result: 0x%X\n" % result.rc)
        if result.rc > 0x80000000:
            raise DeviceError("storage_read_sectors_md(volume=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (volume, sector, count, addr, rc))

    @command(timeout = 50000)
    def storage_write_sectors_md(self, volume, sector, count, addr):
        """ Read sectors from as storage device """
        self.logger.debug("Writing %d sectors from memory at 0x%X to disk at volume %d, sector %d\n" % (count, addr, volume, sector))
        result = self.lib.monitorcommand(struct.pack("<IIQIIII", 29, volume, sector, count, addr, 0, 0), "III", ("rc", None, None))
        self.logger.debug("Wrote sectors, result: 0x%X\n" % result.rc)
        if result.rc > 0x80000000:
            raise DeviceError("storage_write_sectors_md(volume=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (volume, sector, count, addr, rc))
    
    @command(timeout = 30000)
    def fat_enable_flushing(self, state):
        """ Enables/disables flushing the FAT cache after every transaction """
        if state != 0: self.logger.debug("Enabling FAT flushing\n")
        else: self.logger.debug("Disabling FAT flushing\n")
        self.lib.monitorcommand(struct.pack("<IIII", 58, state, 0, 0), "III", (None, None, None))
        if state != 0: self.logger.debug("Enabled FAT flushing\n")
        else: self.logger.debug("Disabled FAT flushing\n")
    
    @command(timeout = 30000)
    def file_open(self, filename, mode):
        """ Opens a file and returns the handle """
        self.logger.debug("Opening remote file %s with mode %d\n" % (filename, mode))
        result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(filename), 30, mode, 0, 0, filename, 0), "III", ("fd", None, None))
        if result.fd > 0x80000000:
            raise DeviceError("file_open(filename=\"%s\", mode=0x%X) failed with RC=0x%08X, errno=%d" % (filename, mode, result.fd, self.errno()))
        self.logger.debug("Opened file as handle 0x%X\n" % result.fd)
        return result.fd
    
    @command(timeout = 30000)
    def file_size(self, fd):
        """ Gets the size of a file referenced by a handle """
        self.logger.debug("Getting file size of handle 0x%X\n" % fd)
        result = self.lib.monitorcommand(struct.pack("<IIII", 31, fd, 0, 0), "III", ("size", None, None))
        if result.size > 0x80000000:
            raise DeviceError("file_size(fd=%d) failed with RC=0x%08X, errno=%d" % (fd, result.size, self.errno()))
        self.logger.debug("Got file size: %d bytes\n" % result.size)
        return result.size
    
    @command(timeout = 30000)
    def file_read(self, fd, size, addr = None):
        """ Reads data from a file referenced by a handle. If addr is not given it allocates a buffer itself. """
        if addr is None:
            addr = self.malloc(size)
            malloc = True
        else:
            malloc = False
        self.logger.debug("Reading %d bytes from file handle 0x%X to 0x%X\n" % (size, fd, addr))
        try:
            result = self.lib.monitorcommand(struct.pack("<IIII", 32, fd, addr, size), "III", ("rc", None, None))
            if result.rc > 0x80000000:
                raise DeviceError("file_read(fd=%d, addr=0x%08X, size=0x%08X) failed with RC=0x%08X, errno=%d" % (fd, addr, size, result.rc, self.errno()))
        except:
            if malloc == True:
                self.free(addr)
            raise
        self.logger.debug("File read result: 0x%X\n" % result.rc)
        return Bunch(rc = result.rc, addr = addr)
    
    @command(timeout = 30000)
    def file_write(self, fd, size, addr):
        """ Writes data from a file referenced by a handle. """
        self.logger.debug("Writing %d bytes from 0x%X to file handle 0x%X\n" % (size, addr, fd))
        result = self.lib.monitorcommand(struct.pack("<IIII", 33, fd, addr, size), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_write(fd=%d, addr=0x%08X, size=0x%08X) failed with RC=0x%08X, errno=%d" % (fd, addr, size, result.rc, self.errno()))
        self.logger.debug("File write result: 0x%X\n" % result.rc)
        return result.rc
    
    @command(timeout = 30000)
    def file_seek(self, fd, offset, whence):
        """ Seeks the file handle to the specified position in the file """
        self.logger.debug("Seeking file handle 0x%X to whence=%d, offset=0x%X\n" % (fd, whence, offset))
        result = self.lib.monitorcommand(struct.pack("<IIII", 34, fd, offset, whence), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_seek(fd=%d, offset=0x%08X, whence=%d) failed with RC=0x%08X, errno=%d" % (fd, offset, whence, result.rc, self.errno()))
        self.logger.debug("File seek result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_truncate(self, fd, length):
        """ Truncates a file referenced by a handle to a specified length """
        self.logger.debug("Truncating file with handle 0x%X to 0x%X bytes\n" % (fd, length))
        result = self.lib.monitorcommand(struct.pack("<IIII", 35, fd, offset, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_truncate(fd=%d, length=0x%08X) failed with RC=0x%08X, errno=%d" % (fd, length, result.rc, self.errno()))
        self.logger.debug("File truncate result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_sync(self, fd):
        """ Flushes a file handles' buffers """
        self.logger.debug("Flushing buffers of file with handle 0x%X\n" % (fd))
        result = self.lib.monitorcommand(struct.pack("<IIII", 36, fd, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_sync(fd=%d) failed with RC=0x%08X, errno=%d" % (fd, result.rc, self.errno()))
        self.logger.debug("File flush result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_close(self, fd):
        """ Closes a file handle """
        self.logger.debug("Closing file handle 0x%X\n" % (fd))
        result = self.lib.monitorcommand(struct.pack("<IIII", 37, fd, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_close(fd=%d) failed with RC=0x%08X, errno=%d" % (fd, result.rc, self.errno()))
        self.logger.debug("File close result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_close_all(self):
        """ Closes all file handles opened through the debugger """
        self.logger.debug("Closing all files that were opened via USB\n")
        result = self.lib.monitorcommand(struct.pack("<IIII", 38, 0, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_close_all() failed with RC=0x%08X, errno=%d" % (result.rc, self.errno()))
        self.logger.debug("Closed %d files\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_kill_all(self, volume):
        """ Kills all file handles of a volume (in the whole system) """
        self.logger.debug("Killing all file handles of volume %d\n" % (volume))
        result = self.lib.monitorcommand(struct.pack("<IIII", 39, volume, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_kill_all() failed with RC=0x%08X, errno=%d" % (result.rc, self.errno()))
        self.logger.debug("Closed %d files\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_unlink(self, filename):
        """ Removes a file """
        self.logger.debug("Deleting file %s\n" % (filename))
        result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(filename), 40, 0, 0, 0, filename, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_unlink(filename=\"%s\") failed with RC=0x%08X, errno=%d" % (filename, result.rc, self.errno()))
        self.logger.debug("Delete file result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_rename(self, oldname, newname):
        """ Renames a file """
        self.logger.debug("Renaming file %s to %s\n" % (oldname, newname))
        result = self.lib.monitorcommand(struct.pack("<IIII248s%dsB" % min(247, len(newname)), 41, 0, 0, 0, oldname, newname, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_rename(oldname=\"%s\", newname=\"%s\") failed with RC=0x%08X, errno=%d" % (oldname, newname, result.rc, self.errno()))
        self.logger.debug("Rename file result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_open(self, dirname):
        """ Opens a directory and returns the handle """
        self.logger.debug("Opening directory %s\n" % (dirname))
        result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dirname), 42, 0, 0, 0, dirname, 0), "III", ("handle", None, None))
        if result.handle == 0:
            raise DeviceError("dir_open(dirname=\"%s\") failed with RC=0x%08X, errno=%d" % (dirname, result.handle, self.errno()))
        self.logger.debug("Opened directory as handle 0x%X\n" % (result.handle))
        return result.handle
    
    @command(timeout = 30000)
    def dir_read(self, handle):
        """ Reads the next entry from a directory """
        self.logger.debug("Reading next entry of directory handle 0x%X\n" % (handle))
        result = self.lib.monitorcommand(struct.pack("<IIII", 43, handle, 0, 0), "III", ("version", "maxpath", "ptr"))
        if result.ptr == 0:
            raise DeviceError("dir_read(handle=0x%08X) failed with RC=0x%08X, errno=%d" % (handle, result.ptr, self.errno()))
        if result.version != 1:
            raise ValueError("Unknown version of dirent struct: %d" % result.version)
        dirent = self.read(result.ptr, result.maxpath + 16)
        ret = Bunch()
        (ret.name, ret.attributes, ret.size, ret.startcluster, ret.wrtdate, ret.wrttime) = struct.unpack("<%dsIIIHH" % result.maxpath, dirent)
        ret.name = ret.name[:ret.name.index('\x00')]
        self.logger.debug("Read directory entry:\n")
        self.logger.debug("Name: %s\n" % ret.name)
        self.logger.debug("Attributes: 0x%X\n" % ret.attributes)
        self.logger.debug("Size: %d\n" % ret.size)
        self.logger.debug("Start cluster: %d\n" % ret.startcluster)
        self.logger.debug("Last written date: 0x%X\n" % ret.wrtdate)
        self.logger.debug("Last written time: 0x%X\n" % ret.wrttime)
        return ret
    
    @command(timeout = 30000)
    def dir_close(self, handle):
        """ Closes a directory handle """
        self.logger.debug("Closing directory handle 0x%X\n" % (handle))
        result = self.lib.monitorcommand(struct.pack("<IIII", 44, handle, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_close(handle=0x%08X) failed with RC=0x%08X, errno=%d" % (handle, result.rc, self.errno()))
        self.logger.debug("Close directory result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_close_all(self):
        """ Closes all directory handles opened through the debugger """
        self.logger.debug("Closing all directories that were opened via USB\n")
        result = self.lib.monitorcommand(struct.pack("<IIII", 45, 0, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_close_all() failed with RC=0x%08X, errno=%d" % (result.rc, self.errno()))
        self.logger.debug("Closed %d directories\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_kill_all(self, volume):
        """ Kills all directory handles of a volume (in the whole system) """
        self.logger.debug("Closing all directories of volume %d\n" % (volume))
        result = self.lib.monitorcommand(struct.pack("<IIII", 46, volume, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_kill_all() failed with RC=0x%08X, errno=%d" % (result.rc, self.errno()))
        self.logger.debug("Closed %d directories\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_create(self, dirname):
        """ Creates a directory """
        self.logger.debug("Creating directory %s\n" % (dirname))
        result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dirname), 47, 0, 0, 0, dirname, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_create(dirname=\"%s\") failed with RC=0x%08X, errno=%d" % (dirname, result.rc, self.errno()))
        self.logger.debug("Create directory result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_remove(self, dirname):
        """ Removes an (empty) directory """
        self.logger.debug("Removing directory %s\n" % (dirname))
        result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dirname), 48, 0, 0, 0, dirname, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_remove(dirname=\"%s\") failed with RC=0x%08X, errno=%d" % (dirname, result.rc, self.errno()))
        self.logger.debug("Remove directory result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command()
    def errno(self):
        """ Returns the number of the last error that happened """
        self.logger.debug("Getting last error number\n")
        result = self.lib.monitorcommand(struct.pack("<IIII", 49, 0, 0, 0), "III", ("errno", None, None))
        self.logger.debug("Last error: 0x%X\n" % (result.errno))
        return result.errno
    
    @command()
    def disk_mount(self, volume):
        """ Mounts a volume """
        self.logger.debug("Mounting volume %d\n" % (volume))
        result = self.lib.monitorcommand(struct.pack("<IIII", 50, volume, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("disk_mount(volume=%d) failed with RC=0x%08X, errno=%d" % (volume, result.rc, self.errno()))
        self.logger.debug("Mount volume result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command()
    def disk_unmount(self, volume):
        """ Unmounts a volume """
        self.logger.debug("Unmounting volume %d\n" % (volume))
        result = self.lib.monitorcommand(struct.pack("<IIII", 51, volume, 0, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("disk_unmount(volume=%d) failed with RC=0x%08X, errno=%d" % (volume, result.rc, self.errno()))
        self.logger.debug("Unmount volume result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command()
    def malloc(self, size):
        """ Allocates 'size' bytes and returns a pointer to the allocated memory """
        self.logger.debug("Allocating %d bytes of memory\n" % size)
        result = self.lib.monitorcommand(struct.pack("<IIII", 52, size, 0, 0), "III", ("ptr", None, None))
        self.logger.debug("Allocated %d bytes of memory at 0x%X\n" % (size, result.ptr))
        return result.ptr
    
    @command()
    def memalign(self, align, size):
        """ Allocates 'size' bytes aligned to 'align' and returns a pointer to the allocated memory """
        self.logger.debug("Allocating %d bytes of memory aligned to 0x%X\n" % (size, align))
        result = self.lib.monitorcommand(struct.pack("<IIII", 53, align, size, 0), "III", ("ptr", None, None))
        self.logger.debug("Allocated %d bytes of memory at 0x%X\n" % (size, result.ptr))
        return result.ptr
    
    @command()
    def realloc(self, ptr, size):
        """ The size of the memory block pointed to by 'ptr' is changed to the 'size' bytes,
            expanding or reducing the amount of memory available in the block.
            Returns a pointer to the reallocated memory.
        """
        self.logger.debug("Reallocating 0x%X to have the new size %d\n" % (ptr, size))
        result = self.lib.monitorcommand(struct.pack("<IIII", 54, ptr, size, 0), "III", ("ptr", None, None))
        self.logger.debug("Reallocated memory at 0x%X to 0x%X with the new size %d\n" % (ptr, result.ptr, size))
        return result.ptr
    
    @command()
    def reownalloc(self, ptr, owner):
        """ Changes the owner of the memory allocation 'ptr' to the thread struct at addr 'owner' """
        self.logger.debug("Changing owner of the memory region 0x%X to 0x%X\n" % (ptr, owner))
        return self.lib.monitorcommand(struct.pack("<IIII", 55, ptr, owner, 0), "III", (None, None, None))
    
    @command()
    def free(self, ptr):
        """ Frees the memory space pointed to by 'ptr' """
        self.logger.debug("Freeing the memory region at 0x%X\n" % ptr)
        return self.lib.monitorcommand(struct.pack("<IIII", 56, ptr, 0, 0), "III", (None, None, None))
    
    @command()
    def free_all(self):
        """ Frees all memory allocations created by the monitor thread """
        self.logger.debug("Freeing all memory allocations created by the monitor thread\n")
        return self.lib.monitorcommand(struct.pack("<IIII", 57, 0, 0, 0), "III", (None, None, None))


class Lib(object):
    def __init__(self, logger):
        self.logger = logger
        self.logger.debug("Initializing Lib object\n")
        self.idVendor = 0xFFFF
        self.idProduct = 0xE000
        
        self.headersize = 0x10
        
        self.connect()
    
    def connect(self):
        self.dev = Dev(self.idVendor, self.idProduct, self.logger)
        self.connected = True
    
    def monitorcommand(self, cmd, rcvdatatypes=None, rcvstruct=None):
        self.logger.debug("Sending monitorcommand [0x%s]\n" % base64.b16encode(cmd[3::-1]).decode("ascii"))
        writelen = self.dev.cout(cmd)
        if rcvdatatypes:
            rcvdatatypes = "I" + rcvdatatypes # add the response
            data = self.dev.cin(struct.calcsize(rcvdatatypes))
            data = struct.unpack(rcvdatatypes, data)
            try:
                response = responsecode(data[0])
            except IndexError:
                self.logger.debug("Response: UNKOWN\n")
                raise DeviceError("Invalid response! This should NOT happen!")
            if response == "OK":
                self.logger.debug("Response: OK\n")
                if rcvstruct:
                    datadict = Bunch()
                    counter = 1 # start with 1, 0 is the id
                    for item in rcvstruct:
                        if item != None: # else the data is undefined
                            datadict[item] = data[counter]
                        counter += 1
                    return datadict
                else:
                    return data
            elif response == "UNSUPPORTED":
                self.logger.debug("Response: UNSUPPORTED\n")
                raise DeviceError("The device does not support this command.")
            elif response == "INVALID":
                self.logger.debug("Response: INVALID\n")
                raise DeviceError("Invalid command! This should NOT happen!")
            elif response == "BUSY":
                self.logger.debug("Response: BUSY\n")
                raise DeviceError("Device busy")
        else:
            return writelen


class Dev(object):
    def __init__(self, idVendor, idProduct, logger):
        self.idVendor = idVendor
        self.idProduct = idProduct
        
        self.logger = logger
        self.logger.debug("Initializing Dev object\n")
        
        self.interface = 0
        self.timeout = 100
        
        self.connect()
        self.findEndpoints()
        
        self.logger.debug("Successfully connected to device\n")
        
        # Device properties
        self.packetsizelimit = Bunch()
        self.packetsizelimit.cout = None
        self.packetsizelimit.cin = None
        self.packetsizelimit.dout = None
        self.packetsizelimit.din = None
        
        self.version = Bunch()
        self.version.revision = None
        self.version.majorv = None
        self.version.minorv = None
        self.version.patchv = None
        self.swtypeid = None
        self.hwtypeid = None
        
        self.usermem = Bunch()
        self.usermem.lower = None
        self.usermem.upper = None
    
    def __del__(self):
        self.disconnect()
    
    def findEndpoints(self):
        self.logger.debug("Searching for device endpoints:\n")
        epcounter = 0
        self.endpoint = Bunch()
        for cfg in self.dev:
            for intf in cfg:
                for ep in intf:
                    if epcounter == 0:
                        self.logger.debug("Found cout endpoint at 0x%X\n" % ep.bEndpointAddress)
                        self.endpoint.cout = ep.bEndpointAddress
                    elif epcounter == 1:
                        self.logger.debug("Found cin endpoint at 0x%X\n" % ep.bEndpointAddress)
                        self.endpoint.cin = ep.bEndpointAddress
                    elif epcounter == 2:
                        self.logger.debug("Found dout endpoint at 0x%X\n" % ep.bEndpointAddress)
                        self.endpoint.dout = ep.bEndpointAddress
                    elif epcounter == 3:
                        self.logger.debug("Found din endpoint at 0x%X\n" % ep.bEndpointAddress)
                        self.endpoint.din = ep.bEndpointAddress
                    epcounter += 1
        if epcounter <= 3:
            raise DeviceError("Not all endpoints found in the descriptor. Only %d endpoints found, at least 4 endpoints were expeceted" % epcounter)
    
    def connect(self):
        self.logger.debug("Looking for emCORE device\n")
        self.dev = usb.core.find(idVendor=self.idVendor, idProduct=self.idProduct)
        if self.dev is None:
            raise DeviceNotFoundError()
        self.logger.debug("Device Found!\n")
        self.logger.debug("Setting first configuration\n")
        self.dev.set_configuration()
    
    def disconnect(self):
        pass
    
    def send(self, endpoint, data):
        size = self.dev.write(endpoint, data, self.interface, self.timeout)
        if size != len(data):
            raise SendError("Not all data was written!")
        return len
    
    def receive(self, endpoint, size):
        read = self.dev.read(endpoint, size, self.interface, self.timeout)
        if len(read) != size:
            raise ReceiveError("Requested size and read size don't match!")
        return read
    
    def cout(self, data):
        self.logger.debug("Sending data to cout endpoint with the size %d\n" % len(data))
        if self.packetsizelimit.cout and len(data) > self.packetsizelimit.cout:
            raise SendError("Packet too big")
        return self.send(self.endpoint.cout, data)
    
    def cin(self, size):
        self.logger.debug("Receiving data on the cin endpoint with the size %d\n" % size)
        if self.packetsizelimit.cin and size > self.packetsizelimit.cin:
            raise ReceiveError("Packet too big")
        return self.receive(self.endpoint.cin, size)
    
    def dout(self, data):
        self.logger.debug("Sending data to cout endpoint with the size %d\n" % len(data))
        if self.packetsizelimit.dout and len(data) > self.packetsizelimit.dout:
            raise SendError("Packet too big")
        return self.send(self.endpoint.dout, data)
    
    def din(self, size):
        self.logger.debug("Receiving data on the din endpoint with the size %d\n" % size)
        if self.packetsizelimit.din and size > self.packetsizelimit.din:
            raise ReceiveError("Packet too big")
        return self.receive(self.endpoint.din, size)


if __name__ == "__main__":
    from misc import Logger
    logger = Logger()
    
    if sys.argv[1] == "gendoc":
        # Generates Documentation
        from misc import gendoc
        logger.write("Generating documentation\n")
        cmddict = {}
        for attr, value in Emcore.__dict__.items():
            if getattr(value, 'func', False):
                if getattr(value.func, '_command', False):
                    cmddict[value.func.__name__] = value
        logger.write(gendoc(cmddict))
    
    elif sys.argv[1] == "malloctest":
        emcore = Emcore()
        logger.write("Allocating 200 bytes of memory: ")
        addr = emcore.malloc(200)
        logger.write("0x%X\n" % addr)
        logger.write("Reallocating to 2000 bytes: ")
        addr = emcore.realloc(addr, 2000)
        logger.write("0x%X\n" % addr)
        logger.write("Freeing 0x%X\n" % addr)
        emcore.free(addr)
        logger.write("Allocating 1000 bytes of memory aligned to 100 bytes: ")
        addr = emcore.memalign(100, 1000)
        logger.write("0x%X\n" % addr)
        logger.write("Freeing 0x%X\n" % addr)
        emcore.free(addr)