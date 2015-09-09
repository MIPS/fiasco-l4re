/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * KVM/MIPS: Assembler support for hardware virtualization extensions
 * Author: Yann Le Du <ledu@kymasys.com>
 *
 * Copyright (C) 2012  MIPS Technologies, Inc.  All rights reserved.
 */

#include <asm.h>
#include <regdef.h>
#include <mipsregs.h>
#include <mipsvzregs.h>

/*
 * This routine sets GuestCtl1.RID to GUESTCTL1_VZ_ROOT_GUESTID
 * Inputs: none
 */
LEAF(mips32_ClearGuestRID)
	.set	push
	.set	mips32r2
	.set	noreorder
	mfc0	t0, CP0_GUESTCTL1
	addiu	t1, zero, GUESTCTL1_VZ_ROOT_GUESTID
	ins	t0, t1, GUESTCTL1_RID_SHIFT, GUESTCTL1_RID_WIDTH
	mtc0	t0, CP0_GUESTCTL1 # Set GuestCtl1.RID = GUESTCTL1_VZ_ROOT_GUESTID
	jr.hb	ra
	nop					# BD Slot
	.set    pop
END(mips32_ClearGuestRID)


/*
 * This routine sets GuestCtl1.RID to a new value
 * Inputs: a0 = new GuestRID value (right aligned)
 */
LEAF(mips32_SetGuestRID)
	.set	push
	.set	mips32r2
	.set	noreorder
	mfc0	t0, CP0_GUESTCTL1
	ins 	t0, a0, GUESTCTL1_RID_SHIFT, GUESTCTL1_RID_WIDTH
	mtc0	t0, CP0_GUESTCTL1		# Set GuestCtl1.RID
	jr.hb	ra
	nop					# BD Slot
	.set	pop
END(mips32_SetGuestRID)


/*
* This routine sets GuestCtl1.RID to GuestCtl1.ID
* Inputs: none
*/
LEAF(mips32_SetGuestRIDtoGuestID)
	.set	push
	.set	mips32r2
	.set	noreorder
	mfc0	t0, CP0_GUESTCTL1		# Get current GuestID
	ext 	t1, t0, GUESTCTL1_ID_SHIFT, GUESTCTL1_ID_WIDTH
	ins 	t0, t1, GUESTCTL1_RID_SHIFT, GUESTCTL1_RID_WIDTH
	mtc0	t0, CP0_GUESTCTL1		# Set GuestCtl1.RID = GuestCtl1.ID
	jr.hb	ra
	nop					# BD Slot
	.set	pop
END(mips32_SetGuestRIDtoGuestID)


/*
 * This routine sets GuestCtl1.ID to a new value
 * Inputs: a0 = new GuestID value (right aligned)
 */
LEAF(mips32_SetGuestID)
	.set	push
	.set	mips32r2
	.set	noreorder
	mfc0	t0, CP0_GUESTCTL1
	ins 	t0, a0, GUESTCTL1_ID_SHIFT, GUESTCTL1_ID_WIDTH
	mtc0	t0, CP0_GUESTCTL1		# Set GuestCtl1.ID
	jr.hb	ra
	nop					# BD Slot
	.set	pop
END(mips32_SetGuestID)
