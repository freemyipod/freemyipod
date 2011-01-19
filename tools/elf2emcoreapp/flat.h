/*
 * Copyright (C) 2002-2005  David McCullough <davidm@snapgear.com>
 * Copyright (C) 1998       Kenneth Albanowski <kjahds@kjahds.com>
 *                          The Silver Hammer Group, Ltd.
 *
 * This file provides the definitions and structures needed to
 * support uClinux flat-format executables.
 *
 * This is Free Software, under the GNU Public Licence v2 or greater.
 *
 */

#ifndef _LINUX_FLAT_H
#define _LINUX_FLAT_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <asm/flat.h>
#endif
#include <stdint.h>


#ifdef CONFIG_BINFMT_SHARED_FLAT
#define	MAX_SHARED_LIBS			(4)
#else
#define	MAX_SHARED_LIBS			(1)
#endif

/*
 * To make everything easier to port and manage cross platform
 * development,  all fields are in network byte order.
 */

#define	EMCOREAPP_HEADER_VERSION 1
struct emcoreapp_header
{
	char signature[8];
	uint32_t version version;
    off_t textstart;
    size_t textsize;
    size_t bsssize;
    size_t stacksize;
	off_t entrypoint;
	off_t relocstart;
	uint32_t reloccount;
	uint32_t flags;
    uint32_t creationtime;
};

#define HEADER_FLAG_UCL 0x00000001 /* all but the header is compressed */

#ifdef __KERNEL__ /* so systems without linux headers can compile the apps */
/*
 * While it would be nice to keep this header clean,  users of older
 * tools still need this support in the kernel.  So this section is
 * purely for compatibility with old tool chains.
 *
 * DO NOT make changes or enhancements to the old format please,  just work
 *        with the format above,  except to fix bugs with old format support.
 */

#include <asm/byteorder.h>

#define	OLD_FLAT_VERSION		0x00000002L
#define OLD_FLAT_RELOC_TYPE_TEXT	0
#define OLD_FLAT_RELOC_TYPE_DATA	1
#define OLD_FLAT_RELOC_TYPE_BSS		2

typedef union {
    uint32_t        value;
    struct {
# if defined(mc68000) && !defined(CONFIG_COLDFIRE)
        int32_t     offset : 30;
        uint32_t    type   : 2;
#   	define OLD_FLAT_FLAG_RAM    0x1 /* load program entirely into RAM */
# elif defined(__BIG_ENDIAN_BITFIELD)
        uint32_t    type   : 2;
        int32_t     offset : 30;
#   	define OLD_FLAT_FLAG_RAM    0x1 /* load program entirely into RAM */
# elif defined(__LITTLE_ENDIAN_BITFIELD)
        int32_t     offset : 30;
        uint32_t    type   : 2;
#   	define OLD_FLAT_FLAG_RAM    0x1 /* load program entirely into RAM */
# else
#   	error "Unknown bitfield order for flat files."
# endif
    } reloc;
} flat_v2_reloc_t;

#endif /* __KERNEL__ */

#endif /* _LINUX_FLAT_H */

/* this __MUST__ be at the VERY end of the file - do NOT move!!
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * end:
 * vi: tabstop=8 shiftwidth=4 textwidth=79 noexpandtab
 */
