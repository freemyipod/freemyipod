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
#include <windows.h>
#include <stdbool.h>
#include <inttypes.h>


struct controlreq {
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
	unsigned char data[];
};


extern char dfuimage[];
extern uint32_t dfuimage_size;


void print_last_error(char* text, bool force)
{
    DWORD dw = GetLastError(); 
    if (!dw && !force) return;
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("%s: Error %d: %s\n", text, dw, lpMsgBuf);
}

int control(HANDLE dfu, struct controlreq* req, int size, bool silent)
{
    DWORD count;
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
    SetLastError(0);
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	DeviceIoControl(dfu, 0x2200A0, req, size, req, size, NULL, &overlapped);
	WaitForSingleObject(overlapped.hEvent, 1000);
	DWORD rc = GetOverlappedResult(dfu, &overlapped, &count, FALSE);
	CloseHandle(overlapped.hEvent);
	if (rc <= 0)
    {
        print_last_error("DeviceIoControl", true);
        count = -1;
        CancelIo(dfu);
        if (!silent) MessageBox(0, "DFU transfer failed!", "Error", MB_OK);
	}
	return count;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SetLastError(0);
    HANDLE dfu = CreateFile("\\\\?\\usb#vid_05ac&pid_1223#87020000000001#{b8085869-feb9-404b-8cb1-1e5c14fa8c54}\\0001",
                            GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
	if (!dfu || dfu == (HANDLE)-1)
    {
        print_last_error("CreateFile", true);
        MessageBox(0, "Could not open DFU device!", "Error", MB_OK);
        return 1;
    }
    unsigned int buf[514];
    struct controlreq* req = (struct controlreq*)buf;
    int i, j;
    for (i = 0; i < 256; i++)
    {
        buf[i] = i;
        for (j = 0; j < 8; j++)
        {
            if (buf[i] & 1) buf[i] = (buf[i] >> 1) ^ 0xedb88320;
            else buf[i] >>= 1;
        }
    }
    unsigned char* ptr = dfuimage;
    int left = dfuimage_size + 4;
    int packet = 0;
    dfuimage_size = 0xffffffff;
    for (i = 0; i < left - 4; i++)
        dfuimage_size = (dfuimage_size >> 8) ^ buf[(dfuimage_size ^ ptr[i]) & 0xff];
    while (left)
    {
        int size = left > 2048 ? 2048 : left;
        req->bmRequestType = 0x21;
        req->bRequest = 1;
        req->wValue = packet;
        req->wIndex = 0;
        req->wLength = size;
        memcpy(req->data, ptr, size);
        if (control(dfu, req, size + 8, false) != size + 8) return 2;
        req->data[4] = 0;
        while (req->data[4] != 5)
        {
            req->bmRequestType = 0xa1;
            req->bRequest = 3;
            req->wValue = 0;
            req->wIndex = 0;
            req->wLength = 6;
            req->data[4] = 0;
            if (control(dfu, req, 14, false) != 14) return 3;
        }
        ptr += size;
        left -= size;
        packet++;
    }
    req->bmRequestType = 0x21;
    req->bRequest = 1;
    req->wValue = packet;
    req->wIndex = 0;
    req->wLength = 0;
    if (control(dfu, req, 8, false) != 8) return 4;
    int timeout = 20;
    req->data[4] = 0;
    while (req->data[4] != 2 && timeout-- > 0)
    {
        Sleep(100);
        req->bmRequestType = 0xa1;
        req->bRequest = 3;
        req->wValue = 0;
        req->wIndex = 0;
        req->wLength = 6;
        req->data[4] = 0;
        if (control(dfu, req, 14, true) != 14) break;
    }
    if (req->data[4] == 2)
    {
        printf("DFU payload was rejected with code: %02X %02X\n", req->data[4], req->data[0]);
        MessageBox(0, "DFU payload was rejected!", "Error", MB_OK);
    }
    else MessageBox(0, "UMSboot has been launched!", "Success", MB_OK);
    return 0;
}
