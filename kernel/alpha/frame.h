/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
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
 * $Log:	frame.h,v $
 * Revision 2.2  93/01/14  17:13:08  danner
 * 	Added reference to documentation source(s).
 * 	[92/12/16  15:15:15  af]
 * 
 * 	Created.
 * 	[92/06/03            af]
 * 
 */

/*
 *	File: frame.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	6/92
 *
 *	Definition of the stack setup generated by a trap,
 *	and expected by the op_rei PAL call (non-privileged)
 *
 *	This code was derived exclusively from information available in
 *	"Alpha Architecture Reference Manual", Richard L. Sites ed.
 *	Digital Press, Burlington, MA 01803
 *	ISBN 1-55558-098-X, Order no. EY-L520E-DP
 */

#ifndef	_ALPHA_FRAME_H_
#define	_ALPHA_FRAME_H_	1

struct trap_frame {
	unsigned long		saved_r2;
	unsigned long		saved_r3;
	unsigned long		saved_r4;
	unsigned long		saved_r5;
	unsigned long		saved_r6;
	unsigned long		saved_r7;
	unsigned long		saved_pc;
	unsigned long		saved_ps;
};

#endif	/* _ALPHA_FRAME_H_ */
