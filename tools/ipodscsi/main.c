//
//
//    Copyright 2011 TheSeven
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


#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ddk/ntddscsi.h>
#include "build/version.h"


struct scsi_cmd
{
    SCSI_PASS_THROUGH_DIRECT sptd;
    unsigned char sense[14];
    unsigned char data[65536];
} cmd;

char devname[] = "\\\\.\\?:";


int usage(char const* msg, char const* msgarg, int argc, char const* const* argv)
{
    printf(msg, msgarg);
    printf("\r\n"
           "\r\n"
           "Usage: %s <drive>: <type> <command> [options...]\r\n"
           "\r\n"
           "Available device types: ipod6g\r\n"
           "\r\n"
           "Commands for ipod6g:\r\n"
           "  writefirmware [-p] <firmware.mse>\r\n"
           "    -r: Reboot device\r\n"
           "    -p: Repartition device\r\n", argv[0]);
    return 1;
}

void print_last_error(char* text, bool force)
{
    DWORD dw = GetLastError(); 
    if (!dw && !force) return;
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("\r\n%s: Error %d: %s\n", text, dw, lpMsgBuf);
}

int send_cmd(HANDLE dev, struct scsi_cmd* req, int size)
{
    SetLastError(0);
    DWORD bytes;
	if (!DeviceIoControl(dev, IOCTL_SCSI_PASS_THROUGH_DIRECT, req, size, req, size, &bytes, NULL))
    {
        print_last_error("DeviceIoControl", true);
        return 0;
	}
	return 1;
}

int cmd_ipod6g_writefirmware(HANDLE dev, int argc, char const* const* argv)
{
    int arg = 4;
    char const* mse_filename = NULL;
    int repartition = 0;
    int reboot = 0;
    while (arg < argc)
    {
        if (argv[arg][0] == '-')
        {
            if (!strcmp(argv[arg], "-p")) repartition = 1;
            else if (!strcmp(argv[arg], "-r")) reboot = 1;
            else return usage("Unknown option: %s", argv[arg], argc, argv);
        }
        else if (mse_filename) return usage("Excessive argument: %s", argv[arg], argc, argv);
        else mse_filename = argv[arg];
        arg++;
    }
    if (!mse_filename) return usage("No MSE file name specified", NULL, argc, argv);
    SetLastError(0);
    HANDLE f = CreateFile(mse_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (!f || f == (HANDLE)-1)
    {
        print_last_error("Error opening MSE file: CreateFile", true);
        return 2;
    }
    LARGE_INTEGER size;
    SetLastError(0);
    if (!GetFileSizeEx(f, &size))
    {
        print_last_error("Error getting MSE file size: GetFileSizeEx", true);
        return 2;
    }
    int bytes = size.LowPart;
    if (bytes & 0xfff)
    {
        printf("MSE file size must be a multiple of 4096\r\n");
        return 2;
    }
    int sectors = bytes >> 12;
    
    if (repartition)
    {
        printf("Repartitioning...");
        int partsize = sectors << 2;
        cmd.sptd.CdbLength = 6;
        cmd.sptd.Cdb[0] = 0xc6;
        cmd.sptd.Cdb[1] = 0x94;
        cmd.sptd.Cdb[2] = (partsize >> 24) & 0xff;
        cmd.sptd.Cdb[3] = (partsize >> 16) & 0xff;
        cmd.sptd.Cdb[4] = (partsize >> 8) & 0xff;
        cmd.sptd.Cdb[5] = (partsize) & 0xff;
        cmd.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
        cmd.sptd.DataTransferLength = 0;
        cmd.sptd.TimeOutValue = 60000;
        if (!send_cmd(dev, &cmd, sizeof(cmd))) return 2;
        printf(" done\r\n");
    }

    printf("Initiating firmware transfer...");
    cmd.sptd.CdbLength = 2;
    cmd.sptd.Cdb[0] = 0xc6;
    cmd.sptd.Cdb[1] = 0x90;
    cmd.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
    cmd.sptd.DataTransferLength = 0;
    cmd.sptd.TimeOutValue = 1000;
    if (!send_cmd(dev, &cmd, sizeof(cmd))) return 2;
    printf(" done\r\n");

    printf("Writing firmware...");
    while (sectors)
    {
        int tsize = sectors > 0x10 ? 0x10 : sectors;
        int got = 0;
        while (got < (tsize << 12))
        {
            SetLastError(0);
            DWORD b;
            if (!ReadFile(f, &cmd.data[got], (tsize << 12) - got, &b, NULL) || !b)
            {
                print_last_error("Error reading from MSE file: ReadFile", true);
                return 2;
            }
            got += b;
        }
        cmd.sptd.CdbLength = 4;
        cmd.sptd.Cdb[0] = 0xc6;
        cmd.sptd.Cdb[1] = 0x91;
        cmd.sptd.Cdb[2] = 0x00;
        cmd.sptd.Cdb[3] = tsize;
        cmd.sptd.DataIn = SCSI_IOCTL_DATA_OUT;
        cmd.sptd.DataTransferLength = tsize << 12;
        cmd.sptd.TimeOutValue = 5000;
        if (!send_cmd(dev, &cmd, sizeof(cmd))) return 2;
        sectors -= tsize;
        printf(".");
    }
    printf(" done\r\n");

    if (reboot)
    {
        printf("Rebooting device...");
        cmd.sptd.CdbLength = 6;
        cmd.sptd.Cdb[0] = 0x1b;
        cmd.sptd.Cdb[1] = 0x00;
        cmd.sptd.Cdb[2] = 0x00;
        cmd.sptd.Cdb[3] = 0x00;
        cmd.sptd.Cdb[4] = 0x02;
        cmd.sptd.Cdb[5] = 0x00;
        cmd.sptd.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
        cmd.sptd.DataTransferLength = 0;
        cmd.sptd.TimeOutValue = 10000;
        if (!send_cmd(dev, &cmd, sizeof(cmd))) return 2;
        printf(" done\r\n");
    }

    CloseHandle(f);
    return 0;
}

int cmd_ipod6g(HANDLE dev, int argc, char const* const* argv)
{
    if (!strcmp(argv[3], "writefirmware")) return cmd_ipod6g_writefirmware(dev, argc, argv);
    return usage("Unknown command for device type type ipod6g: %s", argv[3], argc, argv);
}

int main(int argc, char const* const* argv)
{
    printf("iPodSCSI v. " VERSION " r" VERSION_SVN " - Copyright 2011 by Michael Sparmann (TheSeven)\r\n"
           "This is free software; see the source for copying conditions.  There is NO\r\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\r\n"
           "\r\n");
    if (argc < 4) return usage("Not enough arguments specified", NULL, argc, argv);

    if (strlen(argv[1]) != 2 || argv[1][1] != ':') return usage("Bad drive letter: %s", argv[1], argc, argv);
    devname[4] = argv[1][0];
    SetLastError(0);
    HANDLE dev = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!dev || dev == (HANDLE)-1)
    {
        print_last_error("Error opening SCSI device: CreateFile", true);
        return 2;
    }
    cmd.sptd.Length = sizeof(cmd.sptd);
    cmd.sptd.SenseInfoOffset = sizeof(cmd.sptd);
    cmd.sptd.SenseInfoLength = 14;
    cmd.sptd.DataBuffer = cmd.data;
    
    if (!strcmp(argv[2], "ipod6g")) return cmd_ipod6g(dev, argc, argv);
    return usage("Unknown device type: %s", argv[2], argc, argv);
    
    CloseHandle(dev);
}
