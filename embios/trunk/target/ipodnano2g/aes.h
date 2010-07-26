//
//
//    Copyright 2009 TheSeven
//
//
//    This file is part of the Linux4Nano toolkit.
//
//    TheSeven's iBugger is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    TheSeven's iBugger is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with the Linux4Nano toolkit.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __AES_H__
#define __AES_H__

#include <toolkit.h>


void aes_encrypt(uint32_t keytype, void* data, uint32_t size);
void aes_decrypt(uint32_t keytype, void* data, uint32_t size);


#endif
