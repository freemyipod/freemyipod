#!/usr/bin/env python3
# Copyright (C) 2026 Vikotr Krapivin (standalone bundle extraction and adaptation)
#
# This file is part of emCORE.
#
# emCORE is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# emCORE is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with emCORE. If not, see <http://www.gnu.org/licenses/>.

"""Shared nano5g USB session; commands execute once unless retries are requested."""
import struct
import time
from .protocol import Cmd

class ToolError(Exception):
    pass

class Session:
    def __init__(self, settle=0.05, poll=0.05, verbose=False, ep0=True, bulk_write=False):
        import libemcore
        self.settle, self.poll = settle, poll
        self.verbose, self.ep0 = verbose, ep0
        self.retries = 0
        self.em = libemcore.Emcore()
        if bulk_write and (not self.em.lib.bulkout or (not ep0 and not self.em.lib.bulkin)):
            self.em.close()
            raise ToolError("requested bulk transport is unavailable on this device")
        if ep0:
            self.em.lib.bulkin = False
        self.em.bulk_write = bulk_write

    def close(self):
        self.em.close()

    def _io(self, fn, *args):
        return fn(*args)

    def raw(self, command, reply_size=16):
        result = self.em.lib.monitorcommand(command, "III%ds" % (reply_size - 16))
        return struct.pack("<IIII%ds" % (reply_size - 16), *result)

    def cmd(self, command, a=0, b=0, d=0):
        return self._io(self.em.lib.monitorcommand,
                        struct.pack("<IIII", command, a, b, d), "III")[1:]

    def read(self, addr, size):
        return self._io(self.em.read, addr, size)

    def write_memory(self, addr, data):
        return self.em.write(addr, bytes(data))

    def heartbeat(self):
        return self.em.get_monitor_heartbeat()

    def no_retry(self):
        return self

    sleep = staticmethod(time.sleep)
    now = staticmethod(time.monotonic)
