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


#include "global.h"
#include "console.h"


void handle_panic()
{
  while(1);
}

void panic(const char* string)
{
  cputs(1, "\n*PANIC*\n");
  cputs(1, string);
  cputc(1, '\n');
  handle_panic();
}

void panicf(const char* string, ...)
{
  va_list ap;
  cputs(1, "\n*PANIC*\n");
  va_start(ap, string);
  cvprintf(1, string, ap);
  va_end(ap);
  cputc(1, '\n');
  handle_panic();
}

void __div0()
{
  panic("Division by zero!");
}
