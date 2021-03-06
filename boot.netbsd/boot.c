/*
 * Ported to boot 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 *
 * Mach Operating System
 * Copyright (c) 1992, 1991 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * HISTORY
 * boot.c,v
 * Revision 1.8  1993/07/11  12:02:21  andrew
 * Fixes from bde, including support for loading @ any MB boundary (e.g. a
 * kernel linked for 0xfe100000 will load at the 1MB mark) and read-ahead
 * buffering to speed booting from floppies.  Also works with aha174x
 * controllers in enhanced mode.
 *
 * Revision 1.7  1993/06/18  06:50:52  cgd
 * convert magic numbers to network byte order, and attendent changes
 *
 * Revision 1.6  1993/06/14  00:47:08  deraadt
 * *whoops*. The previous commit killed a few important characters of code.
 *
 * Revision 1.5  1993/06/05  22:52:11  cgd
 * make sure kernel is small enough; this is a really weird fix from
 * rod, pk patch #159.  the comment is:
 *
 * The +28672 is for memory allocated by locore.s that must fit in the bss!
 *
 * this seems way wrong to me, but i'm not going to fix it in locore right
 * now...
 *
 * Revision 1.4  1993/05/04  10:22:39  deraadt
 * if we timeout asking for kernel name, print a \n before proceeding.
 * Funny how one character can bug ya so much, eh?
 *
 * Revision 1.3  1993/04/28  06:37:58  cgd
 * bsd->netbsd
 *
 * Revision 1.2  1993/04/28  05:32:55  cgd
 * new kernel name is "bsd"  also, add "o*" to list of kernels to boot.
 *
 * Revision 1.1  1993/03/21  18:08:26  cgd
 * after 0.2.2 "stable" patches applied
 *
 * Revision 2.2  92/04/04  11:34:37  rpd
 *
 * 93/07/03  bde
 *	Write first 4096 bytes to load address, not always to address 0.
 *
 * 93/06/29  bde
 *	Don't clobber BIOS variables.
 *
 * 	Change date in banner.
 * 	[92/04/03  16:51:14  rvb]
 * 
 * 	Fix Intel Copyright as per B. Davies authorization.
 * 	[92/04/03            rvb]
 * 	From 2.5 version.
 * 	[92/03/30            mg32]
 * 
 */

/*
  Copyright 1988, 1989, 1990, 1991, 1992 
   by Intel Corporation, Santa Clara, California.

                All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <sys/param.h>
#include "boot.h"
#include <a.out.h>
#include <sys/reboot.h>

int MacH = 0;
struct exec head;
int argv[10], esym;
char *name;
char *names[] = {
	"/netbsd", "/onetbsd", "/netbsd.old",
	"/386bsd", "/o386bsd", "/386bsd.old",
	"/vmunix", "/ovmunix", "/vmunix.old"
};
#define NUMNAMES	(sizeof(names)/sizeof(char *))

extern int end;
boot(drive)
int drive;
{
	int loadflags, currname = 0;
	register char *cp;
	register int c;

		
	printf("\n>> NetBSD BOOT @ 0x%x: %d/%d k of memory  [%s]\n",
		ouraddr,
		argv[7] = memsize(0),
		argv[8] = memsize(1),
		"3.8");
/*
	printf("use options hd(1,...... to boot sd0 when wd0 is also installed\n");
*/
	gateA20();
loadstart:
	/***************************************************************\
	* As a default set it to the first partition of the first	*
	* floppy or hard drive						*
	\***************************************************************/
	part = unit = 0;
	maj = (drive&0x80 ? 0 : 2);		/* a good first bet */
	name = names[currname++];

	loadflags = 0;
	if (currname == NUMNAMES)
		currname = 0;
	getbootdev(&loadflags);
	if (openrd()) {
		printf("Can't find %s\n", name);
		goto loadstart;
	}
/*
	if (inode.i_mode&IEXEC)
		loadflags |= RB_KDB;
*/
	/*
	 * openrd() has made name point to the name component
	 * after the closing RPAR in the device name.
	 */
	cp = name;
	while ((c = *cp) && c == '/') cp++;
	if (cp[0] == 'm' && cp[1] == 'a' && cp[2] == 'c' &&
	    cp[3] == 'h')
	{
		MacH = 1;
	}
	loadprog(loadflags);
	goto loadstart;
}

loadprog(howto)
	int		howto;
{
	long int startaddr;
	long int addr;	/* physical address.. not directly useable */
	long int addr0;
	int i;
	static int (*x_entry)() = 0;
	unsigned char	tmpbuf[4096]; /* we need to load the first 4k here */

	argv[3] = 0;
	argv[4] = 0;
	read(&head, sizeof(head));
/*	if ( N_BADMAG(head))
#define	N_BADMAG(ex) \
	(N_GETMAGIC(ex) != NMAGIC && N_GETMAGIC(ex) != OMAGIC && \
	    N_GETMAGIC(ex) != ZMAGIC && N_GETMAGIC(ex) != QMAGIC)
*/
	i = N_GETMAGIC(head);
	if (i != NMAGIC && i != OMAGIC && i != ZMAGIC && i != QMAGIC)
	{
		printf("Invalid format!\n");
		return;
	}

	poff = N_TXTOFF(head);
	/*if(poff==0)
		poff = 32;*/

	startaddr = (int)head.a_entry;
	addr = (startaddr & 0x00f00000); /* some MEG boundary */
	addr0 = addr;
	printf("Booting %s(%d,%c)%s @ 0x%x\n"
			, devs[maj]
			, unit
			, 'a'+part
			, name
			, addr);
	if(addr < ouraddr)
	{
		if((addr + head.a_text + head.a_data) > ouraddr)
		{
			printf("kernel will not fit below loader\n");
			return;
		}
		/*
		 * The +28672 is for memory allocated by locore.s that must
		 * fit in the bss! (XXX - cgd)
		 */
		if((addr + head.a_text + head.a_data + head.a_bss + 28672) > 0xa0000)
		{
			printf("kernel too big, won't fit in 640K with bss\n");
			printf("Only hope is to link the kernel for > 1MB\n");
			return;
		}
		if((addr + head.a_text + head.a_data + head.a_bss) > ouraddr)
		{
			printf("loader overlaps bss, kernel must bzero\n");
		}
	}
	printf("text=0x%x ", head.a_text);
	/********************************************************/
	/* LOAD THE TEXT SEGMENT				*/
	/* don't clobber the first 4k yet (BIOS NEEDS IT) 	*/
	/********************************************************/
	read(tmpbuf,4096);
	addr += 4096; 
	xread(addr, head.a_text - 4096);
	addr += head.a_text - 4096;

	printf("data=0x%x ", head.a_data);
	if (head.a_data) {
		/********************************************************/
		/* Load the Initialised data after the text		*/
		/********************************************************/
		while (addr & CLOFSET)
	                *(char *)addr++ = 0;

		xread(addr, head.a_data);
		addr += head.a_data;
	}

	/********************************************************/
	/* Skip over the uninitialised data			*/
	/* (but clear it)					*/
	/********************************************************/
	printf("bss=0x%x ", head.a_bss);
	if( (addr < ouraddr) && ((addr + head.a_bss) > ouraddr))
	{
		pbzero(addr,ouraddr - (int)addr);
	}
	else
	{
		pbzero(addr,head.a_bss);
	}
	argv[3] = (addr += head.a_bss);

#ifdef LOADSYMS /* not yet, haven't worked this out yet */
	if (addr > 0x100000)
	{
		/********************************************************/
		/*copy in the symbol header				*/
		/********************************************************/
		pcpy(&head.a_syms, addr, sizeof(head.a_syms));
		addr += sizeof(head.a_syms);
	
		/********************************************************/
		/* READ in the symbol table				*/
		/********************************************************/
		printf("symbols=[+0x%x", head.a_syms);
		xread(addr, head.a_syms);
		addr += head.a_syms;
	
		/********************************************************/
		/* Followed by the next integer (another header)	*/
		/* more debug symbols?					*/
		/********************************************************/
		read(&i, sizeof(int));
		pcpy(&i, addr, sizeof(int));
		i -= sizeof(int);
		addr += sizeof(int);
	
	
		/********************************************************/
		/* and that many bytes of (debug symbols?)		*/
		/********************************************************/
		printf("+0x%x] ", i);
		xread(addr, i);
		addr += i;
	}
#endif	LOADSYMS
	/********************************************************/
	/* and note the end address of all this			*/
	/********************************************************/

	argv[4] = ((addr+sizeof(int)-1))&~(sizeof(int)-1);
	printf("total=0x%x ",argv[4]);


	/*
	 *  We now pass the various bootstrap parameters to the loaded
	 *  image via the argument list
	 *  (THIS IS A BIT OF HISTORY FROM MACH.. LEAVE FOR NOW)
	 *  arg1 = boot flags
	 *  arg2 = boot device
	 *  arg3 = start of symbol table (0 if not loaded)
	 *  arg4 = end of symbol table (0 if not loaded)
	 *  arg5 = transfer address from image
	 *  arg6 = transfer address for next image pointer
	 */
	switch(maj)
	{
	case 2:
		if (! find("/mach_servers/startup")) {
			printf("\n\nInsert file system \n");
			getchar();
		}
		break;
	case 1:
		maj = 4;
		unit = 0;
		break;
	}
	if (MacH && maj)
		maj--;
	argv[1] = howto;
	argv[2] = (MAKEBOOTDEV(maj, 0, 0, unit, part)) ;
	argv[5] = (head.a_entry &= 0xfffffff);
	argv[6] = (int) &x_entry;
	argv[0] = 8;
	/****************************************************************/
	/* copy that first page and overwrite any BIOS variables	*/
	/****************************************************************/
	printf("entry point=0x%x\n" ,((int)startaddr) & 0xfffffff);
	/* Under no circumstances overwrite precious BIOS variables! */
	if (!addr0) {
		pcpy(tmpbuf, addr0, 0x400);
		pcpy(tmpbuf + 0x500, addr0 + 0x500, 4096 - 0x500);
	} else {
		pcpy(tmpbuf        , addr0        , 4096        );
	}
	if (MacH)
		startmach(head.a_entry, argv);
	else
		startprog(((int)startaddr & 0xffffff), argv);
}

char namebuf[100];
getbootdev(howto)
     int *howto;
{
	register char c;
	register char *ptr = namebuf;

	printf("Boot: [[[%s(%d,%c)]%s][-s][-a][-d]] :- "
			, devs[maj]
			, unit
			, 'a'+part
			, name);
	if (gets(namebuf)) {
		while (c=*ptr) {
			while (c==' ')
				c = *++ptr;
			if (!c)
				return;
			if (c=='-')
				while ((c = *++ptr) && c!=' ')
					switch (c) {
					      case 'a':
						*howto |= RB_ASKNAME; continue;
					      case 's':
						*howto |= RB_SINGLE; continue;
					      case 'd':
						*howto |= RB_KDB; continue;
					      case 'b':
						*howto |= RB_HALT; continue;
					}

			else {
				name = ptr;
				while ((c = *++ptr) && c!=' ');
				if (c)
					*ptr++ = 0;
			}
		}
	} else
		printf("\n");
}
