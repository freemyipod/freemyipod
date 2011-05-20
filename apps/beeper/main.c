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

void playsong(void *buf, size_t size);

static void main()
{
    if (0x47324e49 != get_platform_id()) // IN2G
    {
        cputs(3, "Your device is not supported!\n");
        return;
    }
    
    size_t size;
    int fd;
    void *buf;
    unsigned char play;

    play = 0;
    fd = file_open("/song.dat", O_RDONLY);
    
    if (fd <= 0)
    {
        cputs(3, "Unable to open /song.dat!\n");
        return;
    }
    
    cputs(3, "File opened\n");
    size = filesize(fd);
        
    if (size <= 0)
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
    playsong(buf, size);
    free(buf);
}

void playsong(void *buf, size_t size) {
    int i;
    unsigned int cycles[size / 8], lengths[size / 8];
    
    for (i = 0; i < size / 8; ++i)
    {
        cycles[i] = *((unsigned int *)(buf) + i + i);
        lengths[i] = *((unsigned int *)(buf) + i + i + 1);
    }
    
    for (i = 0; i < size / 8; ++i)
    {
        if (0 == cycles[i])
        {
            sleep(lengths[i]);
            continue;
        }
        
        singlebeep(cycles[i], lengths[i]);
    }
}

EMCORE_APP_HEADER("Beeper", main, 127)
