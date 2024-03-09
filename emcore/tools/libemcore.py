#!/usr/bin/env python
#
#
#    Copyright 2013 TheSeven, benedikt93, Farthen
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
import datetime

from libemcoredata import *
from misc import Logger, Bunch, remote_pointer, Error, ArgumentError, getthread, gethwname
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
        if self.lib.dev.swtypeid not in swtypes:
            raise DeviceError("Connected to unknown software type. Exiting")
        
        self.getmallocpoolbounds()
    
    @command()
    def _readmem(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. Can handle up to 0xff0 bytes.
        """
        resp = self.lib.monitorcommand(struct.pack("<IIII", 4, addr, size, 0), "III%ds" % size, (None, None, None, "data"))
        return resp.data
    
    @command()
    def _writemem(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. Can handle up to 0xff0 bytes.
        """
        return self.lib.monitorcommand(struct.pack("<IIII%ds" % len(data), 5, addr, len(data), 0, data), "III", (None, None, None))
    
    @command(timeout = 10000)
    def _readmem_bulk(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. Can handle unlimited amounts of bytes,
            however the address and size should be cacheline aligned.
        """
        return self.lib.recvbulk(struct.pack("<III", 1, addr, size), size)
    
    @command(timeout = 10000)
    def _writemem_bulk(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. Can handle unlimited amounts of bytes,
            however the address and size should be cacheline aligned.

        """
        return self.lib.sendbulk(struct.pack("<III", 1, addr, len(data)), data)
    
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
    def getmallocpoolbounds(self):
        """ This returns the memory range of the malloc pool """
        try:
            resp = self.lib.monitorcommand(struct.pack("<IIII", 1, 1, 0, 0), "III", ("lower", "upper", None))
            self.logger.debug("Malloc pool bounds = 0x%X - 0x%X\n" % (resp.lower, resp.upper))
            self.lib.dev.mallocpool.lower = resp.lower
            self.lib.dev.mallocpool.upper = resp.upper
            self.lib.dev.havemalloc = True
            return resp
        except:
            self.logger.debug("Device doesn't have a memory allocator\n")
            self.lib.dev.havemalloc = True
            return None
    
    @command()
    def reset(self, force=False):
        """ Reboot the device """
        return self.lib.monitorcommand(struct.pack("<IIII", 2, 0 if force else 1, 0, 0))
    
    @command()
    def poweroff(self, force=False):
        """ Powers the device off. """
        return self.lib.monitorcommand(struct.pack("<IIII", 3, 0 if force else 1, 0, 0))
    
    @command()
    def read(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. This takes care of splitting long requests.
        """
        data = b""
        self.logger.debug("Downloading %d bytes from 0x%X\n" % (size, addr))
        try:
            if self.lib.bulkin and size > 0x800:
                if addr & 63:
                    align = 64 - (addr & 63)
                    data += self._readmem(addr, align)
                    addr += align
                    size -= align
                align = size & 63
                size -= align
                while size > 0:
                    chunk = min(size, 0xffc00)
                    data += self._readmem_bulk(addr, chunk)
                    addr += chunk
                    size -= chunk
                size = align
        except: raise#self.logger.warn("Bulk read interface failed, falling back to slow reads\n")
        while size > 0:
            readsize = min(size, 0xf00)
            data += self._readmem(addr, readsize)
            addr += readsize
            size -= readsize
        return data
    
    @command()
    def write(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. This takes care of splitting long requests.
        """
        size = len(data)
        self.logger.debug("Uploading %d bytes to 0x%X\n" % (size, addr))
        offset = 0
        try:
            if self.lib.bulkin and size > 0x800:
                if addr & 63:
                    align = 64 - (addr & 63)
                    self._writemem(addr, data[offset:offset+align])
                    offset += align
                    addr += align
                    size -= align
                align = size & 63
                size -= align
                while size > 0:
                    chunk = min(size, 0xffc00)
                    self._writemem_bulk(addr, data[offset:offset+chunk])
                    offset += chunk
                    addr += chunk
                    size -= chunk
                size = align
        except: self.logger.warn("Bulk write interface failed, falling back to slow writes\n")
        while size > 0:
            writesize = min(size, 0xf00)
            self._writemem(addr, data[offset:offset+writesize])
            offset += writesize
            addr += writesize
            size -= writesize
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
    def readstring(self, addr, maxlength = 256, replacement = "."):
        """ Reads a zero terminated string from memory 
            Reads only a maximum of 'maxlength' chars.
        """
        if addr == 0: return "<NULL>"
        cin_maxsize = 1024
        string = ""
        done = False
        while not done and (len(string) < maxlength or maxlength < 0):
            data = self._readmem(addr, min(maxlength - len(string), cin_maxsize))
            length = data.find(b"\0")
            if length >= 0:
                data = data[:length]
                done = True
            for i in range(len(data)):
                byte = ord(data[i : i + 1])
                if byte < 0x20: string = string + replacement
                else: string = string + chr(byte)
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
        if size > 48 or size < 1:
            raise ArgumentError("Size must be a number between 1 and 48")
        return self.lib.monitorcommand(struct.pack("<IBBBBII%ds" % size, 9, index, slaveaddr, startaddr, size, 0, 0, data), "III", (None, None, None))
    
    @command()
    def usbcread(self, maxsize = 48):
        """ Reads one packet with the maximal cin size from the console """
        maxsize = min(0xff0, maxsize)
        resp = self.lib.monitorcommand(struct.pack("<IIII", 10, maxsize, 0, 0), "III%ds" % maxsize, ("validsize", "buffersize", "queuesize", "data"))
        resp.data = resp.data[:resp.validsize].decode("latin_1")
        resp.maxsize = maxsize
        return resp
    
    @command()
    def usbcwrite(self, data):
        """ Writes data to the USB console """
        cin_maxsize = 48
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
        cin_maxsize = 48
        resp = self.lib.monitorcommand(struct.pack("<IIII", 13, bitmask, cin_maxsize, 0), "III%ds" % cin_maxsize, ("size", None, None))
        resp.data = resp.data[size:]
        resp.maxsize = cin_maxsize
        return resp
    
    @command()
    def cwrite(self, data, bitmask=0x1):
        """ Writes data to the device consoles 
            identified with the specified bitmask.
        """
        cin_maxsize = 48
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
        if resp.structver != 3:
            raise DeviceError("Unsupported thread struct version!")
        
        threads = []
        structptr = resp.structptr
        id = 0
        while structptr != 0:
            threadstruct = scheduler_thread()
            self.logger.debug("Reading thread struct of thread at 0x%X\n" % structptr)
            threaddata = self._readmem(structptr, ctypes.sizeof(scheduler_thread))
            threadstruct._from_string(threaddata)
            threadstruct = threadstruct._to_bunch()
            threadstruct.id = id # only for the purpose of detecting the idle thread as it is always the first one
            threadstruct.addr = structptr
            if threadstruct.name != 0:
                threadstruct.name = self.readstring(threadstruct.name)
            else: threadstruct.name = "[Thread 0x%08X]" % structptr
            threadstruct.state = thread_state(threadstruct.state)
            if threadstruct.block_type == "THREAD_BLOCK_MUTEX":
                blocked_by_struct = mutex
            elif threadstruct.block_type == "THREAD_BLOCK_WAKEUP":
                blocked_by_struct = wakeup
            else:
                blocked_by_struct = None
            if blocked_by_struct != None:
                blocked_by_data = self.read(threadstruct.blocked_by, ctypes.sizeof(blocked_by_struct))
                blocked_by = blocked_by_struct()
                blocked_by._from_string(blocked_by_data)
                threadstruct.blocked_by = remote_pointer(threadstruct.blocked_by, blocked_by._to_bunch())
            else:
                threadstruct.blocked_by = 0
            threads.append(threadstruct)
            id += 1
            structptr = threadstruct.thread_next
        self.lockscheduler(schedulerstate)
        
        for thread in threads:
            if thread.block_type == "THREAD_BLOCK_MUTEX":
                thread.blocked_by.owner = remote_pointer(thread.blocked_by.owner, getthread(thread.blocked_by.owner, threads))
            if thread.block_type == "THREAD_BLOCK_WAKEUP":
                thread.blocked_by.waiter = remote_pointer(thread.blocked_by.waiter, getthread(thread.blocked_by.waiter, threads))
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
    def execimage(self, addr, args = [], copy = False):
        """ Runs the emCORE app at 'addr' with command line arguments 'args' """
        flags = 1 if copy else 0
        argptr = b""
        argdata = b""
        ptr = len(args) * 4
        for arg in args:
           arg = arg.encode("utf_8") + "\0"
           argptr += struct.pack("<I", ptr);
           argdata += arg
           ptr += len(arg)
        if ptr > 48:
            buf = self.malloc(ptr)
            try:
                self._writemem(buf, argptr + argdata)
                result = self.lib.monitorcommand(struct.pack("<IIBBBBI", 21, addr, 0, 0, flags, len(args), buf), "III", ("thread", None, None))
            finally: self.free(buf)
        else: return self.lib.monitorcommand(struct.pack("<IIBBBBI", 21, addr, 0, 0, flags, len(args), 0) + argptr + argdata, "III", ("thread", None, None))
    
    @command()
    def run(self, app, args = []):
        """ Uploads and runs the emCORE app in the string 'app'
            with command line arguments 'args'
        """
        if app[:8].decode("latin_1") != "emCOexec":
            raise ArgumentError("The specified app is not an emCORE application")
        baseaddr = self.malloc(len(app))
        self.write(baseaddr, app)
        result = self.execimage(baseaddr, args)
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
        if (rc.rc > 0x80000000):
            raise DeviceError("HDD access (type=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (type, sector, count, addr, rc.rc))
    
    @command(timeout = 30000, target = 0x4c435049)
    def ipodclassic_reloadbbt(self):
        """ Target-specific function: ipodclassic
            Reload the ATA bbt
        """
        rc = self.lib.monitorcommand(struct.pack("<IIII", 0xffff0003, 0, 0, 0), "III", (None, None, None))
    
    @command(timeout = 30000, target = 0x4c435049)
    def ipodclassic_disablebbt(self):
        """ Target-specific function: ipodclassic
            Disable the ATA bbt
        """
        rc = self.lib.monitorcommand(struct.pack("<IIII", 0xffff0004, 0, 0, 0), "III", (None, None, None))
    
    @command(target = 0x4c435049)
    def ipodclassic_readbbt(self, tempaddr = None):
        """ Target-specific function: ipodclassic
            Read hard drive bad block table
        """
        if tempaddr is None:
            tempaddr = self.memalign(0x10, 4096)
            malloc = True
        else:
            malloc = False
        try:
            self.ipodclassic_hddaccess(0, 0, 1, tempaddr)
            bbt = self.read(tempaddr, 4096)
        finally:
            if malloc == True:
                self.free(tempaddr)
        try:
            bbtheader = struct.unpack("<8s2024sQII512I", bbt)
        except struct.error:
            raise ArgumentError("There is no emCORE hard disk BBT present on this device")
        if bbtheader[0] != b"emBIbbth":
            raise ArgumentError("There is no emCORE hard disk BBT present on this device")
        bbtsectors = bbtheader[4]
        if malloc:
            tempaddr = self.memalign(0x10, 4096 * bbtsectors)
        try:
            sector = 1
            count = 0
            offset = 0
            for i in range(0, bbtsectors):
                if bbtheader[5 + i] == sector + count:
                    count = count + 1
                else:
                    self.ipodclassic_hddaccess(0, sector, count, tempaddr + offset)
                    offset = offset + count * 4096
                    sector = bbtheader[5 + i]
                    count = 1
            self.ipodclassic_hddaccess(0, sector, count, tempaddr + offset)
            bbt += self.read(tempaddr, 4096 * bbtsectors)
        finally:
            if malloc == True:
                self.free(tempaddr)
        return bbt
    
    @command(target = 0x4c435049)
    def ipodclassic_writebbt(self, bbt, tempaddr = None):
        """ Target-specific function: ipodclassic
            Write hard drive bad block table
        """
        try:
            bbtheader = struct.unpack("<8s2024sQII512I", bbt[:4096])
        except struct.error:
            raise ArgumentError("The specified file is not an emCORE hard disk BBT")
        if bbtheader[0] != b"emBIbbth":
            raise ArgumentError("The specified file is not an emCORE hard disk BBT")
        bbtsectors = bbtheader[4]
        if (bbtsectors + 1) * 4096 != len(bbt):
            raise ArgumentError("Size of BBT is not consistent: Expected %d bytes, got %d" % ((bbtsectors + 1) * 4096, len(bbt)))
        if tempaddr is None:
            tempaddr = self.memalign(0x10, len(bbt))
            malloc = True
        else:
            malloc = False
        try:
            self.write(tempaddr, bbt)
            self.disk_unmount(0)
            sector = 0
            count = 1
            offset = 0
            for i in range(bbtsectors):
                if bbtheader[5 + i] == sector + count:
                    count = count + 1
                else:
                    self.ipodclassic_hddaccess(1, sector, count, tempaddr + offset)
                    offset = offset + count * 4096
                    sector = bbtheader[5 + i]
                    count = 1
            self.ipodclassic_hddaccess(1, sector, count, tempaddr + offset)
            self.ipodclassic_reloadbbt()
            self.disk_mount(0)
        finally:
            if malloc == True:
                self.free(tempaddr)
    
    @command()
    def storage_get_info(self, volume):
        """ Get information about a storage device """
        self.logger.debug("Getting storage information\n")
        result = self.lib.monitorcommand(struct.pack("<IIII", 27, volume, 0, 0), "IIIIIIIII", ("version", None, None, "sectorsize", "numsectors", "vendorptr", "productptr", "revisionptr", "driverinfoptr"))
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
            raise DeviceError("storage_read_sectors_md(volume=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (volume, sector, count, addr, result.rc))

    @command(timeout = 50000)
    def storage_write_sectors_md(self, volume, sector, count, addr):
        """ Read sectors from as storage device """
        self.logger.debug("Writing %d sectors from memory at 0x%X to disk at volume %d, sector %d\n" % (count, addr, volume, sector))
        result = self.lib.monitorcommand(struct.pack("<IIQIIII", 29, volume, sector, count, addr, 0, 0), "III", ("rc", None, None))
        self.logger.debug("Wrote sectors, result: 0x%X\n" % result.rc)
        if result.rc > 0x80000000:
            raise DeviceError("storage_write_sectors_md(volume=%d, sector=%d, count=%d, addr=0x%08X) failed with RC 0x%08X" % (volume, sector, count, addr, result.rc))
    
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
        fn = filename.encode("utf_8")
        self.logger.debug("Opening remote file %s with mode %d\n" % (filename, mode))
        bytes = len(fn) + 1
        if bytes > 48:
            buf = self.malloc(bytes)
            try:
                self._writemem(buf, fn + b"\0")
                result = self.lib.monitorcommand(struct.pack("<IIII", 30, mode, 0, buf), "III", ("fd", None, None))
            finally: self.free(buf)
        else: result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(fn), 30, mode, 0, 0, fn, 0), "III", ("fd", None, None))
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
        result = self.lib.monitorcommand(struct.pack("<IIII", 35, fd, length, 0), "III", ("rc", None, None))
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
        fn = filename.encode("utf_8")
        self.logger.debug("Deleting file %s\n" % (filename))
        bytes = len(fn)
        if bytes > 48:
            buf = self.malloc(bytes)
            try:
                self._writemem(buf, fn + b"\0")
                result = self.lib.monitorcommand(struct.pack("<IIII", 40, 0, 0, buf), "III", ("rc", None, None))
            finally: self.free(buf)
        else: result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(fn), 40, 0, 0, 0, fn, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("file_unlink(filename=\"%s\") failed with RC=0x%08X, errno=%d" % (filename, result.rc, self.errno()))
        self.logger.debug("Delete file result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def file_rename(self, oldname, newname):
        """ Renames a file """
        on = oldname.encode("utf_8")
        nn = newname.encode("utf_8")
        self.logger.debug("Renaming file %s to %s\n" % (on, nn))
        obytes = len(on) + 1
        nbytes = len(nn) + 1
        buf = self.malloc(obytes + nbytes)
        try:
            self._writemem(buf, on + b"\0" + nn + b"\0")
            result = self.lib.monitorcommand(struct.pack("<IIII", 41, 0, buf, buf + obytes), "III", ("rc", None, None))
        finally: self.free(buf)
        if result.rc > 0x80000000:
            raise DeviceError("file_rename(oldname=\"%s\", newname=\"%s\") failed with RC=0x%08X, errno=%d" % (oldname, newname, result.rc, self.errno()))
        self.logger.debug("Rename file result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_open(self, dirname):
        """ Opens a directory and returns the handle """
        dn = dirname.encode("utf_8")
        self.logger.debug("Opening directory %s\n" % (dirname))
        bytes = len(dn)
        if bytes > 48:
            buf = self.malloc(bytes)
            try:
                self._writemem(buf, dn + b"\0")
                result = self.lib.monitorcommand(struct.pack("<IIII", 42, 0, 0, buf), "III", ("handle", None, None))
            finally: self.free(buf)
        else: result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dn), 42, 0, 0, 0, dn, 0), "III", ("handle", None, None))
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
        ret.name = ret.name[:ret.name.index(b"\0")].decode("utf_8")
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
        dn = dirname.encode("utf_8")
        self.logger.debug("Creating directory %s\n" % (dirname))
        bytes = len(dn)
        if bytes > 48:
            buf = self.malloc(bytes)
            try:
                self._writemem(buf, dn + b"\0")
                result = self.lib.monitorcommand(struct.pack("<IIII", 47, 0, 0, buf), "III", ("rc", None, None))
            finally: self.free(buf)
        else: result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dn), 47, 0, 0, 0, dn, 0), "III", ("rc", None, None))
        if result.rc > 0x80000000:
            raise DeviceError("dir_create(dirname=\"%s\") failed with RC=0x%08X, errno=%d" % (dirname, result.rc, self.errno()))
        self.logger.debug("Create directory result: 0x%X\n" % (result.rc))
        return result.rc
    
    @command(timeout = 30000)
    def dir_remove(self, dirname):
        """ Removes an (empty) directory """
        dn = dirname.encode("utf_8")
        self.logger.debug("Removing directory %s\n" % (dirname))
        bytes = len(dn)
        if bytes > 48:
            buf = self.malloc(bytes)
            try:
                self._writemem(buf, dn + b"\0")
                result = self.lib.monitorcommand(struct.pack("<IIII", 48, 0, 0, buf), "III", ("rc", None, None))
            finally: self.free(buf)
        else: result = self.lib.monitorcommand(struct.pack("<IIII%dsB" % len(dn), 48, 0, 0, 0, dn, 0), "III", ("rc", None, None))
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
    
    @command()
    def rtcread(self):
        """ Reads the real time clock on the device """
        self.logger.debug("Reading the clock\n")
        date = self.lib.monitorcommand(struct.pack("<IIII", 60, 0, 0, 0), "BBBBBBBBI", ("second", "minute", "hour", "weekday", "day", "month", "year", None, None))
        dt = datetime.datetime(date.year + 2000, date.month, date.day, date.hour, date.minute, date.second)
        self.logger.debug("Read date '%s' from device", (dt.ctime()))
        return dt
    
    @command()
    def rtcwrite(self, dt):
        """ Sets the real time clock on the device to the datetime object 'dt' """
        self.logger.debug("Setting the clock to: %s\n" % (dt.ctime()))
        if dt.year < 2000 or dt.year > 2255:
            raise ArgumentError("The Year must be between 2000 and 2255")
        return self.lib.monitorcommand(struct.pack("<IBBBBBBBBI", 61, dt.second, dt.minute, dt.hour, dt.weekday(), dt.day, dt.month, dt.year - 2000, 0, 0), "III", (None, None, None))


class Lib(object):
    def __init__(self, logger):
        self.logger = logger
        self.logger.debug("Initializing Lib object\n")
        self.idVendor = 0xFFFF
        self.idProduct = 0xE000
        self.idProductMask = 0xFFFE
        
        self.headersize = 0x10
        
        self.connect()
    
    def connect(self):
        self.dev = Dev(self.idVendor, self.idProduct, self.idProductMask, self.logger)
        self.connected = True
        self.bulkout = True if self.dev.bulkout else False
        self.bulkin = True if self.dev.bulkin else False
    
    def monitorcommand(self, cmd, rcvdatatypes=None, rcvstruct=None):
        self.logger.debug("Sending monitorcommand [0x%s]\n" % base64.b16encode(cmd[3::-1]).decode("ascii"))
        writelen = self.dev.send(cmd)
        if rcvdatatypes:
            rcvdatatypes = "I" + rcvdatatypes # add the response
            data = self.dev.receive(struct.calcsize(rcvdatatypes))
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
            
    def sendbulk(self, cmd, data):
        self.logger.debug("Sending bulk command [0x%s] with %d bytes\n" % (base64.b16encode(cmd[3::-1]).decode("ascii"), len(data)))
        return self.dev.sendbulk(cmd, data)
            
    def recvbulk(self, cmd, size):
        self.logger.debug("Receiving bulk command [0x%s] with %d bytes\n" % (base64.b16encode(cmd[3::-1]).decode("ascii"), size))
        return self.dev.recvbulk(cmd, size)


class Dev(object):
    def __init__(self, idVendor, idProduct, idProductMask, logger):
        self.idVendor = idVendor
        self.idProduct = idProduct
        self.idProductMask = idProductMask
        
        self.logger = logger
        self.logger.debug("Initializing Dev object\n")
        
        self.dev = None
        self.interface = None
        self.bulkout = None
        self.bulkin = None
        self.claimed = False
        self.timeout = 1000
        
        self.connect()
        
        self.logger.debug("Successfully connected to device\n")
        
        self.version = Bunch()
        self.version.revision = None
        self.version.majorv = None
        self.version.minorv = None
        self.version.patchv = None
        self.swtypeid = None
        self.hwtypeid = None
        
        self.havemalloc = False
        self.mallocpool = Bunch()
        self.mallocpool.lower = None
        self.mallocpool.upper = None
    
    def __del__(self):
        if self.claimed: self.disconnect()
    
    def connect(self):
        self.logger.debug("Looking for emCORE device\n")
        devs = usb.core.find(find_all=True, idVendor=self.idVendor)
        for dev in devs:
            self.logger.debug("%04x:%04x\n" % (dev.idVendor, dev.idProduct))
            if dev.idProduct & self.idProductMask == self.idProduct:
                self.dev = dev
                break
        if self.dev is None:
            raise DeviceNotFoundError()
        self.logger.debug("Device found!\n")
        self.logger.debug("Searching for device interface:\n")
        for cfg in self.dev:
            for intf in cfg:
                self.logger.debug("%02x:%02x:%02x\n" % (intf.bInterfaceClass, intf.bInterfaceSubClass, intf.bInterfaceProtocol))
                if intf.bInterfaceClass == 0xff and intf.bInterfaceSubClass == 0 and intf.bInterfaceProtocol == 0:
                    self.interface = intf.bInterfaceNumber
                    for ep in intf:
                        if not ep.bEndpointAddress & 0x80:
                            self.bulkout = ep
                            break
                    for ep in intf:
                        if ep.bEndpointAddress & 0x80:
                            self.bulkin = ep
                            break
                    break
        if self.interface is None:
            raise DeviceNotFoundError()
        self.logger.debug("Debugger interface found!\n")
        self.logger.debug("Claiming interface...\n")
        usb.util.claim_interface(self.dev, self.interface)
        self.claimed = True
    
    def disconnect(self):
        usb.util.release_interface(self.dev, self.interface)
        self.claimed = False
    
    def send(self, data):
        if len(data) > 0x1000: raise DeviceError("Attempting to send a message that is too big!")
        size = self.dev.ctrl_transfer(0x41, 0x00, 0, self.interface, data, self.timeout)
        if size != len(data): raise SendError("Not all data was written!")
        return size
    
    def receive(self, size):
        if size > 0x1000: raise DeviceError("Attempting to receive a message that is too big!")
        data = self.dev.ctrl_transfer(0xc1, 0x00, 0, self.interface, size, self.timeout)
        if len(data) != size: raise ReceiveError("Requested size and read size don't match!")
        return data
    
    def sendbulk(self, cmd, data):
        cmdsize = self.dev.ctrl_transfer(0x42, 0x00, 0, self.bulkout.bEndpointAddress, cmd, self.timeout)
        if cmdsize != len(cmd):
            raise SendError("Bulk send command could not be fully sent (%d of %d)!" % (cmdsize, len(cmd)))
        size = self.bulkout.write(data, self.timeout)
        if size != len(data):
            raise SendError("Bulk data could not be fully sent (%d of %d)!" % (size, len(data)))
        return size
    
    def recvbulk(self, cmd, size):
        cmdsize = self.dev.ctrl_transfer(0x42, 0x00, 0, self.bulkin.bEndpointAddress, cmd, self.timeout)
        if cmdsize != len(cmd):
            raise ReceiveError("Bulk receive command could not be fully sent (%d of %d)!" % (cmdsize, len(cmd)))
        data = self.bulkin.read(size, self.timeout)
        if len(data) != size:
            raise SendError("Bulk data could not be fully received (%d of %d)!" % (len(data), size))
        return data
    

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
