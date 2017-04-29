//
//
//    Copyright 2011 TheSeven
//    Copyright 2012 njsg
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

// This is a linux-based implementation of ipodscsi, originally
// implemented by TheSeven for Microsoft Windows.
//
// Currently, this only works for iPod 6g/Classic, but it should also
// work with minor modifications for, e.g., Nano 4g, which uses a
// similar firmware update mechanism.

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>
#include <stdlib.h>

#include "build/version.h"

#define APPLE_PREFIX 0xc6
#define CMD_INIT_FIRMWARE 0x90
#define CMD_SEND_FIRMWARE 0x91
#define CMD_PARTITION 0x94

// Return a file descriptor for the SCSI device, after checking if it
// is really a SCSI-generic device.
int open_scsi(const char * scsi_device) {
  int version;
  int sg_fd;
  
  // Open SCSI device 
  if ((sg_fd = open(scsi_device, O_RDWR)) < 0) 
  {
    perror("error opening given file name");
    exit(EXIT_FAILURE);
  }

  // Request a very simple scsi-generic ioctl(). If this fails, this
  // is not a scsi-generic device.
  if (ioctl(sg_fd, SG_GET_VERSION_NUM, &version) < 0) 
  {
    printf("%s is not a SCSI-generic device.\n", scsi_device);
    exit(EXIT_FAILURE);
  }

  return sg_fd;
}

int usage(char const* msg, char const* msgarg, int argc, char const* const* argv)
{
    printf(msg, msgarg);
    printf("\n"
           "\n"
           "Usage: %s <scsi_device> <command> [options...]\n"
           "\n"
	   "  scsi_device is the path to a device managed by the\n"
	   "  'scsi-generic' driver. Those are usually /dev/sgN,\n"
	   "  where N is the device number.\n"
           "\n"
           "Commands:\n"
           "  writefirmware [-p] [-r] <firmware.mse>\n"
           "    -r: Reboot device\n"
           "    -p: Repartition device\n", argv[0]);
    exit(EXIT_SUCCESS);
}

int main(int argc, char const* const* argv)
{
    int arg = 3;
    char const* mse_filename = NULL;
    char const* scsi_device = NULL;
    int repartition = 0;
    int reboot = 0;

    int f; // Firmware file descriptor
    unsigned long bytes;

    printf("iPodSCSI for linux v. " VERSION " r" VERSION_SVN " - Copyright 2012 by Nuno J. Silva (njsg)\n"
	   "Based on the original iPodSCSI Windows code by Michael Sparmann (TheSeven)\n"
           "This is free software; see the source for copying conditions.  There is NO\n"
           "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
           "\n");

    if (argc < 3) return usage("Not enough arguments specified", NULL, argc, argv);

    if (strcmp(argv[2], "writefirmware") != 0)
    {
      usage("Unknown command: %s", argv[2], argc, argv);
    }

    scsi_device = argv[1];
    

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

    int sg_fd = open_scsi (scsi_device);

    if (!mse_filename) return usage("No MSE file name specified", NULL, argc, argv);


    f = open(mse_filename, O_RDONLY);
  
    if (f == -1)
    {
      perror("Error while opening MSE file");
      exit(EXIT_FAILURE);
    }


    {
      // Get MSE file size 
      struct stat *f_stat = malloc (sizeof (struct stat));
      unsigned long size;

      if (fstat (f, f_stat) == -1) 
      {
	perror("Error while getting MSE file size");
	exit(EXIT_FAILURE);
      }

      bytes = f_stat->st_size;

      if (bytes & 0xfff) 
      {
	fprintf(stderr, "MSE file size must be a multiple of 4096\n");
	exit (EXIT_FAILURE);
      }
    }

    int sectors = bytes >> 12;

    // Most commands will have the same prefix.
    unsigned char cmdBlk[] = {APPLE_PREFIX, 0, 0, 0, 0, 0};


    // Prepare a SCSI command structure, which will be changed as
    // needed later.
    sg_io_hdr_t io_hdr;
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.dxfer_direction = SG_DXFER_NONE;
    io_hdr.dxfer_len = 0;
    io_hdr.dxferp = NULL;
    io_hdr.cmdp = cmdBlk;
    io_hdr.cmd_len = sizeof(cmdBlk);
    io_hdr.mx_sb_len = 0;
    io_hdr.sbp = NULL;

    if (repartition)
    {
      printf("Repartitioning...");
      int partsize = sectors << 2;
      cmdBlk[1] = CMD_PARTITION;
      cmdBlk[2] = (partsize >> 24) & 0xff;
      cmdBlk[3] = (partsize >> 16) & 0xff;
      cmdBlk[4] = (partsize >> 8) & 0xff;
      cmdBlk[5] = (partsize) & 0xff;

      io_hdr.timeout = 60000;     
      
      if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) 
      {
	perror("iPod repartitioning SG_IO ioctl error");
	exit(EXIT_FAILURE);
      }
      printf(" done\n");
    }
    

    printf("Initiating firmware transfer...");

    cmdBlk[1] = CMD_INIT_FIRMWARE;
    cmdBlk[2] = 0;
    cmdBlk[3] = 0;
    cmdBlk[4] = 0;
    cmdBlk[5] = 0;

    io_hdr.timeout = 1000;     
	


    if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) 
    {
      perror("iPod firmware transfer init SG_IO ioctl error");
      exit(EXIT_FAILURE);
    }
    printf(" done\n");



    printf("Writing firmware...");

    cmdBlk[1] = CMD_SEND_FIRMWARE;

    while (sectors)
    {
      int tsize = sectors > 0x10 ? 0x10 : sectors;
      int got = 0;
	
      char * buf = malloc((tsize << 12) * sizeof(char));

      while (got < (tsize << 12))
      {
	int b = read(f, buf + got, (tsize << 12) - got);
	if (b == -1) 
	{
	  perror("Error reading from MSE file");
	  exit(EXIT_FAILURE);
	}
	got += b;
      }
		
      cmdBlk[3] = tsize;
	
      io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
      io_hdr.dxfer_len = tsize << 12;
      io_hdr.dxferp = buf;
      io_hdr.timeout = 5000; 

      if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) 
      {
	perror("iPod firmware transfer SG_IO ioctl error");
	exit(EXIT_FAILURE);
      }

      sectors -= tsize;
      printf(".");
      fflush(stdout);
    }
    printf(" done\n");


    if (reboot)
    {
        printf("Rebooting device...");

	cmdBlk[0] = 0x1b;
	cmdBlk[1] = 0;
	cmdBlk[2] = 0;
	cmdBlk[3] = 0;
	cmdBlk[4] = 0x02;
	cmdBlk[5] = 0;

	io_hdr.dxfer_direction = SG_DXFER_NONE;
	io_hdr.dxfer_len = 0;
	io_hdr.dxferp = NULL;
	io_hdr.timeout = 10000;     

	if (ioctl(sg_fd, SG_IO, &io_hdr) < 0) 
	{
	  perror("iPod reboot SG_IO ioctl error");
	  exit(EXIT_FAILURE);
	}

        printf(" done\n");
    }
      
    close(sg_fd);
    exit(EXIT_SUCCESS);
}
