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
import struct
import usb.core
import libembiosdata

class Error(Exception):
    def __init__(self, value=None):
        self.value = value
    def __str__(self):
        if self.value != None:
            return repr(self.value)

class ArgumentError(Error):
    pass

class DeviceNotFoundError(Error):
    pass

class DeviceError(Error):
    pass

class SendError(Error):
    pass

class ReceiveError(Error):
    pass


class Bunch(dict):
    """
        This is a dict whose items can also be accessed with
        bunchinstance.something.
    """
    def __init__(self, **kw):
        dict.__init__(self, kw)
        self.__dict__ = self
    
    def __getstate__(self):
        return self
    
    def __setstate__(self, state):
        self.update(state)
        self.__dict__ = self


class Embios(object):
    """
        Class for all embios functions.
    """
    def __init__(self):
        self.lib = Lib()
        self.getpacketsizeinfo()
    
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
    
    def _readmem(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device.
        """
        resp = self.lib.monitorcommand(struct.pack("IIII", 4, addr, size, 0), "III%ds" % size, (None, None, None, "data"))
        return resp.data
        
    def _writemem(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device.
        """
        return self.lib.monitorcommand(struct.pack("IIII%ds" % len(data), 5, addr, len(data), 0, data), "III", (None, None, None))
    
    def _readdma(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. This uses DMA and the data in endpoint.
        """
        self.lib.monitorcommand(struct.pack("IIII", 6, addr, size, 0), "III", (None, None, None))
        return struct.unpack("%ds" % size, self.lib.dev.din(size))[0]
    
    def _writedma(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. This uses DMA and the data out endpoint.
        """
        self.lib.monitorcommand(struct.pack("IIII", 7, addr, len(data), 0), "III", (None, None, None))
        return self.lib.dev.dout(data)
    
    def getversioninfo(self):
        """ This returns the emBIOS version and device information. """
        return self.lib.monitorcommand(struct.pack("IIII", 1, 0, 0, 0), "IBBBBI", ("revision", "majorv", "minorv", "patchv", "swtypeid", "hwtypeid"))
    
    def getpacketsizeinfo(self):
        """ This returns the emBIOS max packet size information.
            It also sets the properties of the device object accordingly.
        """
        resp = self.lib.monitorcommand(struct.pack("IIII", 1, 1, 0, 0), "HHII", ("coutmax", "cinmax", "doutmax", "dinmax"))
        self.lib.dev.packetsizelimit['cout'] = resp.coutmax
        self.lib.dev.packetsizelimit['cin'] = resp.cinmax
        self.lib.dev.packetsizelimit['din'] = resp.dinmax
        self.lib.dev.packetsizelimit['dout'] = resp.doutmax
        return resp
    
    def getusermemrange(self):
        """ This returns the memory range the user has access to. """
        return self.lib.monitorcommand(struct.pack("IIII", 1, 2, 0, 0), "III", ("lower", "upper", None))
    
    def reset(self, force=False):
        """ Reboot the device """
        if force:
            return self.lib.monitorcommand(struct.pack("IIII", 2, 0, 0, 0))
        else:
            return self.lib.monitorcommand(struct.pack("IIII", 2, 1, 0, 0), "III", (None, None, None))
    
    def poweroff(self, force=False):
        """ Powers the device off. """
        if force:
            return self.lib.monitorcommand(struct.pack("IIII", 3, 0, 0, 0))
        else:
            return self.lib.monitorcommand(struct.pack("IIII", 3, 1, 0, 0), "III", (None, None, None))
    
    def read(self, addr, size):
        """ Reads the memory from location 'addr' with size 'size'
            from the device. This cares about too long packages
            and decides whether to use DMA or not.
        """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        din_maxsize = self.lib.dev.packetsizelimit["din"]
        data = ""
        (headsize, bodysize, tailsize) = self._alignsplit(addr, size, cin_maxsize, 16)
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
    
    def write(self, addr, data):
        """ Writes the data in 'data' to the location 'addr'
            in the memory of the device. This cares about too long packages
            and decides whether to use DMA or not.
        """
        cout_maxsize = self.lib.dev.packetsizelimit["cout"] - self.lib.headersize
        dout_maxsize = self.lib.dev.packetsizelimit["dout"]
        (headsize, bodysize, tailsize) = self._alignsplit(addr, len(data), cout_maxsize, 16)
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
    
    def readstring(self, addr, maxlength = 256):
        """ Reads a zero terminated string from memory 
            Reads only a maximum of 'maxlength' chars.
        """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        string = ""
        while (len(string) < maxlength or maxlength < 0):
            data = self._readmem(addr, min(maxlength - len(string), cin_maxsize))
            length = data.find("\0")
            if length >= 0:
                string += data[:length]
                break
            else:
                string += data
            addr += cin_maxsize
        return string
    
    def i2cread(self, index, slaveaddr, startaddr, size):
        """ Reads data from an i2c slave """
        data = ""
        for i in range(size):
            resp = self.lib.monitorcommand(struct.pack("IBBBBII", 8, index, slaveaddr, startaddr + i, 1, 0, 0), "III1s", (None, None, None, "data"))
            data += resp.data
        return data
    
    def i2cwrite(self, index, slaveaddr, startaddr, data):
        """ Writes data to an i2c slave """
        size = len(data)
        if size > 256 or size < 1:
            raise ValueError("Size must be a number between 1 and 256")
        if size == 256:
            size = 0
        return self.lib.monitorcommand(struct.pack("IBBBBII%ds" % size, 9, index, slaveaddr, startaddr, size, 0, 0, data), "III", (None, None, None))
    
    def usbcread(self):
        """ Reads one packet with the maximal cin size """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        resp = self.lib.monitorcommand(struct.pack("IIII", 10, cin_maxsize, 0, 0), "III%ds" % cin_maxsize, ("validsize", "buffersize", "queuesize", "data"))
        resp.data = resp.data[:resp.validsize]
        resp.maxsize = cin_maxsize
        return resp
    
    def usbcwrite(self, data):
        """ Writes data to the USB console """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        size = len(data)
        while len(data) > 0:
            writesize = min(cin_maxsize, len(data))
            resp = self.lib.monitorcommand(struct.pack("IIII%ds" % writesize, 11, writesize, 0, 0, data[:writesize]), "III", ("validsize", "buffersize", "freesize"))
            data = data[resp.validsize:]
        return size
    
    def cread(self, bitmask=0x1):
        """ Reads one packet with the maximal cin size from the device consoles
            identified with the specified bitmask
        """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        resp = self.lib.monitorcommand(struct.pack("IIII", 13, bitmask, cin_maxsize, 0), "III%ds" % cin_maxsize, ("size", None, None))
        resp.data = resp.data[size:]
        resp.maxsize = cin_maxsize
        return resp

    def cwrite(self, data, bitmask=0x1):
        """ Writes data to the device consoles 
            identified with the specified bitmask.
        """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        size = len(data)
        while len(data) > 0:
            writesize = min(cin_maxsize, len(data))
            resp = self.lib.monitorcommand(struct.pack("IIII%ds" % writesize, 12, bitmask, writesize, 0, data[:writesize]), "III", (None, None, None))
            data = data[writesize:]
        return size
    
    def cflush(self, bitmask):
        """ Flushes the consoles specified with 'bitmask' """
        return self.lib.monitorcommand(struct.pack("IIII", 14, bitmask, 0, 0), "III", (None, None, None))
    
    def getprocinfo(self):
        """ Gets current state of the scheduler """
        cin_maxsize = self.lib.dev.packetsizelimit["cin"] - self.lib.headersize
        # Get the size
        schedulerstate = self.lockscheduler()
        resp = self.lib.monitorcommand(struct.pack("IIII", 15, 0, 0, 0), "III", ("structver", "tablesize", None))
        tablesize = resp.tablesize
        size = tablesize
        structver = resp.structver
        offset = 0
        data = ""
        while size > 0:
            if size > cin_maxsize:
                readsize = cin_maxsize
            else:
                readsize = size
            resp = self.lib.monitorcommand(struct.pack("IIII", 15, offset, readsize, 0), "III%ds" % readsize, ("structver", "tablesize", None, "data"))
            data += resp.data
            offset += readsize
            size -= readsize
        self.lockscheduler(schedulerstate)
        threadstructsize = 120
        registersize = 32
        if len(data) % threadstructsize != 0:
            raise DeviceError("The thread struct is not a multiple of "+str(threadsturcsize)+"!")
        threadcount = len(data) / threadstructsize
        threads = []
        id = 0
        for thread in range(threadcount):
            offset = threadstructsize * thread
            threaddata = struct.unpack("<16IIIIIQIIIIIIIBBBB", data[offset:offset+threadstructsize])
            info = Bunch()
            info.id = id
            state = threaddata[17]
            info.state = libembiosdata.thread_state[state]
            if info.state == "THREAD_FREE":
                id += 1
                continue
            info.regs = Bunch()
            for register in range(16):
                info.regs["r"+str(register)] = threaddata[register]
            info.regs.cpsr = threaddata[16]
            info.nameptr = threaddata[18]
            if info.nameptr == 0:
                info.name = "Thread %d" % info.id
            else:
                info.name = self.readstring(info.nameptr)
            info.cputime_current = threaddata[19]
            info.cputime_total = threaddata[20]
            info.startusec = threaddata[21]
            info.queue_next_ptr = threaddata[22]
            info.timeout = threaddata[23]
            info.blocked_since = threaddata[24]
            info.blocked_by_ptr = threaddata[25]
            info.stackaddr = threaddata[26]
            info.err_no = threaddata[27]
            info.block_type = libembiosdata.thread_block[threaddata[28]]
            info.type = libembiosdata.thread_type[threaddata[29]]
            info.priority = threaddata[30]
            info.cpuload = threaddata[31]
            threads.append(info)
            id += 1
        return threads
    
    def lockscheduler(self, freeze=True):
        """ Freezes/Unfreezes the scheduler """
        resp = self.lib.monitorcommand(struct.pack("IIII", 16, 1 if freeze else 0, 0, 0), "III", ("before", None, None))
        return True if resp.before == 1 else False
    
    def unlockscheduler(self):
        """ Unfreezes the scheduler """
        return self.lib.monitorcommand(struct.pack("IIII", 16, 0, 0, 0), "III", ("before", None, None))
    
    def suspendthread(self, id, suspend=True):
        """ Suspends the thread with the specified id """
        resp = self.lib.monitorcommand(struct.pack("IIII", 17, 1 if suspend else 0, id, 0), "III", ("before", None, None))
        return True if resp.before == 1 else False
    
    def resumethread(self, id):
        """ Resumes the thread with the specified id """
        return self.lib.monitorcommand(struct.pack("IIII", 17, 0, id, 0), "III", ("before", None, None))
    
    def killthread(self, id):
        """ Kills the thread with the specified id """
        return self.lib.monitorcommand(struct.pack("IIII", 18, id, 0, 0), "III", ("before", None, None))
    
    def createthread(self, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state):
        """ Creates a thread with the specified attributes """
        if threadtype == "user":
            threadtype = 0
        elif threadtype == "system":
            threadtype = 1
        else:
            raise ValueError("Threadtype must be either 'system' or 'user'")
        if priority > 256 or priority < 0:
            raise ValueError("Priority must be a number between 0 and 256")
        if state == "ready":
            state = 0
        elif state == "suspended":
            state = 1
        else:
            raise ValueError("State must be either 'ready' or 'suspended'")
        resp = self.lib.monitorcommand(struct.pack("IIIIIIII", 19, nameptr, entrypoint, stackptr, stacksize, threadtype, priority, state), "III", (id, None, None))
        if resp.id < 0:
            raise DeviceError("The device returned the error code "+str(resp.id))
        return resp
    
    def flushcaches(self):
        """ Flushes the CPU instruction and data cache """
        return self.lib.monitorcommand(struct.pack("IIII", 20, 0, 0, 0), "III", (None, None, None))
    
    def execimage(self, addr):
        """ Runs the emBIOS app at 'addr' """
        return self.lib.monitorcommand(struct.pack("IIII", 21, addr, 0, 0), "III", ("excecimage", None, None))
    
    def run(self, app):
        """ Uploads and runs the emBIOS app in the string 'app' """
        try:
            appheader = struct.unpack("<8sIIIIIIIIII", app[:48])
        except struct.error:
            raise ArgumentError("The specified file is not an emBIOS application")
        header = appheader[0]
        if header != "emBIexec":
            raise ArgumentError("The specified file is not an emBIOS application")
        baseaddr = appheader[2]
        threadnameptr = appheader[8]
        nameptr = threadnameptr - baseaddr
        name = ""
        while True:
            char = app[nameptr:nameptr+1]
            try:
                if ord(char) == 0:
                    break
            except TypeError:
                raise ArgumentError("The specified file is not an emBIOS application")
            name += char
            nameptr += 1
        usermem = self.getusermemrange()
        if usermem.lower > baseaddr or usermem.upper < baseaddr + len(app):
            raise ArgumentError("The baseaddress of the specified emBIOS application is out of range of the user memory range on the device. Are you sure that this application is compatible with your device?")
        self.write(baseaddr, app)
        self.execimage(baseaddr)
        return Bunch(baseaddr=baseaddr, name=name)
    
    
    def bootflashread(self, memaddr, flashaddr, size):
        """ Copies the data in the bootflash at 'flashaddr' of the specified size
            to the memory at addr 'memaddr'
        """
        return self.lib.monitorcommand(struct.pack("IIII", 22, memaddr, flashaddr, size), "III", (None, None, None))
    
    def bootflashwrite(self, memaddr, flashaddr, size):
        """ Copies the data in the memory at 'memaddr' of the specified size
            to the boot flash at addr 'flashaddr'
        """
        return self.lib.monitorcommand(struct.pack("IIII", 23, memaddr, flashaddr, size), "III", (None, None, None))
    
    def execfirmware(self, addr):
        """ Executes the firmware at 'addr' and passes all control to it. """
        return self.lib.monitorcommand(struct.pack("IIII", 24, addr, 0, 0), "III", (None, None, None))
    
    def aesencrypt(self, addr, size, keyindex):
        """ Encrypts the buffer at 'addr' with the specified size
            with the hardware AES key index 'keyindex'
        """
        return self.lib.monitorcommand(struct.pack("IBBHII", 25, 1, 0, keyindex, addr, size), "III", (None, None, None))
    
    def aesdecrypt(self, addr, size, keyindex):
        """ Decrypts the buffer at 'addr' with the specified size
            with the hardware AES key index 'keyindex'
        """
        return self.lib.monitorcommand(struct.pack("IBBHII", 25, 0, 0, keyindex, addr, size), "III", (None, None, None))
    
    def hmac_sha1(self, addr, size, destination):
        """ Generates a HMAC-SHA1 hash of the buffer and saves it to 'destination' """
        return self.lib.monitorcommand(struct.pack("IIII", 26, addr, size, destination), "III", (None, None, None))

    def ipodnano2g_getnandinfo(self):
        """ Target-specific function: ipodnano2g
            Gathers some information about the NAND chip used
        """
        return self.lib.monitorcommand(struct.pack("IIII", 0xffff0001, 0, 0, 0), "IHHHH", ("type", "pagesperblock", "banks", "userblocks", "blocks"))
    
    def ipodnano2g_nandread(self, addr, start, count, doecc, checkempty):
        """ Target-specific function: ipodnano2g
            Reads data from the NAND chip into memory
        """
        return self.lib.monitorcommand(struct.pack("IIII", 0xffff0002, addr | (0x80000000 if doecc != 0 else 0) | (0x40000000 if checkempty != 0 else 0), start, count), "III", (None, None, None))
    
    def ipodnano2g_nandwrite(self, addr, start, count, doecc):
        """ Target-specific function: ipodnano2g
            Writes data to the NAND chip
        """
        return self.lib.monitorcommand(struct.pack("IIII", 0xffff0003, addr | (0x80000000 if doecc != 0 else 0), start, count), "III", (None, None, None))
    
    def ipodnano2g_nanderase(self, addr, start, count):
        """ Target-specific function: ipodnano2g
            Erases blocks on the NAND chip and stores the results to memory
        """
        return self.lib.monitorcommand(struct.pack("IIII", 0xffff0004, addr, start, count), "III", (None, None, None))
    

class Lib(object):
    def __init__(self):
        self.idVendor = 0xFFFF
        self.idProduct = 0xE000
        
        self.headersize = 0x10
        
        self.connect()
    
    def connect(self):
        self.dev = Dev(self.idVendor, self.idProduct)
        self.connected = True
    
    def monitorcommand(self, cmd, rcvdatatypes=None, rcvstruct=None):
        self.dev.cout(cmd)
        if rcvdatatypes:
            rcvdatatypes = "I" + rcvdatatypes # add the response
            data = self.dev.cin(struct.calcsize(rcvdatatypes))
            data = struct.unpack(rcvdatatypes, data)
            response = data[0]
            if libembiosdata.responsecodes[response] == "ok":
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
            elif libembiosdata.responsecodes[response] == "unsupported":
                raise DeviceError("The device does not support this command.")
            elif libembiosdata.responsecodes[response] == "invalid":
                raise DeviceError("Invalid command! This should NOT happen!")
            elif libembiosdata.responsecodes[response] == "busy":
                raise DeviceError("Device busy")


class Dev(object):
    def __init__(self, idVendor, idProduct):
        self.idVendor = idVendor
        self.idProduct = idProduct
        
        self.interface = 0
        self.timeout = 100

        self.connect()
        self.findEndpoints()
        
        self.packetsizelimit = {}
        self.packetsizelimit['cout'] = None
        self.packetsizelimit['cin'] = None
        self.packetsizelimit['dout'] = None
        self.packetsizelimit['din'] = None
    
    def __del__(self):
        self.disconnect()
    
    def findEndpoints(self):
        epcounter = 0
        self.endpoint = {}
        for cfg in self.dev:
            for intf in cfg:
                for ep in intf:
                    if epcounter == 0:
                        self.endpoint['cout'] = ep.bEndpointAddress
                    elif epcounter == 1:
                        self.endpoint['cin'] = ep.bEndpointAddress
                    elif epcounter == 2:
                        self.endpoint['dout'] = ep.bEndpointAddress
                    elif epcounter == 3:
                        self.endpoint['din'] = ep.bEndpointAddress
                    epcounter += 1
        if epcounter <= 3:
            raise DeviceError("Not all endpoints found in the descriptor. Only "+str(epcounter)+" found, we need 4")
    
    def connect(self):
        self.dev = usb.core.find(idVendor=self.idVendor, idProduct=self.idProduct)
        if self.dev is None:
            raise DeviceNotFoundError()
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
        if self.packetsizelimit['cout'] and len(data) > self.packetsizelimit['cout']:
            raise SendError("Packet too big")
        return self.send(self.endpoint['cout'], data)
    
    def cin(self, size):
        if self.packetsizelimit['cin'] and size > self.packetsizelimit['cin']:
            raise ReceiveError("Packet too big")
        return self.receive(self.endpoint['cin'], size)
    
    def dout(self, data):
        if self.packetsizelimit['dout'] and len(data) > self.packetsizelimit['dout']:
            raise SendError("Packet too big")
        return self.send(self.endpoint['dout'], data)
    
    def din(self, size):
        if self.packetsizelimit['din'] and size > self.packetsizelimit['din']:
            raise ReceiveError("Packet too big")
        return self.receive(self.endpoint['din'], size)


if __name__ == "__main__":
    # Some tests
    import sys
    embios = Embios()
    resp = embios.getversioninfo()
    sys.stdout.write("Embios device version information: " + libembiosdata.swtypes[resp.swtypeid] + " v" + str(resp.majorv) + "." + str(resp.minorv) + 
                     "." + str(resp.patchv) + " r" + str(resp.revision) + " running on " + libembiosdata.hwtypes[resp.hwtypeid] + "\n")
    resp = embios.getusermemrange()
    sys.stdout.write("Usermemrange: "+hex(resp.lower)+" - "+hex(resp.upper)+"\n")
    memaddr = resp.lower
    maxlen = resp.upper - resp.lower
    f = open("./embios.py", "rb")
    sys.stdout.write("Loading test file (embios.py) to send over USB...\n")
    datastr = f.read()[:maxlen]
    sys.stdout.write("Sending data...\n")
    embios.write(memaddr, datastr)
    sys.stdout.write("Encrypting data with the hardware key...\n")
    embios.aesencrypt(memaddr, len(datastr), 0)
    sys.stdout.write("Reading data back and saving it to 'libembios-test-encrypted.bin'...\n")
    f = open("./libembios-test-encrypted.bin", "wb")
    f.write(embios.read(memaddr, len(datastr)))
    sys.stdout.write("Decrypting the data again...\n")
    embios.aesdecrypt(memaddr, len(datastr), 0)
    sys.stdout.write("Reading data back from device...\n")
    readdata = embios.read(memaddr, len(datastr))
    if readdata == datastr:
        sys.stdout.write("Data matches!")
    else:
        sys.stdout.write("Data does NOT match. Something got wrong")