/*
 * Internal interface to L4 cirrusfb driver
 *
 * This file is distributed under the terms of the GNU General Public License 2.
 * Please see the COPYING-GPL-2 file for details.
 */

#ifndef __L4_CIRRUSFB_H
#define __L4_CIRRUSFB_H

struct fb_info;

/*
 * Some of the graphics libraries in mag and l4con/libgfxbitmap make extensive
 * use of unaligned memory accesses which cause ARCH_mips to take an address
 * alignment exception if L4_CIRRUSFB_DEFAULT_BPP 24 is used; 16 is safe.
 */
#define L4_CIRRUSFB_DEFAULT_BPP 16

struct fb_info* l4_cirrusfb_get_fbinfo(void);
int l4_cirrusfb_config(char *options);
int l4_cirrusfb_init(void);

#endif /* __L4_CIRRUSFB_H */
