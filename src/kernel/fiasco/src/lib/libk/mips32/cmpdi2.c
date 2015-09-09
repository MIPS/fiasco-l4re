/*
 * Included from linux arch/mips/lib/cmpdi2.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

//#include <linux/module.h>

#include "libgcc.h"

word_type __cmpdi2(long long a, long long b)
{
	const DWunion au = {
		.ll = a
	};
	const DWunion bu = {
		.ll = b
	};

	if (au.s.high < bu.s.high)
		return 0;
	else if (au.s.high > bu.s.high)
		return 2;

	if ((unsigned int) au.s.low < (unsigned int) bu.s.low)
		return 0;
	else if ((unsigned int) au.s.low > (unsigned int) bu.s.low)
		return 2;

	return 1;
}

EXPORT_SYMBOL(__cmpdi2);
