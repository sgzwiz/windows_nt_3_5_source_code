/***
*_open.c - open a stream, with string mode
*
*	Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _openfile() - opens a stream, with string arguments for mode
*
*Revision History:
*	09-02-83  RN	initial version
*	03-02-87  JCR	made _openfile recognize "wb+" as equal to "w+b", etc.
*			got rid of intermediate _openfile flags (internal) and
*			now go straight from mode string to open system call
*			and system->_flags.
*	09-28-87  JCR	Corrected _iob2 indexing (now uses _iob_index() macro).
*	02-21-88  SKS	Removed #ifdef IBMC20
*	06-06-88  JCR	Optimized _iob2 references
*	06-10-88  JCR	Use near pointer to reference _iob[] entries
*	08-19-88  GJF	Initial adaption for the 386.
*	11-14-88  GJF	Added shflag (file sharing flag) parameter, also some
*			cleanup (now specific to the 386).
*	08-17-89  GJF	Clean up, now specific to OS/2 2.0 (i.e., 386 flat
*			model). Also fixed copyright and indents.
*	02-15-90  GJF	_iob[], _iob2[] merge. Also, fixed copyright.
*	03-16-90  GJF	Made calling type _CALLTYPE1, added #include
*			<cruntime.h> and removed #include <register.h>.
*	03-27-90  GJF	Added const qualifier to types of filename and mode.
*			Added #include <io.h>.
*	07-11-90  SBM	Added support for 'c' and 'n' flags
*	07-23-90  SBM	Replaced <assertm.h> by <assert.h>
*	10-03-90  GJF	New-style function declarator.
*	01-18-91  GJF	ANSI naming.
*	03-11-92  GJF	Replaced __tmpnum field with _tmpfname field for
*			Win32.
*	03-25-92  DJM	POSIX support.
*	08-26-92  GJF	Fixed POSIX support.
*	05-24-93  PML	Added support for 'D', 'R', 'S' and 'T' flags
*
*******************************************************************************/

#include <cruntime.h>
#include <stdio.h>
#include <fcntl.h>
#include <file2.h>
#include <io.h>
#include <assert.h>
#include <internal.h>

#define CMASK	0644	/* rw-r--r-- */
#define P_CMASK 0666	/* different for Posix */

/***
*FILE *_openfile(filename, mode, shflag, stream) - open a file with string
*	mode and file sharing flag.
*
*Purpose:
*	parse the string, looking for exactly one of {rwa}, at most one '+',
*	at most one of {tb}, at most one of {cn}, at most one of {SR}, at most
*	one 'T', and at most one 'D'. pass the result on as an int containing
*	flags of what was found. open a file with proper mode if permissions
*	allow. buffer not allocated until first i/o call is issued. intended
*	for use inside library only
*
*Entry:
*	char *filename - file to open
*	char *mode - mode to use (see above)
*	int shflag - file sharing flag
*	FILE *stream - stream to use for file
*
*Exit:
*	set stream's fields, and causes system file management by system calls
*	returns stream or NULL if fails
*
*Exceptions:
*
*******************************************************************************/

FILE * _CRTAPI1 _openfile (
	const char *filename,
	REG3 const char *mode,
#ifndef _POSIX_
	int shflag,
#endif
	FILE *str
	)
{
	REG2 int modeflag;
#ifdef _POSIX_
	int streamflag = 0;
#else
	int streamflag = _commode;
	int commodeset = 0;
	int scanset    = 0;
#endif
	int whileflag;
	int filedes;
	REG1 FILE *stream;

	assert(filename != NULL);
	assert(mode != NULL);
	assert(str != NULL);

	/* Parse the user's specification string as set flags in
	       (1) modeflag - system call flags word
	       (2) streamflag - stream handle flags word. */

	/* First mode character must be 'r', 'w', or 'a'. */

	switch (*mode) {
	case 'r':
#ifdef _POSIX_
		modeflag = O_RDONLY;
#else
		modeflag = _O_RDONLY;
#endif
		streamflag |= _IOREAD;
		break;
	case 'w':
#ifdef _POSIX_
		modeflag = O_WRONLY | O_CREAT | O_TRUNC;
#else
		modeflag = _O_WRONLY | _O_CREAT | _O_TRUNC;
#endif
		streamflag |= _IOWRT;
		break;
	case 'a':
#ifdef _POSIX_
		modeflag = O_WRONLY | O_CREAT | O_APPEND;
		streamflag |= _IOWRT | _IOAPPEND;
#else
		modeflag = _O_WRONLY | _O_CREAT | _O_APPEND;
		streamflag |= _IOWRT;
#endif
		break;
	default:
		return(NULL);
		break;
	}

	/* There can be up to three more optional mode characters:
	   (1) A single '+' character,
	   (2) One of 't' and 'b' and
	   (3) One of 'c' and 'n'.
	*/

	whileflag=1;

	while(*++mode && whileflag)
		switch(*mode) {

		case '+':
#ifdef	_POSIX_
			if (modeflag & O_RDWR)
				whileflag=0;
			else {
				modeflag |= O_RDWR;
				modeflag &= ~(O_RDONLY | O_WRONLY);
#else
			if (modeflag & _O_RDWR)
				whileflag=0;
			else {
				modeflag |= _O_RDWR;
				modeflag &= ~(_O_RDONLY | _O_WRONLY);
#endif
				streamflag |= _IORW;
				streamflag &= ~(_IOREAD | _IOWRT);
			}
			break;

		case 'b':
#ifndef _POSIX_
			if (modeflag & (_O_TEXT | _O_BINARY))
				whileflag=0;
			else
				modeflag |= _O_BINARY;
#endif
			break;

#ifndef _POSIX_
		case 't':
			if (modeflag & (_O_TEXT | _O_BINARY))
				whileflag=0;
			else
				modeflag |= _O_TEXT;
			break;

		case 'c':
			if (commodeset)
				whileflag=0;
			else {
				commodeset = 1;
				streamflag |= _IOCOMMIT;
			}
			break;

		case 'n':
			if (commodeset)
				whileflag=0;
			else {
				commodeset = 1;
				streamflag &= ~_IOCOMMIT;
			}
			break;

		case 'S':
			if (scanset)
				whileflag=0;
			else {
				scanset = 1;
				modeflag |= _O_SEQUENTIAL;
			}
			break;

		case 'R':
			if (scanset)
				whileflag=0;
			else {
				scanset = 1;
				modeflag |= _O_RANDOM;
			}
			break;

		case 'T':
			if (modeflag & _O_SHORT_LIVED)
				whileflag=0;
			else
				modeflag |= _O_SHORT_LIVED;
			break;

		case 'D':
			if (modeflag & _O_TEMPORARY)
				whileflag=0;
			else
				modeflag |= _O_TEMPORARY;
			break;
#endif

		default:
			whileflag=0;
			break;
		}

	/* Try to open the file.  Note that if neither 't' nor 'b' is
	   specified, _sopen will use the default. */

#ifdef _POSIX_
	if ((filedes = open(filename, modeflag, P_CMASK)) < 0)
#else
	if ((filedes = _sopen(filename, modeflag, shflag, CMASK)) < 0)
#endif
		return(NULL);

	/* Set up the stream data base. */

	_cflush++;  /* force library pre-termination procedure */

	/* Init pointers */
	stream = str;

	stream->_flag = streamflag;
#ifdef	_CRUISER_
	stream->_cnt = stream->__tmpnum = 0;
	stream->_base = stream->_ptr = NULL;
#else	/* ndef _CRUISER_ */
	stream->_cnt = 0;
	stream->_tmpfname = stream->_base = stream->_ptr = NULL;
#endif	/* _CRUISER_ */

	stream->_file = filedes;

	return(stream);
}
