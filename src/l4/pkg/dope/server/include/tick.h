/*
 * \brief   Interface of tick module
 * \date    2004-04-07
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _DOPE_TICK_H_
#define _DOPE_TICK_H_

struct tick_services {
	int   (*add)    (s32 msec, int (*callback)(void *), void *arg);
	void  (*handle) (void);
};

#endif /* _DOPE_TICK_H_ */

