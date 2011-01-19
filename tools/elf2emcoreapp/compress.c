/*
 * Helper functions to handle compression via UCL
 *
 * Copyright (C) 2011 Michael Sparmann
 *
 * Based on: Helper functions to handle compression via zlib
 *
 * Copyright (C) 2007-2008 Julian Brown
 * Copyright (C) 2008 Mike Frysinger
 *
 * Licensed under the GPL-2 or later.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <zlib.h>
#include "ucl/ucl.h"
#include "compress.h"
#include "stubs.h"

/* Open an (uncompressed) file as a stream.  Return 0 on success, 1 on
   error.
   NOTE: The MODE argument must remain valid for the lifetime of the stream,
   because it is referred to by reopen_stream_compressed() if it is called.
   String constants work fine.  */

int
fopen_stream_u(stream *fp, const char *path, const char *mode)
{
	fp->filep = fopen(path, mode);
	fp->type = (fp->filep) ? UNCOMPRESSED : INVALID;
	fp->mode = mode;
    fp->buffer = NULL;
    fp->bufsize = 0;
    fp->bufused = 0;
	return (fp->filep) ? 0 : 1;
}

/* Read from stream.  Return number of elements read.  */

size_t
fread_stream(void *ptr, size_t size, size_t nmemb, stream *str)
{
	size_t read;

	switch (str->type) {
		case UNCOMPRESSED:
		read = fread(ptr, size, nmemb, str->filep);
		break;

//		case COMPRESSED:
//		read = gzread(str->u.gzfilep, ptr, size * nmemb) / size;
//		break;

		default:
		abort();
	}

	return read;
}

/* Write to stream.  Return number of elements written.  */

size_t
fwrite_stream(const void *ptr, size_t size, size_t nmemb, stream *str)
{
	size_t written;

	switch (str->type) {
		case UNCOMPRESSED:
		written = fwrite(ptr, size, nmemb, str->filep);
		break;

		case COMPRESSED:
        if (str->bufused + size * nmemb > str->bufsize)
        {
            str->bufsize = (str->bufused + size * nmemb + 0xfff) & ~0xfff;
            str->buffer = (char*)realloc(str->buffer, str->bufsize);
        }
        memcpy(&str->buffer[str->bufused], ptr, size * nmemb);
        str->bufused += size * nmemb;
        written = nmemb;
		//written = gzwrite(str->u.gzfilep, ptr, size * nmemb) / size;
		break;

		default:
		abort();
	}

	return written;
}

/* Close stream.  */

int
fclose_stream(stream *str)
{
	switch (str->type) {
		case UNCOMPRESSED:
		return fclose(str->filep);

		case COMPRESSED:
        {
            int outsize = str->bufused + str->bufused / 8 + 256;
            str->buffer = (char*)realloc(str->buffer, str->bufused);
            char* buffer = (char*)malloc(outsize);
            int r = ucl_nrv2e_99_compress(str->buffer, str->bufused, buffer, &outsize, 0, 10, NULL, NULL);
            free(str->buffer);
            if (r != UCL_E_OK)
            {
                free(buffer);
                /* this should NEVER happen */
                printf("internal error - compression failed: %d\n", r);
                return 2;
            }
            buffer = (char*)realloc(buffer, outsize);
            int done = 0;
            while (done < outsize)
            {
                int bytes = fwrite(&buffer[done], 1, outsize - done, str->filep);
                if (bytes <= 0) return 3;
                done += bytes;
            }
            free(buffer);
		    return fclose(str->filep);
        }
//		return gzclose(str->u.gzfilep);

		default:
		abort();
	}

	return 0;
}

int
ferror_stream(stream *str)
{
	switch (str->type) {
		case UNCOMPRESSED:
		return ferror(str->filep);

		case COMPRESSED:
		return ferror(str->filep);
//		{
//			const char *err;
//			int errno;
//
//			err = gzerror(str->u.gzfilep, &errno);
//			if (errno == Z_OK || errno == Z_STREAM_END)
//				return 0;
//			else if (errno == Z_ERRNO)
//				return 1;
//			else {
//				fprintf(stderr, "%s\n", err);
//				return 1;
//			}
//		}
//		break;

		default:
		abort();
	}

	return 0;
}

int
fseek_stream(stream *str, long offset, int whence)
{
	switch (str->type) {
		case UNCOMPRESSED:
		return fseek(str->filep, offset, whence);

//		case COMPRESSED:
//		return gzseek(str->u.gzfilep, offset, whence);

		default:
		abort();
	}
}

/* Reopen a stream at the current file position.  */

void
reopen_stream_compressed(stream *str)
{
	int fd;
	long offset, roffset;

	/* Already a compressed stream, return immediately  */
	if (str->type == COMPRESSED)
		return;

	if (str->type == INVALID)
		abort();

	fd = fileno(str->filep);
	/* Get current (buffered) file position.  */
	offset = ftell(str->filep);

	/* Make sure there's nothing left in buffers.  */
	fflush(str->filep);

	/* Reposition underlying FD.  (Might be unnecessary?)  */
	roffset = lseek(fd, offset, SEEK_SET);

	assert(roffset == offset);

	/* Reopen as compressed stream.  */
	//str->u.gzfilep = gzdopen(fd, str->mode);
	//gzsetparams(str->u.gzfilep, 9, Z_DEFAULT_STRATEGY);
	str->type = COMPRESSED;
}

void
transfer(stream *ifp, stream *ofp, int count)
{
	char cmd[1024];
	int n, num;

	while (count == -1 || count > 0) {
		if (count == -1 || count > sizeof(cmd))
			num = sizeof(cmd);
		else
			num = count;
		n = fread_stream(cmd, 1, num, ifp);
		if (n == 0)
			break;
		if (fwrite_stream(cmd, n, 1, ofp) != 1)
			fatal_perror("Write failed :-(\n");
		if (count != -1)
			count -= n;
	}
	if (count > 0)
		fatal("Failed to transfer %d bytes\n", count);
}
