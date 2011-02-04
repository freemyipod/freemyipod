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
    This file includes some reusable functions and classes that might be useful
    to all python scripts
"""

import sys
from ctypes import *
from _ctypes import _SimpleCData
import libemcoredata

class Logger(object):
    """
        Simple stdout/stderr/file logger.
        Loglevel 3 is most verbose, Loglevel 0: Only log something if there is an error.
        Loglevel -1 means that nothing is logged.
        The write function doesn't care about the loglevel and always logs everything.
    """
    def __init__(self, loglevel = 2, target = "stderr", logfile = "tools.log"):
        """
            loglevel: Possible values: 0 (only errors), 1 (warnings), 2 (info,
                      recommended for production use), 3 and more (debug)
            logfile: File to log to if using the target = "file"
            target: Default logging target. Can be "stdout", "file" or "string"
        """
        self.loglevel = loglevel
        self.logfile = logfile
        self.target = target
        
    def write(self, text, indent = 0, target = None):
        if self.loglevel >= 0:
            if target is None: target = self.target
            text = (indent * " ") + text
            text = text.replace("\n", "\n" + (indent * " "), text.count("\n") - 1)
            if target == "stdout":
                sys.stderr.write(text)
            if target == "stderr":
                sys.stderr.write(text)
            elif target == "file":
                with open(self.logfile, 'a') as f:
                    f.write(text)
                    f.close()
            elif target == "string":
                return text
    
    def debug(self, text, indent = 0, target = None):
        if self.loglevel >= 3:
            self.write("DEBUG: " + text, indent, target)
    
    def info(self, text, indent = 0, target = None):
        if self.loglevel >= 2:
            self.write(text, indent, target)
    
    def warn(self, text, indent = 0, target = None):
        if self.loglevel >= 1:
            self.write("WARNING: " + text, indent, target)
    
    def error(self, text, indent = 0, target = None):
        if self.loglevel >= 0:
            self.write("ERROR: " + text, indent, target)


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


class c_enum(_SimpleCData):
    """
        Resembles the enum datatype from C with an 8 bit size.
        Returns the associated string of a value with c_enum[i]
        Returns the current value of the associated value as c_enum.__repr__()
        Comparison operators work with strings and values at the same time.
        
        ATTENTION: You can not really see if this is initialized or not.
        If it is uninitialized it will return the first entry of the enum.
        While this may be circumvented by changing the default value to
        something else this will not work if the enum is placed inside a
        ctypes structure as the __init__() method will not be called then.
    """    
    _type_ = c_uint8._type_
    
    def __init__(self, value = 0):
        if type(value) == str:
            value = getattr(self, value)
        _SimpleCData.__init__(self, value)
        self[value]
    
    def __getattr__(self, name):
        if name == "value":
            return self.value
        for key, value in enumerate(self._fields_):
            if value == name:
                return key
    
    def __getitem__(self, lookupkey):
        for key, value in enumerate(self._fields_):
            if key == lookupkey:
                return value
        raise IndexError("Value %d not in range of possible enum values for %s!" % (lookupkey, self.__class__.__name__))
    
    def __str__(self):
        return self[self.value]
    
    def __repr__(self):
        return self.__str__()
    
    def __int__(self):
        return self.value
    
    def __eq__(self, other):
        if type(other) == str:
            try: return getattr(self, other) == self.value
            except AttributeError: return False
        else:
            return self.value == other
    
    def __lt__(self, other):
        if type(other) == str:
            try: return self.value < getattr(self, other)
            except AttributeError: return False
        else:
            return self.value < other
    
    def __gt__(self, other):
        if type(other) == str:
            try: return  self.value > getattr(self, other)
            except AttributeError: return False
        else:
            return self.value > other
    
    def __le__(self, other):
        if self.value == other or self.value < other:
            return True
        return False
    
    def __ge__(self, other):
        if self.value == other or self.value > other:
            return True
        return False
    
    def __ne__(self, other):
        if self.value == other:
            return False
        return True


class ExtendedCStruct(LittleEndianStructure):
    """
        This is a subclass of the LittleEndianStructure.
        It implements functions to easily convert
        structures to/from strings and Bunches.
    """
    def _from_bunch(self, bunch):
        for field, _ in self._fields_:
            if field in bunch:
                setattr(self, field, getattr(bunch, field))
    
    def _to_bunch(self):
        bunch = Bunch()
        for field, _ in self._fields_:
            setattr(bunch, field, getattr(self, field))
        return bunch
    
    def _from_string(self, string):
        memmove(addressof(self), string, sizeof(self))
    
    def _to_string(self):
        return string_at(addressof(self), sizeof(self))



class Error(Exception):
    def __init__(self, value=None):
        self.value = value
    def __str__(self):
        if self.value != None:
            return repr(self.value)


def gethwname(id):
    try:
        hwtype = libemcoredata.hwtypes[id]
    except KeyError:
        hwtype = "UNKNOWN (ID = " + self._hex(id) + ")"
    return hwtype


def trimdoc(docstring):
    """
        Trims whitespace from docstrings
    """
    if not docstring:
        return ''
    # Convert tabs to spaces (following the normal Python rules)
    # and split into a list of lines:
    lines = docstring.expandtabs().splitlines()
    # Determine minimum indentation (first line doesn't count):
    indent = sys.maxint
    for line in lines[1:]:
        stripped = line.lstrip()
        if stripped:
            indent = min(indent, len(line) - len(stripped))
    # Remove indentation (first line is special):
    trimmed = [lines[0].strip()]
    if indent < sys.maxint:
        for line in lines[1:]:
            trimmed.append(line[indent:].rstrip())
    # Strip off trailing and leading blank lines:
    while trimmed and not trimmed[-1]:
        trimmed.pop()
    while trimmed and not trimmed[0]:
        trimmed.pop(0)
    # Return a single string:
    return '\n'.join(trimmed)


def getfuncdoc(funcdict):
    """
        Extracts important information from a dict of functions like the
        docstring and arguments and returns them in a human readable format
    """
    import inspect
    import re
    functions = Bunch()
    for function in funcdict:
        function = funcdict[function].func
        docinfo = Bunch()
        name = function.__name__
        args = inspect.getargspec(function)[0]
        docinfo['varargs'] = False
        if inspect.getargspec(function)[1]:
            docinfo['varargs'] = True
        kwargvalues = inspect.getargspec(function)[3]
        kwargs = Bunch()
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
            docinfo['documentation'] = trimdoc(function.__doc__)
        else:
            docinfo['documentation'] = None
        functions[name] = docinfo
    return functions


def gendoc(funcdict, indentwidth = 4, logtarget = "string"):
    logger = Logger()
    doc = getfuncdoc(funcdict)
    ret = ""
    for function in sorted(doc.items()):
        function = function[0]
        ret += logger.log("def " + function + "(", target = logtarget)
        counter = 0
        if doc[function]['args']:
            for arg in doc[function]['args']:
                if counter > 0:
                    sys.stdout.write(", ")
                counter += 1
                ret += logger.log(arg, target = logtarget)
        if doc[function]['kwargs']:
            for kwarg, kwargvalue in doc[function]['kwargs'].items():
                if counter > 0:
                    sys.stdout.write(", ")
                counter += 1
                ret += logger.log(kwarg + "=" + str(kwargvalue), target = logtarget)
        if doc[function]['varargs']:
            ret += logger.log("*argv", target = logtarget)
        ret += logger.log("):\n", target = logtarget)
        if doc[function]['documentation']:
            ret += logger.log("\"\"\"\n", indent = indentwidth, target = logtarget)
            ret += logger.log(trimdoc(doc[function]['documentation']) + "\n", indent = 2 * indentwidth, target = logtarget)
            ret += logger.log("\"\"\"\n", indent = indentwidth, target = logtarget)
        ret += logger.log("\n", target = logtarget)
    return ret