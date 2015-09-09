/*-
 * Copyright (c) 1998 - 2004 S�ren Schmidt <sos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/dev/ata/ata-isa.c,v 1.22.2.1 2004/10/10 15:01:47 sos Exp $");

#include "opt_ata.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ata.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/sema.h>
#include <sys/taskqueue.h>
#include <vm/uma.h>
#include <machine/stdarg.h>
#include <machine/resource.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <isa/isavar.h>
#include <dev/ata/ata-all.h>

/* local vars */
static struct isa_pnp_id ata_ids[] = {
    {0x0006d041,	"Generic ESDI/IDE/ATA controller"},	/* PNP0600 */
    {0x0106d041,	"Plus Hardcard II"},			/* PNP0601 */
    {0x0206d041,	"Plus Hardcard IIXL/EZ"},		/* PNP0602 */
    {0x0306d041,	"Generic ATA"},				/* PNP0603 */
								/* PNP0680 */
    {0x8006d041,	"Standard bus mastering IDE hard disk controller"},
    {0}
};

static int
ata_isa_locknoop(struct ata_channel *ch, int type)
{
    return ch->unit;
}

static void
ata_isa_setmode(struct ata_device *atadev, int mode)
{
    atadev->mode = ata_limit_mode(atadev, mode, ATA_PIO_MAX);
}

static int
ata_isa_probe(device_t dev)
{
    struct ata_channel *ch = device_get_softc(dev);
    struct resource *io = NULL, *altio = NULL;
    u_long tmp;
    int i, rid;

    /* check isapnp ids */
    if (ISA_PNP_PROBE(device_get_parent(dev), dev, ata_ids) == ENXIO)
	return ENXIO;
    
    /* allocate the io port range */
    rid = ATA_IOADDR_RID;
    io = bus_alloc_resource(dev, SYS_RES_IOPORT, &rid, 0, ~0,
			    ATA_IOSIZE, RF_ACTIVE);
    if (!io)
	return ENXIO;

    /* set the altport range */
    if (bus_get_resource(dev, SYS_RES_IOPORT, ATA_ALTADDR_RID, &tmp, &tmp)) {
	bus_set_resource(dev, SYS_RES_IOPORT, ATA_ALTADDR_RID,
			 rman_get_start(io) + ATA_ALTOFFSET, ATA_ALTIOSIZE);
    }

    /* allocate the altport range */
    rid = ATA_ALTADDR_RID; 
    altio = bus_alloc_resource(dev, SYS_RES_IOPORT, &rid, 0, ~0,
			       ATA_ALTIOSIZE, RF_ACTIVE);
    if (!altio) {
	bus_release_resource(dev, SYS_RES_IOPORT, ATA_IOADDR_RID, io);
	return ENXIO;
    }

    /* setup the resource vectors */
    for (i = ATA_DATA; i <= ATA_STATUS; i++) {
	ch->r_io[i].res = io;
	ch->r_io[i].offset = i;
    }
    ch->r_io[ATA_ALTSTAT].res = altio;
    ch->r_io[ATA_ALTSTAT].offset = 0;
    
    /* initialize softc for this channel */
    ch->unit = 0;
    ch->flags |= ATA_USE_16BIT;
    ch->locking = ata_isa_locknoop;
    ch->device[MASTER].setmode = ata_isa_setmode;
    ch->device[SLAVE].setmode = ata_isa_setmode;
    ata_generic_hw(ch);
    return ata_probe(dev);
}

static device_method_t ata_isa_methods[] = {
    /* device interface */
    DEVMETHOD(device_probe,	ata_isa_probe),
    DEVMETHOD(device_attach,	ata_attach),
    DEVMETHOD(device_resume,	ata_resume),
    { 0, 0 }
};

static driver_t ata_isa_driver = {
    "ata",
    ata_isa_methods,
    sizeof(struct ata_channel),
};

DRIVER_MODULE(ata, isa, ata_isa_driver, ata_devclass, 0, 0);
