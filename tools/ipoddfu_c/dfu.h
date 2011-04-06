//
//
//    Copyright 2011 user890104
//
//
//    This file is part of ipoddfu.
//
//    ipoddfu is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    ipoddfu is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with ipoddfu.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __DFU_H__
#define __DFU_H__

int dfu_write(const unsigned char i, const unsigned int length, const unsigned char *data);
int dfu_read(unsigned char result[6]);
int dfu_send(const unsigned long int size, const unsigned char *data);

#endif
