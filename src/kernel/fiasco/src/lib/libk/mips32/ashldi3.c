/*
 * Included from linux arch/mips/lib/ashldi3.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

//#include <linux/module.h>

#include "libgcc.h"

long long __ashldi3(long long u, word_type b)
{
	DWunion uu, w;
	word_type bm;

	if (b == 0)
		return u;

	uu.ll = u;
	bm = 32 - b;

	if (bm <= 0) {
		w.s.low = 0;
		w.s.high = (unsigned int) uu.s.low << -bm;
	} else {
		const unsigned int carries = (unsigned int) uu.s.low >> bm;

		w.s.low = (unsigned int) uu.s.low << b;
		w.s.high = ((unsigned int) uu.s.high << b) | carries;
	}

	return w.ll;
}

EXPORT_SYMBOL(__ashldi3);
