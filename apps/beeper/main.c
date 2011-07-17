//
//
//    Copyright 2011 TheSeven, user890104
//
//
//    This file is part of emCORE.
//
//    emCORE is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as
//    published by the Free Software Foundation, either version 2 of the
//    License, or (at your option) any later version.
//
//    emCORE is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//    See the GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with emCORE.  If not, see <http://www.gnu.org/licenses/>.
//
//


#include "emcoreapp.h"
#include "beep.h"

struct note
{
    unsigned int cycles;
    unsigned int length;
};

static void main()
{
    if (0x47324e49 != get_platform_id()) // IN2G
    {
        cputs(3, "Your device is not supported!\n");
        return;
    }
    
    size_t size;
    int fd;
    struct note *buf;
    unsigned int i, count;

    fd = file_open("/.apps/beeper/song.dat", O_RDONLY);
    
    if (fd <= 0)
    {
        cputs(3, "Unable to open /.apps/beeper/song.dat!\n");
        return;
    }
    
    cputs(3, "File opened\n");
    size = filesize(fd);
        
    if (0 == size)
    {
        close(fd);
        cputs(3, "Unable to get file size or file is empty!\n");
        return;
    }
    
    cprintf(3, "File size: %d bytes\n", size);
    
    if (0 != size % 8)
    {
        close(fd);
        cputs(3, "Invalid file, unable to continue!\n");
        return;
    }
    
    buf = memalign(0x10, size);
    
    if (!buf)
    {
        close(fd);
        cputs(3, "Memory allocation failed!\n");
        return;
    }
    
    if (size != read(fd, buf, size))
    {
        free(buf);
        close(fd);
        cputs(3, "Read error!\n");
        return;
    }
    
    cputs(3, "Read successful\n");
    close(fd);
    cputs(3, "File closed\n");
    
    count = size / 8;
    
    for (i = 0; i < count; ++i)
    {
        if (0 == buf[i].cycles)
        {
            sleep(buf[i].length);
            continue;
        }
        
        singlebeep(buf[i].cycles, buf[i].length);
    }
    
    free(buf);
}

EMCORE_APP_HEADER("Beeper", main, 127)
