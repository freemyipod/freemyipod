//
//
//    Copyright 2010 TheSeven
//
//
//    This file is part of emBIOS.
//
//    emBIOS is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emBIOS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emBIOS.  If not, see <http://www.gnu.org/licenses/>.
//
//


#ifndef __I2C_H__
#define __I2C_H__


#include "global.h"


void i2c_init() INITCODE_ATTR;
void i2c_send(uint32_t bus, uint32_t device, uint32_t address, const uint8_t* data, uint32_t length);
void i2c_recv(uint32_t bus, uint32_t device, uint32_t address, uint8_t* data, uint32_t length);
void i2c_sendbyte(uint32_t bus, uint32_t device, uint32_t address, uint32_t data);
uint8_t i2c_recvbyte(uint32_t bus, uint32_t device, uint32_t address);


#endif
