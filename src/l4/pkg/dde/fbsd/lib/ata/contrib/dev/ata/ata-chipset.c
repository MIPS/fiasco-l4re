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
__FBSDID("$FreeBSD: src/sys/dev/ata/ata-chipset.c,v 1.81.2.6 2005/03/23 05:14:11 mdodd Exp $");

#include "opt_ata.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/ata.h>
#include <sys/bus.h>
#include <sys/endian.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sema.h>
#include <sys/taskqueue.h>
#include <vm/uma.h>
#include <machine/stdarg.h>
#include <machine/resource.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/ata/ata-all.h>
#include <dev/ata/ata-pci.h>

/* misc defines */
#define GRANDPARENT(dev)	device_get_parent(device_get_parent(dev))
#define ATAPI_DEVICE(atadev) \
				((atadev->unit == ATA_MASTER && \
				atadev->channel->devices & ATA_ATAPI_MASTER) ||\
				(atadev->unit == ATA_SLAVE && \
				atadev->channel->devices & ATA_ATAPI_SLAVE))

/* local prototypes */
static int ata_generic_chipinit(device_t);
static void ata_generic_intr(void *);
static void ata_generic_setmode(struct ata_device *, int);
static int ata_acard_chipinit(device_t);
static void ata_acard_intr(void *);
static void ata_acard_850_setmode(struct ata_device *, int);
static void ata_acard_86X_setmode(struct ata_device *, int);
static int ata_ali_chipinit(device_t);
static void ata_ali_setmode(struct ata_device *, int);
static int ata_amd_chipinit(device_t);
static int ata_cyrix_chipinit(device_t);
static void ata_cyrix_setmode(struct ata_device *, int);
static int ata_cypress_chipinit(device_t);
static void ata_cypress_setmode(struct ata_device *, int);
static int ata_highpoint_chipinit(device_t);
static void ata_highpoint_intr(void *);
static void ata_highpoint_setmode(struct ata_device *, int);
static int ata_highpoint_check_80pin(struct ata_device *, int);
static int ata_intel_chipinit(device_t);
static void ata_intel_intr(void *);
static void ata_intel_reset(struct ata_channel *);
static void ata_intel_old_setmode(struct ata_device *, int);
static void ata_intel_new_setmode(struct ata_device *, int);
static int ata_ite_chipinit(device_t);
static void ata_ite_setmode(struct ata_device *, int);
static int ata_national_chipinit(device_t);
static void ata_national_setmode(struct ata_device *, int);
static int ata_nvidia_chipinit(device_t);
static int ata_via_chipinit(device_t);
static void ata_via_family_setmode(struct ata_device *, int);
static void ata_via_southbridge_fixup(device_t);
static int ata_promise_chipinit(device_t);
static int ata_promise_mio_allocate(device_t, struct ata_channel *);
static void ata_promise_mio_intr(void *);
static void ata_promise_sx4_intr(void *);
static void ata_promise_mio_dmainit(struct ata_channel *);
static void ata_promise_mio_reset(struct ata_channel *ch);
static int ata_promise_mio_command(struct ata_device *atadev, u_int8_t command, u_int64_t lba, u_int16_t count, u_int16_t feature);
static int ata_promise_sx4_command(struct ata_device *atadev, u_int8_t command, u_int64_t lba, u_int16_t count, u_int16_t feature);
static int ata_promise_apkt(u_int8_t *bytep, struct ata_device *atadev, u_int8_t command, u_int64_t lba, u_int16_t count, u_int16_t feature);
static void ata_promise_queue_hpkt(struct ata_pci_controller *ctlr, u_int32_t hpkt);
static void ata_promise_next_hpkt(struct ata_pci_controller *ctlr);
static void ata_promise_tx2_intr(void *);
static void ata_promise_old_intr(void *);
static void ata_promise_new_dmainit(struct ata_channel *);
static void ata_promise_setmode(struct ata_device *, int);
static int ata_serverworks_chipinit(device_t);
static void ata_serverworks_setmode(struct ata_device *, int);
static int ata_sii_chipinit(device_t);
static int ata_sii_allocate(device_t, struct ata_channel *);
static void ata_sii_reset(struct ata_channel *);
static void ata_sii_intr(void *);
static void ata_cmd_intr(void *);
static void ata_cmd_old_intr(void *);
static void ata_sii_setmode(struct ata_device *, int);
static void ata_cmd_setmode(struct ata_device *, int);
static int ata_sis_chipinit(device_t);
static void ata_sis_setmode(struct ata_device *, int);
static int ata_check_80pin(struct ata_device *, int);
static struct ata_chip_id *ata_find_chip(device_t, struct ata_chip_id *, int);
static struct ata_chip_id *ata_match_chip(device_t, struct ata_chip_id *);
static int ata_setup_interrupt(device_t);
static int ata_serialize(struct ata_channel *, int);
static int ata_mode2idx(int);

/* generic or unknown ATA chipset init code */
int
ata_generic_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    device_set_desc(dev, "GENERIC ATA controller");
    ctlr->chipinit = ata_generic_chipinit;
    return 0;
}

static int
ata_generic_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;
    ctlr->setmode = ata_generic_setmode;
    return 0;
}

static void
ata_generic_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
	    int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

	    if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		ATA_BMSTAT_INTERRUPT)
		continue;
	    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
	    DELAY(1);
	}
	ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_generic_setmode(struct ata_device *atadev, int mode)
{
    mode = ata_limit_mode(atadev, mode, ATA_UDMA2);
    mode = ata_check_80pin(atadev, mode);
    if (!ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode))
	atadev->mode = mode;
}

static void
ata_sata_setmode(struct ata_device *atadev, int mode)
{
    /*
     * if we detect that the device isn't a real SATA device we limit 
     * the transfer mode to UDMA5/ATA100.
     * this works around the problems some devices has with the 
     * Marvell 88SX8030 SATA->PATA converters and UDMA6/ATA133.
     */
    if (atadev->param->satacapabilities != 0x0000 &&
	atadev->param->satacapabilities != 0xffff) {
        if (!ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0,
			    ata_limit_mode(atadev, mode, ATA_UDMA6)))
	    atadev->mode = ATA_SA150;
    }
    else {
	mode = ata_limit_mode(atadev, mode, ATA_UDMA5);
	if (!ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode))
	    atadev->mode = mode;
    }
}

/*
 * Acard chipset support functions
 */
int
ata_acard_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_ATP850R, 0, ATPOLD, 0x00, ATA_UDMA2, "Acard ATP850" },
     { ATA_ATP860A, 0, 0,      0x00, ATA_UDMA4, "Acard ATP860A" },
     { ATA_ATP860R, 0, 0,      0x00, ATA_UDMA4, "Acard ATP860R" },
     { ATA_ATP865A, 0, 0,      0x00, ATA_UDMA6, "Acard ATP865A" },
     { ATA_ATP865R, 0, 0,      0x00, ATA_UDMA6, "Acard ATP865R" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64]; 

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_acard_chipinit;
    return 0;
}

static int
ata_acard_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
					       RF_SHAREABLE | RF_ACTIVE))) {
	device_printf(dev, "unable to map interrupt\n");
	return ENXIO;
    }
    if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			ata_acard_intr, ctlr, &ctlr->handle))) {
	device_printf(dev, "unable to setup interrupt\n");
	return ENXIO;
    }
    if (ctlr->chip->cfg1 == ATPOLD) {
	ctlr->setmode = ata_acard_850_setmode;
	ctlr->locking = ata_serialize;
    }
    else
	ctlr->setmode = ata_acard_86X_setmode;
    return 0;
}

static void
ata_acard_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ctlr->chip->cfg1 == ATPOLD && ch->locking(ch, ATA_LF_WHICH) != unit)
	    continue;
	if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
	    int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

	    if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		ATA_BMSTAT_INTERRUPT)
		continue;
	    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
	    DELAY(1);
	    ATA_IDX_OUTB(ch, ATA_BMCMD_PORT,
			 ATA_IDX_INB(ch, ATA_BMCMD_PORT)&~ATA_BMCMD_START_STOP);
	    DELAY(1);
	}
	ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_acard_850_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;

    mode = ata_limit_mode(atadev, mode,
			  ATAPI_DEVICE(atadev)?ATA_PIO_MAX:ctlr->chip->max_dma);

/* XXX missing WDMA0+1 + PIO modes */
    if (mode >= ATA_WDMA2) {
	error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0,mode);
	if (bootverbose)
	    ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		       (error) ? "FAILURE " : "",
		       ata_mode2str(mode), ctlr->chip->text);
	if (!error) {
	    u_int8_t reg54 = pci_read_config(parent, 0x54, 1);
	    
	    reg54 &= ~(0x03 << (devno << 1));
	    if (mode >= ATA_UDMA0)
		reg54 |= (((mode & ATA_MODE_MASK) + 1) << (devno << 1));
	    pci_write_config(parent, 0x54, reg54, 1);
	    pci_write_config(parent, 0x4a, 0xa6, 1);
	    pci_write_config(parent, 0x40 + (devno << 1), 0x0301, 2);
	    atadev->mode = mode;
	    return;
	}
    }
    /* we could set PIO mode timings, but we assume the BIOS did that */
}

static void
ata_acard_86X_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;


    mode = ata_limit_mode(atadev, mode,
			  ATAPI_DEVICE(atadev)?ATA_PIO_MAX:ctlr->chip->max_dma);

    mode = ata_check_80pin(atadev, mode);

/* XXX missing WDMA0+1 + PIO modes */
    if (mode >= ATA_WDMA2) {
	error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0,mode);
	if (bootverbose)
	    ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		       (error) ? "FAILURE " : "",
		       ata_mode2str(mode), ctlr->chip->text);
	if (!error) {
	    u_int16_t reg44 = pci_read_config(parent, 0x44, 2);
	    
	    reg44 &= ~(0x000f << (devno << 2));
	    if (mode >= ATA_UDMA0)
		reg44 |= (((mode & ATA_MODE_MASK) + 1) << (devno << 2));
	    pci_write_config(parent, 0x44, reg44, 2);
	    pci_write_config(parent, 0x4a, 0xa6, 1);
	    pci_write_config(parent, 0x40 + devno, 0x31, 1);
	    atadev->mode = mode;
	    return;
	}
    }
    /* we could set PIO mode timings, but we assume the BIOS did that */
}

/*
 * Acer Labs Inc (ALI) chipset support functions
 */
int
ata_ali_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_ALI_5229, 0xc4, 0, ALINEW, ATA_UDMA5, "AcerLabs Aladdin" },
     { ATA_ALI_5229, 0xc2, 0, ALINEW, ATA_UDMA4, "AcerLabs Aladdin" },
     { ATA_ALI_5229, 0x20, 0, ALIOLD, ATA_UDMA2, "AcerLabs Aladdin" },
     { ATA_ALI_5229, 0x00, 0, ALIOLD, ATA_WDMA2, "AcerLabs Aladdin" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64]; 

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_ali_chipinit;
    return 0;
}

static int
ata_ali_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    /* deactivate the ATAPI FIFO and enable ATAPI UDMA */
    pci_write_config(dev, 0x53, pci_read_config(dev, 0x53, 1) | 0x03, 1);
 
    /* enable cable detection and UDMA support on newer chips */
    if (ctlr->chip->cfg2 & ALINEW)
	pci_write_config(dev, 0x4b, pci_read_config(dev, 0x4b, 1) | 0x09, 1);
    ctlr->setmode = ata_ali_setmode;
    return 0;
}

static void
ata_ali_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if (ctlr->chip->cfg2 & ALINEW) {
	if (mode > ATA_UDMA2 &&
	    pci_read_config(parent, 0x4a, 1) & (1 << atadev->channel->unit)) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
    }
    else
	mode = ata_check_80pin(atadev, mode);

    if (ctlr->chip->cfg2 & ALIOLD) {
	/* doesn't support ATAPI DMA on write */
	atadev->channel->flags |= ATA_ATAPI_DMA_RO;
	if (atadev->channel->devices & ATA_ATAPI_MASTER &&
	    atadev->channel->devices & ATA_ATAPI_SLAVE) {
	    /* doesn't support ATAPI DMA on two ATAPI devices */
	    ata_prtdev(atadev, "two atapi devices on this channel, no DMA\n");
	    mode = ata_limit_mode(atadev, mode, ATA_PIO_MAX);
	}
    }

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "", 
		   ata_mode2str(mode), ctlr->chip->text);
    if (!error) {
	if (mode >= ATA_UDMA0) {
	    u_int8_t udma[] = {0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x0f};
	    u_int32_t word54 = pci_read_config(parent, 0x54, 4);

	    word54 &= ~(0x000f000f << (devno << 2));
	    word54 |= (((udma[mode&ATA_MODE_MASK]<<16)|0x05)<<(devno<<2));
	    pci_write_config(parent, 0x54, word54, 4);
	    pci_write_config(parent, 0x58 + (atadev->channel->unit << 2),
			     0x00310001, 4);
	}
	else {
	    u_int32_t piotimings[] =
		{ 0x006d0003, 0x00580002, 0x00440001, 0x00330001,
		  0x00310001, 0x00440001, 0x00330001, 0x00310001};

	    pci_write_config(parent, 0x54, pci_read_config(parent, 0x54, 4) &
					   ~(0x0008000f << (devno << 2)), 4);
	    pci_write_config(parent, 0x58 + (atadev->channel->unit << 2),
			     piotimings[ata_mode2idx(mode)], 4);
	}
	atadev->mode = mode;
    }
}

/*
 * American Micro Devices (AMD) support functions
 */
int
ata_amd_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_AMD756,  0x00, AMDNVIDIA, 0x00,	      ATA_UDMA4, "AMD 756" },
     { ATA_AMD766,  0x00, AMDNVIDIA, AMDCABLE|AMDBUG, ATA_UDMA5, "AMD 766" },
     { ATA_AMD768,  0x00, AMDNVIDIA, AMDCABLE,	      ATA_UDMA5, "AMD 768" },
     { ATA_AMD8111, 0x00, AMDNVIDIA, AMDCABLE,	      ATA_UDMA6, "AMD 8111" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64]; 

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_amd_chipinit;
    return 0;
}

static int
ata_amd_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    /* disable/set prefetch, postwrite */
    if (ctlr->chip->cfg2 & AMDBUG)
	pci_write_config(dev, 0x41, pci_read_config(dev, 0x41, 1) & 0x0f, 1);
    else
	pci_write_config(dev, 0x41, pci_read_config(dev, 0x41, 1) | 0xf0, 1);

    ctlr->setmode = ata_via_family_setmode;
    return 0;
}

/*
 * Cyrix chipset support functions
 */
int
ata_cyrix_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (pci_get_devid(dev) == ATA_CYRIX_5530) {
	device_set_desc(dev, "Cyrix 5530 ATA33 controller");
	ctlr->chipinit = ata_cyrix_chipinit;
	return 0;
    }
    return ENXIO;
}

static int
ata_cyrix_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    if (ctlr->r_res1)
	ctlr->setmode = ata_cyrix_setmode;
    else
	ctlr->setmode = ata_generic_setmode;
    return 0;
}

static void
ata_cyrix_setmode(struct ata_device *atadev, int mode)
{
    struct ata_channel *ch = atadev->channel;
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    u_int32_t piotiming[] = 
	{ 0x00009172, 0x00012171, 0x00020080, 0x00032010, 0x00040010 };
    u_int32_t dmatiming[] = { 0x00077771, 0x00012121, 0x00002020 };
    u_int32_t udmatiming[] = { 0x00921250, 0x00911140, 0x00911030 };
    int error;

    atadev->channel->dma->alignment = 16;
    atadev->channel->dma->max_iosize = 126 * DEV_BSIZE;

    mode = ata_limit_mode(atadev, mode, ATA_UDMA2);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on Cyrix chip\n",
		   (error) ? "FAILURE " : "", ata_mode2str(mode));
    if (!error) {
	if (mode >= ATA_UDMA0) {
	    ATA_OUTL(ch->r_io[ATA_BMCMD_PORT].res,
		     0x24 + (devno << 3), udmatiming[mode & ATA_MODE_MASK]);
	}
	else if (mode >= ATA_WDMA0) {
	    ATA_OUTL(ch->r_io[ATA_BMCMD_PORT].res,
		     0x24 + (devno << 3), dmatiming[mode & ATA_MODE_MASK]);
	}
	else {
	    ATA_OUTL(ch->r_io[ATA_BMCMD_PORT].res,
		     0x20 + (devno << 3), piotiming[mode & ATA_MODE_MASK]);
	}
	atadev->mode = mode;
    }
}

/*
 * Cypress chipset support functions
 */
int
ata_cypress_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    /*
     * the Cypress chip is a mess, it contains two ATA functions, but
     * both channels are visible on the first one.
     * simply ignore the second function for now, as the right
     * solution (ignoring the second channel on the first function)
     * doesn't work with the crappy ATA interrupt setup on the alpha.
     */
    if (pci_get_devid(dev) == ATA_CYPRESS_82C693 &&
	pci_get_function(dev) == 1 &&
	pci_get_subclass(dev) == PCIS_STORAGE_IDE) {
	device_set_desc(dev, "Cypress 82C693 ATA controller");
	ctlr->chipinit = ata_cypress_chipinit;
	return 0;
    }
    return ENXIO;
}

static int
ata_cypress_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    ctlr->setmode = ata_cypress_setmode;
    return 0;
}

static void
ata_cypress_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    int error;

    mode = ata_limit_mode(atadev, mode, ATA_WDMA2);

/* XXX missing WDMA0+1 + PIO modes */
    if (mode == ATA_WDMA2) { 
	error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0,mode);
	if (bootverbose)
	    ata_prtdev(atadev, "%ssetting WDMA2 on Cypress chip\n",
		       error ? "FAILURE " : "");
	if (!error) {
	    pci_write_config(parent, atadev->channel->unit?0x4e:0x4c,0x2020,2);
	    atadev->mode = mode;
	    return;
	}
    }
    /* we could set PIO mode timings, but we assume the BIOS did that */
}

/*
 * HighPoint chipset support functions
 */
int
ata_highpoint_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_HPT366, 0x05, HPT372, 0x00,	 ATA_UDMA6, "HighPoint HPT372" },
     { ATA_HPT366, 0x03, HPT370, 0x00,	 ATA_UDMA5, "HighPoint HPT370" },
     { ATA_HPT366, 0x02, HPT366, 0x00,	 ATA_UDMA4, "HighPoint HPT368" },
     { ATA_HPT366, 0x00, HPT366, HPTOLD, ATA_UDMA4, "HighPoint HPT366" },
     { ATA_HPT372, 0x01, HPT372, 0x00,	 ATA_UDMA6, "HighPoint HPT372" },
     { ATA_HPT302, 0x01, HPT372, 0x00,	 ATA_UDMA6, "HighPoint HPT302" },
     { ATA_HPT371, 0x01, HPT372, 0x00,	 ATA_UDMA6, "HighPoint HPT371" },
     { ATA_HPT374, 0x07, HPT374, 0x00,	 ATA_UDMA6, "HighPoint HPT374" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64];

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    strcpy(buffer, idx->text);
    if (idx->cfg1 == HPT374) {
	if (pci_get_function(dev) == 0)
	    strcat(buffer, " (channel 0+1)");
	else if (pci_get_function(dev) == 1)
	    strcat(buffer, " (channel 2+3)");
    }
    sprintf(buffer, "%s %s controller", buffer, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_highpoint_chipinit;
    return 0;
}

static int
ata_highpoint_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
					       RF_SHAREABLE | RF_ACTIVE))) {
	device_printf(dev, "unable to map interrupt\n");
	return ENXIO;
    }
    if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			ata_highpoint_intr, ctlr, &ctlr->handle))) {
	device_printf(dev, "unable to setup interrupt\n");
	return ENXIO;
    }

    if (ctlr->chip->cfg2 == HPTOLD) {
	/* disable interrupt prediction */
	pci_write_config(dev, 0x51, (pci_read_config(dev, 0x51, 1) & ~0x80), 1);
    }
    else {
	/* disable interrupt prediction */
	pci_write_config(dev, 0x51, (pci_read_config(dev, 0x51, 1) & ~0x03), 1);
	pci_write_config(dev, 0x55, (pci_read_config(dev, 0x55, 1) & ~0x03), 1);

	/* enable interrupts */
	pci_write_config(dev, 0x5a, (pci_read_config(dev, 0x5a, 1) & ~0x10), 1);

	/* set clocks etc */
	if (ctlr->chip->cfg1 < HPT372)
	    pci_write_config(dev, 0x5b, 0x22, 1);
	else
	    pci_write_config(dev, 0x5b,
			     (pci_read_config(dev, 0x5b, 1) & 0x01) | 0x20, 1);
    }
    ctlr->setmode = ata_highpoint_setmode;
    return 0;
}

static void
ata_highpoint_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ch->dma) {
	    int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

	    if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		ATA_BMSTAT_INTERRUPT)
		continue;
	    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
	    DELAY(1);
	}
	ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_highpoint_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;
    u_int32_t timings33[][4] = {
    /*	  HPT366      HPT370	  HPT372      HPT374		   mode */
	{ 0x40d0a7aa, 0x06914e57, 0x0d029d5e, 0x0ac1f48a },	/* PIO 0 */
	{ 0x40d0a7a3, 0x06914e43, 0x0d029d26, 0x0ac1f465 },	/* PIO 1 */
	{ 0x40d0a753, 0x06514e33, 0x0c829ca6, 0x0a81f454 },	/* PIO 2 */
	{ 0x40c8a742, 0x06514e22, 0x0c829c84, 0x0a81f443 },	/* PIO 3 */
	{ 0x40c8a731, 0x06514e21, 0x0c829c62, 0x0a81f442 },	/* PIO 4 */
	{ 0x20c8a797, 0x26514e97, 0x2c82922e, 0x228082ea },	/* MWDMA 0 */
	{ 0x20c8a732, 0x26514e33, 0x2c829266, 0x22808254 },	/* MWDMA 1 */
	{ 0x20c8a731, 0x26514e21, 0x2c829262, 0x22808242 },	/* MWDMA 2 */
	{ 0x10c8a731, 0x16514e31, 0x1c82dc62, 0x121882ea },	/* UDMA 0 */
	{ 0x10cba731, 0x164d4e31, 0x1c9adc62, 0x12148254 },	/* UDMA 1 */
	{ 0x10caa731, 0x16494e31, 0x1c91dc62, 0x120c8242 },	/* UDMA 2 */
	{ 0x10cfa731, 0x166d4e31, 0x1c8edc62, 0x128c8242 },	/* UDMA 3 */
	{ 0x10c9a731, 0x16454e31, 0x1c8ddc62, 0x12ac8242 },	/* UDMA 4 */
	{ 0,	      0x16454e31, 0x1c6ddc62, 0x12848242 },	/* UDMA 5 */
	{ 0,	      0,	  0x1c81dc62, 0x12448242 }	/* UDMA 6 */
    };

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if (ctlr->chip->cfg1 == HPT366 && ATAPI_DEVICE(atadev))
	mode = ata_limit_mode(atadev, mode, ATA_PIO_MAX);

    mode = ata_highpoint_check_80pin(atadev, mode);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on HighPoint chip\n",
		   (error) ? "FAILURE " : "", ata_mode2str(mode));
    if (!error)
	pci_write_config(parent, 0x40 + (devno << 2),
			 timings33[ata_mode2idx(mode)][ctlr->chip->cfg1], 4);
    atadev->mode = mode;
}

static int
ata_highpoint_check_80pin(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    u_int8_t reg, val, res;

    if (ctlr->chip->cfg1 == HPT374 && pci_get_function(parent) == 1) {
	reg = atadev->channel->unit ? 0x57 : 0x53;
	val = pci_read_config(parent, reg, 1);
	pci_write_config(parent, reg, val | 0x80, 1);
    }
    else {
	reg = 0x5b;
	val = pci_read_config(parent, reg, 1);
	pci_write_config(parent, reg, val & 0xfe, 1);
    }
    res = pci_read_config(parent, 0x5a, 1) & (atadev->channel->unit ? 0x1:0x2);
    pci_write_config(parent, reg, val, 1);

    if (mode > ATA_UDMA2 && res) {
	ata_prtdev(atadev,"DMA limited to UDMA33, non-ATA66 cable or device\n");
	mode = ATA_UDMA2;
    }
    return mode;
}

/*
 * Intel chipset support functions
 */
int
ata_intel_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_I82371FB,   0, 0, 0x00, ATA_WDMA2, "Intel PIIX" },
     { ATA_I82371SB,   0, 0, 0x00, ATA_WDMA2, "Intel PIIX3" },
     { ATA_I82371AB,   0, 0, 0x00, ATA_UDMA2, "Intel PIIX4" },
     { ATA_I82443MX,   0, 0, 0x00, ATA_UDMA2, "Intel PIIX4" },
     { ATA_I82451NX,   0, 0, 0x00, ATA_UDMA2, "Intel PIIX4" },
     { ATA_I82801AB,   0, 0, 0x00, ATA_UDMA2, "Intel ICH0" },
     { ATA_I82801AA,   0, 0, 0x00, ATA_UDMA4, "Intel ICH" },
     { ATA_I82372FB,   0, 0, 0x00, ATA_UDMA4, "Intel ICH" },
     { ATA_I82801BA,   0, 0, 0x00, ATA_UDMA5, "Intel ICH2" },
     { ATA_I82801BA_1, 0, 0, 0x00, ATA_UDMA5, "Intel ICH2" },
     { ATA_I82801CA,   0, 0, 0x00, ATA_UDMA5, "Intel ICH3" },
     { ATA_I82801CA_1, 0, 0, 0x00, ATA_UDMA5, "Intel ICH3" },
     { ATA_I82801DB,   0, 0, 0x00, ATA_UDMA5, "Intel ICH4" },
     { ATA_I82801DB_1, 0, 0, 0x00, ATA_UDMA5, "Intel ICH4" },
     { ATA_I82801EB,   0, 0, 0x00, ATA_UDMA5, "Intel ICH5" },
     { ATA_I82801EB_S1,0, 0, 0x00, ATA_SA150, "Intel ICH5" },
     { ATA_I82801EB_R1,0, 0, 0x00, ATA_SA150, "Intel ICH5" },
     { ATA_I6300ESB,   0, 0, 0x00, ATA_UDMA5, "Intel 6300ESB" },
     { ATA_I6300ESB_S1,0, 0, 0x00, ATA_SA150, "Intel 6300ESB" },
     { ATA_I6300ESB_R1,0, 0, 0x00, ATA_SA150, "Intel 6300ESB" },
     { ATA_I82801FB,   0, 0, 0x00, ATA_UDMA5, "Intel ICH6" },
     { ATA_I82801FB_S1,0, 0, 0x00, ATA_SA150, "Intel ICH6" },
     { ATA_I82801FB_R1,0, 0, 0x00, ATA_SA150, "Intel ICH6" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64]; 

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_intel_chipinit;
    return 0;
}

static int
ata_intel_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!ata_legacy(dev)) {
	if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
						   RF_SHAREABLE | RF_ACTIVE))) {
	    device_printf(dev, "unable to map interrupt\n");
	    return ENXIO;
	}
	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_intel_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}
    }

    if (ctlr->chip->chipid == ATA_I82371FB) {
	ctlr->setmode = ata_intel_old_setmode;
    }
    else if (ctlr->chip->max_dma < ATA_SA150) {
	ctlr->setmode = ata_intel_new_setmode;
    }
    else {
	pci_write_config(dev, PCIR_COMMAND,
			 pci_read_config(dev, PCIR_COMMAND, 2) & ~0x0400, 2);
	ctlr->reset = ata_intel_reset;
	ctlr->setmode = ata_sata_setmode;
    }
    return 0;
}

static void
ata_intel_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ch->dma) {
	    int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

	    if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		ATA_BMSTAT_INTERRUPT)
		continue;
	    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
	    DELAY(1);
	}
	ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_intel_old_setmode(struct ata_device *atadev, int mode)
{
    /* NOT YET */
}

static void
ata_intel_reset(struct ata_channel *ch)
{
    device_t parent = device_get_parent(ch->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int mask, timeout = 100;

    /* ICH6 has 4 SATA ports as master/slave on 2 channels so deal with pairs */
    if (ctlr->chip->chipid == ATA_I82801FB_S1 ||
	ctlr->chip->chipid == ATA_I82801FB_R1) {
	mask = (0x0005 << ch->unit);
    }
    else {
	/* ICH5 in compat mode has SATA ports as master/slave on 1 channel */
	if (pci_read_config(parent, 0x90, 1) & 0x04)
	    mask = 0x0003;
	else
	    mask = (0x0001 << ch->unit);
    }
    pci_write_config(parent, 0x92, pci_read_config(parent, 0x92, 2) & ~mask, 2);
    DELAY(10);
    pci_write_config(parent, 0x92, pci_read_config(parent, 0x92, 2) | mask, 2);

    while (timeout--) {
	ata_udelay(10000);
	if ((pci_read_config(parent, 0x92, 2) & (mask << 4)) == (mask << 4)) {
	    ata_udelay(10000);
	    return;
	}
    }
}

static void
ata_intel_new_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    u_int32_t reg40 = pci_read_config(parent, 0x40, 4);
    u_int8_t reg44 = pci_read_config(parent, 0x44, 1);
    u_int8_t reg48 = pci_read_config(parent, 0x48, 1);
    u_int16_t reg4a = pci_read_config(parent, 0x4a, 2);
    u_int16_t reg54 = pci_read_config(parent, 0x54, 2);
    u_int32_t mask40 = 0, new40 = 0;
    u_int8_t mask44 = 0, new44 = 0;
    int error;
    u_int8_t timings[] = { 0x00, 0x00, 0x10, 0x21, 0x23, 0x10, 0x21, 0x23,
			   0x23, 0x23, 0x23, 0x23, 0x23, 0x23 };

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if ( mode > ATA_UDMA2 && !(reg54 & (0x10 << devno))) {
	ata_prtdev(atadev,"DMA limited to UDMA33, non-ATA66 cable or device\n");
	mode = ATA_UDMA2;
    }

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (error)
	return;

    if (mode >= ATA_UDMA0) {
	pci_write_config(parent, 0x48, reg48 | (0x0001 << devno), 2);
	pci_write_config(parent, 0x4a, (reg4a & ~(0x3 << (devno<<2))) | 
				       (0x01 + !(mode & 0x01)), 2);
    }
    else {
	pci_write_config(parent, 0x48, reg48 & ~(0x0001 << devno), 2);
	pci_write_config(parent, 0x4a, (reg4a & ~(0x3 << (devno << 2))), 2);
    }
    reg54 |= 0x0400;
    if (mode >= ATA_UDMA2)
	pci_write_config(parent, 0x54, reg54 | (0x1 << devno), 2);
    else
	pci_write_config(parent, 0x54, reg54 & ~(0x1 << devno), 2);

    if (mode >= ATA_UDMA5)
	pci_write_config(parent, 0x54, reg54 | (0x1000 << devno), 2);
    else 
	pci_write_config(parent, 0x54, reg54 & ~(0x1000 << devno), 2);

    reg40 &= ~0x00ff00ff;
    reg40 |= 0x40774077;

    if (atadev->unit == ATA_MASTER) {
	mask40 = 0x3300;
	new40 = timings[ata_mode2idx(mode)] << 8;
    }
    else {
	mask44 = 0x0f;
	new44 = ((timings[ata_mode2idx(mode)] & 0x30) >> 2) |
		(timings[ata_mode2idx(mode)] & 0x03);
    }
    if (atadev->channel->unit) {
	mask40 <<= 16;
	new40 <<= 16;
	mask44 <<= 4;
	new44 <<= 4;
    }
    pci_write_config(parent, 0x40, (reg40 & ~mask40) | new40, 4);
    pci_write_config(parent, 0x44, (reg44 & ~mask44) | new44, 1);

    atadev->mode = mode;
}

/*
 * Integrated Technology Express Inc. (ITE) chipset support functions
 */
int
ata_ite_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (pci_get_devid(dev) == ATA_IT8212F) {
	device_set_desc(dev, "ITE IT8212F ATA133 controller");
	ctlr->chipinit = ata_ite_chipinit;
	return 0;
    }
    return ENXIO;
}

static int
ata_ite_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    ctlr->setmode = ata_ite_setmode;

    /* set PCI mode and 66Mhz reference clock */
    pci_write_config(dev, 0x50, pci_read_config(dev, 0x50, 1) & ~0x83, 1);

    /* set default active & recover timings */
    pci_write_config(dev, 0x54, 0x31, 1);
    pci_write_config(dev, 0x56, 0x31, 1);
    return 0;
}
 
static void
ata_ite_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_channel *ch = atadev->channel;
    int devno = (ch->unit << 1) + ATA_DEV(atadev->unit);
    int error;

    /* correct the mode for what the HW supports */
    mode = ata_limit_mode(atadev, mode, ATA_UDMA6);

    /* check the CBLID bits for 80 conductor cable detection */
    if (mode > ATA_UDMA2 && (pci_read_config(parent, 0x40, 2) &
			     (ch->unit ? (1<<3) : (1<<2)))) {
	ata_prtdev(atadev,"DMA limited to UDMA33, non-ATA66 cable or device\n");
	mode = ATA_UDMA2;
    }

    /* set the wanted mode on the device */
    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%s setting %s on ITE8212F chip\n",
		   (error) ? "failed" : "success", ata_mode2str(mode));

    /* if the device accepted the mode change, setup the HW accordingly */
    if (!error) {
	if (mode >= ATA_UDMA0) {
	    u_int8_t udmatiming[] =
		{ 0x44, 0x42, 0x31, 0x21, 0x11, 0xa2, 0x91 };

	    /* enable UDMA mode */
	    pci_write_config(parent, 0x50,
			     pci_read_config(parent, 0x50, 1) &
			     ~(1 << (devno + 3)), 1);

	    /* set UDMA timing */
	    pci_write_config(parent,
			     0x56 + (ch->unit << 2) + ATA_DEV(atadev->unit),
			     udmatiming[mode & ATA_MODE_MASK], 1);
	}
	else {
	    u_int8_t chtiming[] =
		{ 0xaa, 0xa3, 0xa1, 0x33, 0x31, 0x88, 0x32, 0x31 };

	    /* disable UDMA mode */
	    pci_write_config(parent, 0x50,
			     pci_read_config(parent, 0x50, 1) |
			     (1 << (devno + 3)), 1);

	    /* set active and recover timing (shared between master & slave) */
	    if (pci_read_config(parent, 0x54 + (ch->unit << 2), 1) <
		chtiming[ata_mode2idx(mode)])
	        pci_write_config(parent, 0x54 + (ch->unit << 2),
			         chtiming[ata_mode2idx(mode)], 1);
	}
	atadev->mode = mode;
    }
}
/*
 * National chipset support functions
 */
int
ata_national_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    /* this chip is a clone of the Cyrix chip, bugs and all */
    if (pci_get_devid(dev) == ATA_SC1100) {
	device_set_desc(dev, "National Geode SC1100 ATA33 controller");
	ctlr->chipinit = ata_national_chipinit;
	return 0;
    }
    return ENXIO;
}
    
static device_t nat_host = NULL;

static int
ata_national_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    device_t *children;
    int nchildren, i;
    
    if (ata_setup_interrupt(dev))
	return ENXIO;
		    
    /* locate the ISA part in the southbridge and enable UDMA33 */
    if (!device_get_children(device_get_parent(dev), &children,&nchildren)){
	for (i = 0; i < nchildren; i++) {
	    if (pci_get_devid(children[i]) == 0x0510100b) {
		nat_host = children[i];
		break;
	    }
	}
	free(children, M_TEMP);
    }
    ctlr->setmode = ata_national_setmode;
    return 0;
}

static void
ata_national_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    u_int32_t piotiming[] =
       { 0x9172d132, 0x21717121, 0x00803020, 0x20102010, 0x00100010,
	  0x00803020, 0x20102010, 0x00100010,
	  0x00100010, 0x00100010, 0x00100010 };
    u_int32_t dmatiming[] = { 0x80077771, 0x80012121, 0x80002020 };
    u_int32_t udmatiming[] = { 0x80921250, 0x80911140, 0x80911030 };
    int error;

    atadev->channel->dma->alignment = 16;
    atadev->channel->dma->max_iosize = 126 * DEV_BSIZE;

    mode = ata_limit_mode(atadev, mode, ATA_UDMA2);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%s setting %s on National chip\n",
		   (error) ? "failed" : "success", ata_mode2str(mode));
    if (!error) {
	if (mode >= ATA_UDMA0) {
	    pci_write_config(parent, 0x44 + (devno << 3),
			     udmatiming[mode & ATA_MODE_MASK], 4);
	}
	else if (mode >= ATA_WDMA0) {
	    pci_write_config(parent, 0x44 + (devno << 3),
			     dmatiming[mode & ATA_MODE_MASK], 4);
	}
	else {
	    pci_write_config(parent, 0x44 + (devno << 3),
			     pci_read_config(parent, 0x44 + (devno << 3), 4) |
			     0x80000000, 4);
	}
	pci_write_config(parent, 0x40 + (devno << 3),
			 piotiming[ata_mode2idx(mode)], 4);
	atadev->mode = mode;
    }
}

/*
 * nVidia chipset support functions
 */
int
ata_nvidia_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_NFORCE1,     0, AMDNVIDIA, NVIDIA, ATA_UDMA5, "nVidia nForce" },
     { ATA_NFORCE2,     0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce2" },
     { ATA_NFORCE2_MCP, 0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce2 MCP" },
     { ATA_NFORCE3,     0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce3" },
     { ATA_NFORCE3_PRO, 0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce3 Pro" },
     { ATA_NFORCE3_MCP, 0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce3 MCP" },
     { ATA_NFORCE4,     0, AMDNVIDIA, NVIDIA, ATA_UDMA6, "nVidia nForce4" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64];

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_nvidia_chipinit;
    return 0;
}

static int
ata_nvidia_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    /* disable prefetch, postwrite */
    pci_write_config(dev, 0x51, pci_read_config(dev, 0x51, 1) & 0x0f, 1);

    ctlr->setmode = ata_via_family_setmode;
    return 0;
}

/*
 * Promise chipset support functions
 */
#define ATA_PDC_APKT_OFFSET	0x00000010 
#define ATA_PDC_HPKT_OFFSET	0x00000040
#define ATA_PDC_ASG_OFFSET	0x00000080
#define ATA_PDC_LSG_OFFSET	0x000000c0
#define ATA_PDC_HSG_OFFSET	0x00000100
#define ATA_PDC_CHN_OFFSET	0x00000400
#define ATA_PDC_BUF_BASE	0x00400000
#define ATA_PDC_BUF_OFFSET	0x00100000
#define ATA_PDC_MAX_HPKT	8
#define ATA_PDC_WRITE_REG	0x00
#define ATA_PDC_WRITE_CTL	0x0e
#define ATA_PDC_WRITE_END	0x08
#define ATA_PDC_WAIT_NBUSY	0x10
#define ATA_PDC_WAIT_READY	0x18
#define ATA_PDC_1B		0x20
#define ATA_PDC_2B		0x40

struct ata_promise_sx4 {
    struct mtx	mtx;
    u_int32_t	array[ATA_PDC_MAX_HPKT];
    int		head, tail;
    int		busy;
};

int
ata_promise_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_PDC20246,  0, PROLD, 0x00,	 ATA_UDMA2, "Promise PDC20246" },
     { ATA_PDC20262,  0, PRNEW, 0x00,	 ATA_UDMA4, "Promise PDC20262" },
     { ATA_PDC20263,  0, PRNEW, 0x00,	 ATA_UDMA4, "Promise PDC20263" },
     { ATA_PDC20265,  0, PRNEW, 0x00,	 ATA_UDMA5, "Promise PDC20265" },
     { ATA_PDC20267,  0, PRNEW, 0x00,	 ATA_UDMA5, "Promise PDC20267" },
     { ATA_PDC20268,  0, PRTX,	PRTX4,	 ATA_UDMA5, "Promise PDC20268" },
     { ATA_PDC20269,  0, PRTX,	0x00,	 ATA_UDMA6, "Promise PDC20269" },
     { ATA_PDC20270,  0, PRTX,	PRTX4,	 ATA_UDMA5, "Promise PDC20270" },
     { ATA_PDC20271,  0, PRTX,	0x00,	 ATA_UDMA6, "Promise PDC20271" },
     { ATA_PDC20275,  0, PRTX,	0x00,	 ATA_UDMA6, "Promise PDC20275" },
     { ATA_PDC20276,  0, PRTX,	PRSX6K,  ATA_UDMA6, "Promise PDC20276" },
     { ATA_PDC20277,  0, PRTX,	0x00,	 ATA_UDMA6, "Promise PDC20277" },
     { ATA_PDC20318,  0, PRMIO, PRSATA,  ATA_SA150, "Promise PDC20318" },
     { ATA_PDC20319,  0, PRMIO, PRSATA,  ATA_SA150, "Promise PDC20319" },
     { ATA_PDC20371,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20371" },
     { ATA_PDC20375,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20375" },
     { ATA_PDC20376,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20376" },
     { ATA_PDC20377,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20377" },
     { ATA_PDC20378,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20378" },
     { ATA_PDC20379,  0, PRMIO, PRCMBO,  ATA_SA150, "Promise PDC20379" },
     { ATA_PDC20571,  0, PRMIO, PRCMBO2, ATA_SA150, "Promise PDC20571" },
     { ATA_PDC20575,  0, PRMIO, PRCMBO2, ATA_SA150, "Promise PDC20575" },
     { ATA_PDC20579,  0, PRMIO, PRCMBO2, ATA_SA150, "Promise PDC20579" },
     { ATA_PDC20580,  0, PRMIO, PRCMBO2, ATA_SA150, "Promise PDC20580" },
     { ATA_PDC20617,  0, PRMIO, PRPATA,  ATA_UDMA6, "Promise PDC20617" },
     { ATA_PDC20618,  0, PRMIO, PRPATA,  ATA_UDMA6, "Promise PDC20618" },
     { ATA_PDC20619,  0, PRMIO, PRPATA,  ATA_UDMA6, "Promise PDC20619" },
     { ATA_PDC20620,  0, PRMIO, PRPATA,  ATA_UDMA6, "Promise PDC20620" },
     { ATA_PDC20621,  0, PRMIO, PRSX4X,  ATA_UDMA5, "Promise PDC20621" },
     { ATA_PDC20622,  0, PRMIO, PRSX4X,  ATA_SA150, "Promise PDC20622" },
     { ATA_PDC40518,  0, PRMIO, PRSATA2, ATA_SA150, "Promise PDC40518" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64];
    uintptr_t devid = 0;

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    /* if we are on a SuperTrak SX6000 dont attach */
    if ((idx->cfg2 & PRSX6K) && pci_get_class(GRANDPARENT(dev))==PCIC_BRIDGE &&
	!BUS_READ_IVAR(device_get_parent(GRANDPARENT(dev)),
		       GRANDPARENT(dev), PCI_IVAR_DEVID, &devid) &&
	devid == ATA_I960RM) 
	return ENXIO;

    strcpy(buffer, idx->text);

    /* if we are on a FastTrak TX4, adjust the interrupt resource */
    if ((idx->cfg2 & PRTX4) && pci_get_class(GRANDPARENT(dev))==PCIC_BRIDGE &&
	!BUS_READ_IVAR(device_get_parent(GRANDPARENT(dev)),
		       GRANDPARENT(dev), PCI_IVAR_DEVID, &devid) &&
	((devid == ATA_DEC_21150) || (devid == ATA_DEC_21150_1))) {
	static long start = 0, end = 0;

	if (pci_get_slot(dev) == 1) {
	    bus_get_resource(dev, SYS_RES_IRQ, 0, &start, &end);
	    strcat(buffer, " (channel 0+1)");
	}
	else if (pci_get_slot(dev) == 2 && start && end) {
	    bus_set_resource(dev, SYS_RES_IRQ, 0, start, end);
	    start = end = 0;
	    strcat(buffer, " (channel 2+3)");
	}
	else {
	    start = end = 0;
	}
    }
    sprintf(buffer, "%s %s controller", buffer, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_promise_chipinit;
    return 0;
}

static int
ata_promise_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
					       RF_SHAREABLE | RF_ACTIVE))) {
	device_printf(dev, "unable to map interrupt\n");
	return ENXIO;
    }

    if (ctlr->chip->max_dma >= ATA_SA150)
	ctlr->setmode = ata_sata_setmode;
    else
	ctlr->setmode = ata_promise_setmode;

    switch  (ctlr->chip->cfg1) {
    case PRNEW:
	/* setup clocks */
	ATA_OUTB(ctlr->r_res1, 0x11, ATA_INB(ctlr->r_res1, 0x11) | 0x0a);

	ctlr->dmainit = ata_promise_new_dmainit;
	/* FALLTHROUGH */

    case PROLD:
	/* enable burst mode */
	ATA_OUTB(ctlr->r_res1, 0x1f, ATA_INB(ctlr->r_res1, 0x1f) | 0x01);

	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_promise_old_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}
	break;

    case PRTX:
	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_promise_tx2_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}
	break;

    case PRMIO:
//	if (ctlr->r_res1)
//	    bus_release_resource(dev, ctlr->r_type1, ctlr->r_rid1,ctlr->r_res1);
	ctlr->r_type1 = SYS_RES_MEMORY;
	ctlr->r_rid1 = PCIR_BAR(4);
	if (!(ctlr->r_res1 = bus_alloc_resource_any(dev, ctlr->r_type1,
						    &ctlr->r_rid1, RF_ACTIVE)))
	    return ENXIO;

	ctlr->r_type2 = SYS_RES_MEMORY;
	ctlr->r_rid2 = PCIR_BAR(3);
	if (!(ctlr->r_res2 = bus_alloc_resource_any(dev, ctlr->r_type2,
						    &ctlr->r_rid2, RF_ACTIVE)))
	    return ENXIO;

	ctlr->reset = ata_promise_mio_reset;
	ctlr->dmainit = ata_promise_mio_dmainit;
	ctlr->allocate = ata_promise_mio_allocate;

	switch (ctlr->chip->cfg2) {
	case PRPATA:
            ctlr->channels = ((ATA_INL(ctlr->r_res2, 0x48) & 0x01) > 0) +
                             ((ATA_INL(ctlr->r_res2, 0x48) & 0x02) > 0) + 2;
	    break;

	case PRCMBO:
            ATA_OUTL(ctlr->r_res2, 0x06c, 0x000000ff);
            ctlr->channels = ((ATA_INL(ctlr->r_res2, 0x48) & 0x02) > 0) + 3;
	    break;

	case PRSATA:
            ATA_OUTL(ctlr->r_res2, 0x06c, 0x000000ff);
            ctlr->channels = 4;
	    break;

	case PRCMBO2:
            ATA_OUTL(ctlr->r_res2, 0x060, 0x000000ff);
            ctlr->channels = 3;
	    break;

	case PRSATA2:
            ATA_OUTL(ctlr->r_res2, 0x060, 0x000000ff);
            ctlr->channels = 4;
	    break;

	case PRSX4X: {
	    struct ata_promise_sx4 *hpkt;
	    u_int32_t dimm = ATA_INL(ctlr->r_res2, 0x000c0080);

	    /* print info about cache memory */
	    device_printf(dev, "DIMM size %dMB @ 0x%08x%s\n",
			  (((dimm >> 16) & 0xff)-((dimm >> 24) & 0xff)+1) << 4,
			  ((dimm >> 24) & 0xff),
			  ATA_INL(ctlr->r_res2, 0x000c0088) & (1<<16) ?
			  " ECC enabled" : "" );

	    ATA_OUTL(ctlr->r_res2, 0x000c000c, 
		     (ATA_INL(ctlr->r_res2, 0x000c000c) & 0xffff0000));

	    ctlr->driver = malloc(sizeof(struct ata_promise_sx4),
				  M_TEMP, M_NOWAIT | M_ZERO);
	    hpkt = ctlr->driver;
	    mtx_init(&hpkt->mtx, "ATA promise HPKT lock", NULL, MTX_DEF);
	    hpkt->busy = hpkt->head = hpkt->tail = 0;

            ctlr->channels = 4;

	    if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
				ata_promise_sx4_intr, ctlr, &ctlr->handle))) {
		device_printf(dev, "unable to setup interrupt\n");
		return ENXIO;
	    }
	    return 0;
	    }
	}

	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_promise_mio_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}
	return 0;
    }
    return ENXIO;
}

static int
ata_promise_mio_allocate(device_t dev, struct ata_channel *ch)
{
    struct ata_pci_controller *ctlr = device_get_softc(device_get_parent(dev));
    int offset = (ctlr->chip->cfg2 & PRSX4X) ? 0x000c0000 : 0;
    int i;
 
    for (i = ATA_DATA; i <= ATA_STATUS; i++) {
	ch->r_io[i].res = ctlr->r_res2;
	ch->r_io[i].offset = offset + 0x0200 + (i << 2) + (ch->unit << 7); 
    }
    ch->r_io[ATA_ALTSTAT].res = ctlr->r_res2;
    ch->r_io[ATA_ALTSTAT].offset = offset + 0x0238 + (ch->unit << 7);
    ch->r_io[ATA_IDX_ADDR].res = ctlr->r_res2;
    ch->flags |= ATA_USE_16BIT;
    if ((ctlr->chip->cfg2 & (PRSATA | PRSATA2)) ||
	((ctlr->chip->cfg2 & (PRCMBO | PRCMBO2)) && ch->unit < 2))
	ch->flags |= ATA_NO_SLAVE;
    ata_generic_hw(ch);
    if (ctlr->chip->cfg2 & PRSX4X)
	ch->hw.command = ata_promise_sx4_command;
    else
	ch->hw.command = ata_promise_mio_command;
    return 0;
}

static void
ata_promise_mio_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    u_int32_t vector = ATA_INL(ctlr->r_res2, 0x00040);
    u_int32_t status = 0;
    int unit;

    if (ctlr->chip->cfg2 & (PRSATA | PRCMBO)) {
	status = ATA_INL(ctlr->r_res2, 0x06c);
	ATA_OUTL(ctlr->r_res2, 0x06c, status & 0x000000ff);
    }
    if (ctlr->chip->cfg2 & (PRSATA2 | PRCMBO2)) {
	ATA_OUTL(ctlr->r_res2, 0x040, vector & 0x0000ffff);
	status = ATA_INL(ctlr->r_res2, 0x060);
	ATA_OUTL(ctlr->r_res2, 0x060, status & 0x000000ff);
    }
    for (unit = 0; unit < ctlr->channels; unit++) {
	if (status & (0x00000011 << unit))
	    if ((ch = ctlr->interrupt[unit].argument))
		ata_promise_mio_reset(ch);
	if (vector & (1 << (unit + 1)))
	    if ((ch = ctlr->interrupt[unit].argument))
		ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_promise_sx4_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    u_int32_t vector = ATA_INL(ctlr->r_res2, 0x000c0480);
    int unit;

    for (unit = 0; unit < ctlr->channels; unit++) {
	if (vector & (1 << (unit + 1)))
	    if ((ch = ctlr->interrupt[unit].argument))
		ctlr->interrupt[unit].function(ch);
	if (vector & (1 << (unit + 5)))
	    if ((ch = ctlr->interrupt[unit].argument))
		ata_promise_queue_hpkt(ctlr,
				       htole32((ch->unit * ATA_PDC_CHN_OFFSET) +
					       ATA_PDC_HPKT_OFFSET));
	if (vector & (1 << (unit + 9))) {
	    ata_promise_next_hpkt(ctlr);
	    if ((ch = ctlr->interrupt[unit].argument))
		ctlr->interrupt[unit].function(ch);
	}
	if (vector & (1 << (unit + 13))) {
	    ata_promise_next_hpkt(ctlr);
	    if ((ch = ctlr->interrupt[unit].argument))
		ATA_OUTL(ctlr->r_res2, 0x000c0240 + (ch->unit << 7),
			 htole32((ch->unit * ATA_PDC_CHN_OFFSET) +
			 ATA_PDC_APKT_OFFSET));
	}
    }
}

static int
ata_promise_mio_dmastart(struct ata_channel *ch)
{
    ch->flags |= ATA_DMA_ACTIVE;
    return 0;
}

static int
ata_promise_mio_dmastop(struct ata_channel *ch)
{
    ch->flags &= ~ATA_DMA_ACTIVE;
    /* get status XXX SOS */
    return 0;
}

static void
ata_promise_mio_dmainit(struct ata_channel *ch)
{
    ata_dmainit(ch);
    if (ch->dma) {
	ch->dma->start = ata_promise_mio_dmastart;
	ch->dma->stop = ata_promise_mio_dmastop;
    }
}

static void
ata_promise_mio_reset(struct ata_channel *ch)
{
    struct ata_pci_controller *ctlr = 
	device_get_softc(device_get_parent(ch->dev));

    switch (ctlr->chip->cfg2) {
    case PRSX4X: {
	struct ata_promise_sx4 *hpktp = ctlr->driver;

	/* softreset channel ATA module */
	ATA_OUTL(ctlr->r_res2, 0xc0260 + (ch->unit << 7), ch->unit + 1);
	DELAY(1000);
	ATA_OUTL(ctlr->r_res2, 0xc0260 + (ch->unit << 7),
		 (ATA_INL(ctlr->r_res2, 0xc0260 + (ch->unit << 7)) &
		  ~0x00003f9f) | (ch->unit + 1));

	/* softreset HOST module */
	mtx_lock(&hpktp->mtx);
	ATA_OUTL(ctlr->r_res2, 0xc012c,
		 (ATA_INL(ctlr->r_res2, 0xc012c) & ~0x00000f9f) | (1 << 11));
	DELAY(10);
	ATA_OUTL(ctlr->r_res2, 0xc012c,
		 (ATA_INL(ctlr->r_res2, 0xc012c) & ~0x00000f9f));
	mtx_unlock(&hpktp->mtx);
        }
        break;

    case PRCMBO:
    case PRCMBO2:
	/* softreset channel ATA module */
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7), (1 << 11));
	ata_udelay(10000);
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7),
		 (ATA_INL(ctlr->r_res2, 0x0260 + (ch->unit << 7)) &
		  ~0x00003f9f) | (ch->unit + 1));
	break;

    case PRSATA: {
	u_int32_t status = 0;
	int timeout;

	/* mask plug/unplug intr */
	ATA_OUTL(ctlr->r_res2, 0x06c, (0x00110000 << ch->unit));

	/* softreset channels ATA module */
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7), (1 << 11));
	ata_udelay(10000);
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7),
		 (ATA_INL(ctlr->r_res2, 0x0260 + (ch->unit << 7)) &
		  ~0x00003f9f) | (ch->unit + 1));

	/* enable PHY XXX SOS */
	/* wait up to 1 sec for "connect well" */
	for (timeout = 0; timeout > 1000000 ; timeout += 100) {
		status = ATA_INL(ctlr->r_res2, 0x400 + (ch->unit << 8));

	    if ((status & 0x313) == 0x112)
		break;
	    ata_udelay(10000);
	}
	if (timeout >= 1000000)
	    device_printf(ch->dev, "connect status=%08x\n", status);

	/* reset and enable plug/unplug intr */
	ATA_OUTL(ctlr->r_res2, 0x06c, (0x00000011 << ch->unit));
        }
	break;

    case PRSATA2: {
	u_int32_t status = 0;
	int timeout;

	/* set portmultiplier port */
	ATA_OUTL(ctlr->r_res2, 0x4e8 + (ch->unit << 8), 0x0f);

	/* mask plug/unplug intr */
	ATA_OUTL(ctlr->r_res2, 0x060, (0x00110000 << ch->unit));

	/* softreset channels ATA module */
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7), (1 << 11));
	ata_udelay(10000);
	ATA_OUTL(ctlr->r_res2, 0x0260 + (ch->unit << 7),
		 (ATA_INL(ctlr->r_res2, 0x0260 + (ch->unit << 7)) &
		  ~0x00003f9f) | (ch->unit + 1));

	/* enable PHY XXX SOS */
	/* set PHY mode to "improved" */
	ATA_OUTL(ctlr->r_res2, 0x414 + (ch->unit << 8),
		 (ATA_INL(ctlr->r_res2, 0x414 + (ch->unit << 8)) &
		 ~0x00000003) | 0x00000001);

	/* wait up to 1 sec for "connect well" */
	for (timeout = 0; timeout > 1000000 ; timeout += 100) {
	    status = ATA_INL(ctlr->r_res2, 0x400 + (ch->unit << 8));

	    if ((status & 0x737) == 0x113 || (status & 0x727) == 0x123)
		break;
	    ata_udelay(10000);
	}
	if (timeout >= 1000000)
	    device_printf(ch->dev, "connect status=%08x\n", status);

	/* reset and enable plug/unplug intr */
	ATA_OUTL(ctlr->r_res2, 0x060, (0x00000011 << ch->unit));

	/* set portmultiplier port */
	ATA_OUTL(ctlr->r_res2, 0x4e8 + (ch->unit << 8), 0x00);
	}
	break;
    }
}

static int
ata_promise_mio_command(struct ata_device *atadev, u_int8_t command,
			u_int64_t lba, u_int16_t count, u_int16_t feature)
{
    struct ata_pci_controller *ctlr = 
	device_get_softc(device_get_parent(atadev->channel->dev));
    u_int32_t *wordp = (u_int32_t *)atadev->channel->dma->workspace;

    ATA_OUTL(ctlr->r_res2, (atadev->channel->unit + 1) << 2, 0x00000001);

    switch (command) {
    default:
	return ata_generic_command(atadev, command, lba, count, feature);

    case ATA_READ_DMA:
	wordp[0] = htole32(0x04 | ((atadev->channel->unit+1)<<16) | (0x00<<24));
	break;

    case ATA_WRITE_DMA:
	wordp[0] = htole32(0x00 | ((atadev->channel->unit+1)<<16) | (0x00<<24));
	break;
    }
    wordp[1] = htole32(atadev->channel->dma->mdmatab);
    wordp[2] = 0;
    ata_promise_apkt((u_int8_t*)wordp, atadev, command, lba, count, feature);

    ATA_OUTL(ctlr->r_res2, 0x0240 + (atadev->channel->unit << 7),
	     atadev->channel->dma->wdmatab);
    return 0;
}

static int
ata_promise_sx4_command(struct ata_device *atadev, u_int8_t command,
			u_int64_t lba, u_int16_t count, u_int16_t feature)
{
    struct ata_channel *ch = atadev->channel;
    struct ata_dma_prdentry *prd = ch->dma->dmatab;
    struct ata_pci_controller *ctlr = 
	device_get_softc(device_get_parent(ch->dev));
    caddr_t window = rman_get_virtual(ctlr->r_res1);
    u_int32_t *wordp;
    int i, idx, length = 0;

    switch (command) {    

    default:
	return -1;

    case ATA_ATA_IDENTIFY:
    case ATA_READ:
    case ATA_READ_MUL:
    case ATA_WRITE:
    case ATA_WRITE_MUL:
	ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit + 1) << 2), 0x00000001);
	return ata_generic_command(atadev, command, lba, count, feature);

    case ATA_SETFEATURES:
    case ATA_FLUSHCACHE:
    case ATA_SLEEP:
    case ATA_SET_MULTI:
	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_APKT_OFFSET);
	wordp[0] = htole32(0x08 | ((ch->unit + 1)<<16) | (0x00 << 24));
	wordp[1] = 0;
	wordp[2] = 0;
	ata_promise_apkt((u_int8_t *)wordp, atadev, command, lba,count,feature);
	ATA_OUTL(ctlr->r_res2, 0x000c0484, 0x00000001);
	ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit + 1) << 2), 0x00000001);
	ATA_OUTL(ctlr->r_res2, 0x000c0240 + (ch->unit << 7),
		 htole32((ch->unit * ATA_PDC_CHN_OFFSET)+ATA_PDC_APKT_OFFSET));
	return 0;

    case ATA_READ_DMA:
    case ATA_WRITE_DMA:
	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_HSG_OFFSET);
	i = idx = 0;
	do {
	    wordp[idx++] = htole32(prd[i].addr);
	    wordp[idx++] = htole32(prd[i].count & ~ATA_DMA_EOT);
	    length += (prd[i].count & ~ATA_DMA_EOT);
	} while (!(prd[i++].count & ATA_DMA_EOT));
	wordp[idx - 1] |= htole32(ATA_DMA_EOT);

	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_LSG_OFFSET);
	wordp[0] = htole32((ch->unit * ATA_PDC_BUF_OFFSET) + ATA_PDC_BUF_BASE);
	wordp[1] = htole32((count * DEV_BSIZE) | ATA_DMA_EOT);

	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_ASG_OFFSET);
	wordp[0] = htole32((ch->unit * ATA_PDC_BUF_OFFSET) + ATA_PDC_BUF_BASE);
	wordp[1] = htole32((count * DEV_BSIZE) | ATA_DMA_EOT);

	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_HPKT_OFFSET);
	if (command == ATA_READ_DMA)
	    wordp[0] = htole32(0x14 | ((ch->unit+9)<<16) | ((ch->unit+5)<<24));
	if (command == ATA_WRITE_DMA)
	    wordp[0] = htole32(0x00 | ((ch->unit+13)<<16) | (0x00<<24));
	wordp[1] = htole32((ch->unit * ATA_PDC_CHN_OFFSET)+ATA_PDC_HSG_OFFSET);
	wordp[2] = htole32((ch->unit * ATA_PDC_CHN_OFFSET)+ATA_PDC_LSG_OFFSET);
	wordp[3] = 0;

	wordp = (u_int32_t *)
	    (window + (ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_APKT_OFFSET);
	if (command == ATA_READ_DMA)
	    wordp[0] = htole32(0x04 | ((ch->unit+5)<<16) | (0x00<<24));
	if (command == ATA_WRITE_DMA)
	    wordp[0] = htole32(0x10 | ((ch->unit+1)<<16) | ((ch->unit+13)<<24));
	wordp[1] = htole32((ch->unit * ATA_PDC_CHN_OFFSET)+ATA_PDC_ASG_OFFSET);
	wordp[2] = 0;
	ata_promise_apkt((u_int8_t *)wordp, atadev, command, lba,count,feature);
	ATA_OUTL(ctlr->r_res2, 0x000c0484, 0x00000001);

	if (command == ATA_READ_DMA) {
	    ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit+5)<<2), 0x00000001);
	    ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit+9)<<2), 0x00000001);
	    ATA_OUTL(ctlr->r_res2, 0x000c0240 + (ch->unit << 7),
		htole32((ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_APKT_OFFSET));
	}
	if (command == ATA_WRITE_DMA) {
	    ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit+1)<<2), 0x00000001);
	    ATA_OUTL(ctlr->r_res2, 0x000c0400 + ((ch->unit+13)<<2), 0x00000001);
	    ata_promise_queue_hpkt(ctlr,
		htole32((ch->unit * ATA_PDC_CHN_OFFSET) + ATA_PDC_HPKT_OFFSET));
	}
	return 0;
    }
}

static int
ata_promise_apkt(u_int8_t *bytep, struct ata_device *atadev, u_int8_t command,
		 u_int64_t lba, u_int16_t count, u_int16_t feature)
{ 
    int i = 12;

    bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_PDC_WAIT_NBUSY|ATA_DRIVE;
    bytep[i++] = ATA_D_IBM | ATA_D_LBA | atadev->unit;
    bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_CTL;
    bytep[i++] = ATA_A_4BIT;

    if ((lba >= ATA_MAX_28BIT_LBA || count > 256) && atadev->param &&
	(atadev->param->support.command2 & ATA_SUPPORT_ADDRESS48)) {
	atadev->channel->flags |= ATA_48BIT_ACTIVE;
	if (command == ATA_READ_DMA)
	    command = ATA_READ_DMA48;
	if (command == ATA_WRITE_DMA)
	    command = ATA_WRITE_DMA48;
	bytep[i++] = ATA_PDC_2B | ATA_PDC_WRITE_REG | ATA_FEATURE;
	bytep[i++] = (feature >> 8) & 0xff;
	bytep[i++] = feature & 0xff;
	bytep[i++] = ATA_PDC_2B | ATA_PDC_WRITE_REG | ATA_COUNT;
	bytep[i++] = (count >> 8) & 0xff;
	bytep[i++] = count & 0xff;
	bytep[i++] = ATA_PDC_2B | ATA_PDC_WRITE_REG | ATA_SECTOR;
	bytep[i++] = (lba >> 24) & 0xff;
	bytep[i++] = lba & 0xff;
	bytep[i++] = ATA_PDC_2B | ATA_PDC_WRITE_REG | ATA_CYL_LSB;
	bytep[i++] = (lba >> 32) & 0xff;
	bytep[i++] = (lba >> 8) & 0xff;
	bytep[i++] = ATA_PDC_2B | ATA_PDC_WRITE_REG | ATA_CYL_MSB;
	bytep[i++] = (lba >> 40) & 0xff;
	bytep[i++] = (lba >> 16) & 0xff;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_DRIVE;
	bytep[i++] = ATA_D_LBA | atadev->unit;
    }
    else {
	atadev->channel->flags &= ~ATA_48BIT_ACTIVE;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_FEATURE;
	bytep[i++] = feature;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_COUNT;
	bytep[i++] = count;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_SECTOR;
	bytep[i++] = lba & 0xff;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_CYL_LSB;
	bytep[i++] = (lba >> 8) & 0xff;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_CYL_MSB;
	bytep[i++] = (lba >> 16) & 0xff;
	bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_REG | ATA_DRIVE;
	bytep[i++] = (atadev->flags & ATA_D_USE_CHS ? 0 : ATA_D_LBA) |
		   ATA_D_IBM | atadev->unit | ((lba >> 24) & 0xf);
    }
    bytep[i++] = ATA_PDC_1B | ATA_PDC_WRITE_END | ATA_CMD;
    bytep[i++] = command;
    return i;
}

static void
ata_promise_queue_hpkt(struct ata_pci_controller *ctlr, u_int32_t hpkt)
{
    struct ata_promise_sx4 *hpktp = ctlr->driver;

    mtx_lock(&hpktp->mtx);
    if (hpktp->tail == hpktp->head && !hpktp->busy) {
	ATA_OUTL(ctlr->r_res2, 0x000c0100, hpkt);
	hpktp->busy = 1;
    }
    else
	hpktp->array[(hpktp->head++) & (ATA_PDC_MAX_HPKT - 1)] = hpkt;
    mtx_unlock(&hpktp->mtx);
}

static void
ata_promise_next_hpkt(struct ata_pci_controller *ctlr)
{
    struct ata_promise_sx4 *hpktp = ctlr->driver;

    mtx_lock(&hpktp->mtx);
    if (hpktp->tail != hpktp->head) {
	ATA_OUTL(ctlr->r_res2, 0x000c0100, 
		 hpktp->array[(hpktp->tail++) & (ATA_PDC_MAX_HPKT - 1)]);
    }
    else
	hpktp->busy = 0;
    mtx_unlock(&hpktp->mtx);
}

static void
ata_promise_tx2_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	ATA_IDX_OUTB(ch, ATA_BMDEVSPEC_0, 0x0b);
	if (ATA_IDX_INB(ch, ATA_BMDEVSPEC_1) & 0x20) {
	    if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
		int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

		if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		    ATA_BMSTAT_INTERRUPT)
		    continue;
		ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
		DELAY(1);
	    }
	    ctlr->interrupt[unit].function(ch);
	}
    }
}

static void
ata_promise_old_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ATA_INL(ctlr->r_res1, 0x1c) & (ch->unit ? 0x00004000 : 0x00000400)){
	    if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
		int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

		if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		    ATA_BMSTAT_INTERRUPT)
		    continue;
		ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
		DELAY(1);
	    }
	    ctlr->interrupt[unit].function(ch);
	}
    }
}

static int
ata_promise_new_dmastart(struct ata_channel *ch)
{
    struct ata_pci_controller *ctlr = 
	device_get_softc(device_get_parent(ch->dev));

    if (ch->flags & ATA_48BIT_ACTIVE) {
	ATA_OUTB(ctlr->r_res1, 0x11,
		 ATA_INB(ctlr->r_res1, 0x11) | (ch->unit ? 0x08 : 0x02));
	ATA_OUTL(ctlr->r_res1, 0x20,
		 ((ch->dma->flags & ATA_DMA_READ) ? 0x05000000 : 0x06000000) |
		 (ch->dma->cur_iosize >> 1));
    }
    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, (ATA_IDX_INB(ch, ATA_BMSTAT_PORT) |
		 (ATA_BMSTAT_INTERRUPT | ATA_BMSTAT_ERROR)));
    ATA_IDX_OUTL(ch, ATA_BMDTP_PORT, ch->dma->mdmatab);
    ATA_IDX_OUTB(ch, ATA_BMCMD_PORT,
		 ((ch->dma->flags & ATA_DMA_READ) ? ATA_BMCMD_WRITE_READ : 0) |
		 ATA_BMCMD_START_STOP);
    ch->flags |= ATA_DMA_ACTIVE;
    return 0;
}

static int
ata_promise_new_dmastop(struct ata_channel *ch)
{
    struct ata_pci_controller *ctlr = 
	device_get_softc(device_get_parent(ch->dev));
    int error;

    if (ch->flags & ATA_48BIT_ACTIVE) {
	ATA_OUTB(ctlr->r_res1, 0x11,
		 ATA_INB(ctlr->r_res1, 0x11) & ~(ch->unit ? 0x08 : 0x02));
	ATA_OUTL(ctlr->r_res1, 0x20, 0);
    }
    error = ATA_IDX_INB(ch, ATA_BMSTAT_PORT);
    ATA_IDX_OUTB(ch, ATA_BMCMD_PORT,
		 ATA_IDX_INB(ch, ATA_BMCMD_PORT) & ~ATA_BMCMD_START_STOP);
    ch->flags &= ~ATA_DMA_ACTIVE;
    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, ATA_BMSTAT_INTERRUPT | ATA_BMSTAT_ERROR); 
    return error;
}

static void
ata_promise_new_dmainit(struct ata_channel *ch)
{
    ata_dmainit(ch);
    if (ch->dma) {
	ch->dma->start = ata_promise_new_dmastart;
	ch->dma->stop = ata_promise_new_dmastop;
    }
}

static void
ata_promise_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;
    u_int32_t timings33[][2] = {
    /*	  PROLD	      PRNEW		   mode */
	{ 0x004ff329, 0x004fff2f },	/* PIO 0 */
	{ 0x004fec25, 0x004ff82a },	/* PIO 1 */
	{ 0x004fe823, 0x004ff026 },	/* PIO 2 */
	{ 0x004fe622, 0x004fec24 },	/* PIO 3 */
	{ 0x004fe421, 0x004fe822 },	/* PIO 4 */
	{ 0x004567f3, 0x004acef6 },	/* MWDMA 0 */
	{ 0x004467f3, 0x0048cef6 },	/* MWDMA 1 */
	{ 0x004367f3, 0x0046cef6 },	/* MWDMA 2 */
	{ 0x004367f3, 0x0046cef6 },	/* UDMA 0 */
	{ 0x004247f3, 0x00448ef6 },	/* UDMA 1 */
	{ 0x004127f3, 0x00436ef6 },	/* UDMA 2 */
	{ 0,	      0x00424ef6 },	/* UDMA 3 */
	{ 0,	      0x004127f3 },	/* UDMA 4 */
	{ 0,	      0x004127f3 }	/* UDMA 5 */
    };

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    switch (ctlr->chip->cfg1) {
    case PROLD:
    case PRNEW:
	if (mode > ATA_UDMA2 && (pci_read_config(parent, 0x50, 2) &
				 (atadev->channel->unit ? 1 << 11 : 1 << 10))) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
	if (ATAPI_DEVICE(atadev) && mode > ATA_PIO_MAX)
	    mode = ata_limit_mode(atadev, mode, ATA_PIO_MAX);
	break;

    case PRTX:
	ATA_IDX_OUTB(atadev->channel, ATA_BMDEVSPEC_0, 0x0b);
	if (mode > ATA_UDMA2 &&
	    ATA_IDX_INB(atadev->channel, ATA_BMDEVSPEC_1) & 0x04) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
	break;
   
    case PRMIO:
	if (mode > ATA_UDMA2 &&
	    (ATA_INL(ctlr->r_res2,
		     (ctlr->chip->cfg2 & PRSX4X ? 0x000c0260 : 0x0260) +
		     (atadev->channel->unit << 7)) & 0x01000000)) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
	break;
    }

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (!error) {
	if (ctlr->chip->cfg1 < PRTX)
	    pci_write_config(parent, 0x60 + (devno << 2),
			     timings33[ctlr->chip->cfg1][ata_mode2idx(mode)],4);
	atadev->mode = mode;
    }
    return;
}

/*
 * ServerWorks chipset support functions
 */
int
ata_serverworks_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_ROSB4,  0x00, SWKS33,  0x00, ATA_UDMA2, "ServerWorks ROSB4" },
     { ATA_CSB5,   0x92, SWKS100, 0x00, ATA_UDMA5, "ServerWorks CSB5" },
     { ATA_CSB5,   0x00, SWKS66,  0x00, ATA_UDMA4, "ServerWorks CSB5" },
     { ATA_CSB6,   0x00, SWKS100, 0x00, ATA_UDMA5, "ServerWorks CSB6" },
     { ATA_CSB6_1, 0x00, SWKS66,  0x00, ATA_UDMA4, "ServerWorks CSB6" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64];

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_serverworks_chipinit;
    return 0;
}

static int
ata_serverworks_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;

    if (ctlr->chip->cfg1 == SWKS33) {
	device_t *children;
	int nchildren, i;

	/* locate the ISA part in the southbridge and enable UDMA33 */
	if (!device_get_children(device_get_parent(dev), &children,&nchildren)){
	    for (i = 0; i < nchildren; i++) {
		if (pci_get_devid(children[i]) == ATA_ROSB4_ISA) {
		    pci_write_config(children[i], 0x64,
				     (pci_read_config(children[i], 0x64, 4) &
				      ~0x00002000) | 0x00004000, 4);
		    break;
		}
	    }
	    free(children, M_TEMP);
	}
    }
    else {
	pci_write_config(dev, 0x5a,
			 (pci_read_config(dev, 0x5a, 1) & ~0x40) |
			 (ctlr->chip->cfg1 == SWKS100) ? 0x03 : 0x02, 1);
    }
    ctlr->setmode = ata_serverworks_setmode;
    return 0;
}

static void
ata_serverworks_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int offset = (devno ^ 0x01) << 3;
    int error;
    u_int8_t piotimings[] = { 0x5d, 0x47, 0x34, 0x22, 0x20, 0x34, 0x22, 0x20,
			      0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };
    u_int8_t dmatimings[] = { 0x77, 0x21, 0x20 };

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    mode = ata_check_80pin(atadev, mode);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (!error) {
	if (mode >= ATA_UDMA0) {
	    pci_write_config(parent, 0x56, 
			     (pci_read_config(parent, 0x56, 2) &
			      ~(0xf << (devno << 2))) |
			     ((mode & ATA_MODE_MASK) << (devno << 2)), 2);
	    pci_write_config(parent, 0x54,
			     pci_read_config(parent, 0x54, 1) |
			     (0x01 << devno), 1);
	    pci_write_config(parent, 0x44, 
			     (pci_read_config(parent, 0x44, 4) &
			      ~(0xff << offset)) |
			     (dmatimings[2] << offset), 4);
	}
	else if (mode >= ATA_WDMA0) {
	    pci_write_config(parent, 0x54,
			     pci_read_config(parent, 0x54, 1) &
			      ~(0x01 << devno), 1);
	    pci_write_config(parent, 0x44, 
			     (pci_read_config(parent, 0x44, 4) &
			      ~(0xff << offset)) |
			     (dmatimings[mode & ATA_MODE_MASK] << offset),4);
	}
	else
	    pci_write_config(parent, 0x54,
			     pci_read_config(parent, 0x54, 1) &
			     ~(0x01 << devno), 1);

	pci_write_config(parent, 0x40, 
			 (pci_read_config(parent, 0x40, 4) &
			  ~(0xff << offset)) |
			 (piotimings[ata_mode2idx(mode)] << offset), 4);
	atadev->mode = mode;
    }
}

/*
 * Silicon Image (former CMD) chipset support functions
 */
int
ata_sii_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_SII3114,   0x00, SIIMEMIO, SII4CH,	 ATA_SA150, "SiI 3114" },
     { ATA_SII3512,   0x02, SIIMEMIO, 0,	 ATA_SA150, "SiI 3512" },
     { ATA_SII3112,   0x02, SIIMEMIO, 0,	 ATA_SA150, "SiI 3112" },
     { ATA_SII3112_1, 0x02, SIIMEMIO, 0,	 ATA_SA150, "SiI 3112" },
     { ATA_SII3512,   0x00, SIIMEMIO, SIIBUG,	 ATA_SA150, "SiI 3512" },
     { ATA_SII3112,   0x00, SIIMEMIO, SIIBUG,	 ATA_SA150, "SiI 3112" },
     { ATA_SII3112_1, 0x00, SIIMEMIO, SIIBUG,	 ATA_SA150, "SiI 3112" },
     { ATA_SII0680,   0x00, SIIMEMIO, SIISETCLK, ATA_UDMA6, "SiI 0680" },
     { ATA_CMD649,    0x00, 0,	      SIIINTR,	 ATA_UDMA5, "CMD 649" },
     { ATA_CMD648,    0x00, 0,	      SIIINTR,	 ATA_UDMA4, "CMD 648" },
     { ATA_CMD646,    0x07, 0,	      0,	 ATA_UDMA2, "CMD 646U2" },
     { ATA_CMD646,    0x00, 0,	      0,	 ATA_WDMA2, "CMD 646" },
     { 0, 0, 0, 0, 0, 0}};
    char buffer[64];

    if (!(idx = ata_match_chip(dev, ids)))
	return ENXIO;

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_sii_chipinit;
    return 0;
}

static int
ata_sii_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
					       RF_SHAREABLE | RF_ACTIVE))) {
	device_printf(dev, "unable to map interrupt\n");
	return ENXIO;
    }

    if (ctlr->chip->cfg1 == SIIMEMIO) {
	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_sii_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}

	ctlr->r_type2 = SYS_RES_MEMORY;
	ctlr->r_rid2 = PCIR_BAR(5);
	if (!(ctlr->r_res2 = bus_alloc_resource_any(dev, ctlr->r_type2,
						    &ctlr->r_rid2, RF_ACTIVE)))
	    return ENXIO;

	if (ctlr->chip->cfg2 & SIISETCLK) {
	    if ((pci_read_config(dev, 0x8a, 1) & 0x30) != 0x10)
		pci_write_config(dev, 0x8a, 
				 (pci_read_config(dev, 0x8a, 1) & 0xcf)|0x10,1);
	    if ((pci_read_config(dev, 0x8a, 1) & 0x30) != 0x10)
		device_printf(dev, "%s could not set ATA133 clock\n",
			      ctlr->chip->text);
	}

	/* enable interrupt as BIOS might not */
	pci_write_config(dev, 0x8a, (pci_read_config(dev, 0x8a, 1) & 0x3f), 1);

	if (ctlr->chip->cfg2 & SII4CH) {
	    ATA_OUTL(ctlr->r_res2, 0x0200, 0x00000002);
	    ctlr->channels = 4;
	}

	ctlr->allocate = ata_sii_allocate;
	if (ctlr->chip->max_dma >= ATA_SA150) {
	    ctlr->reset = ata_sii_reset;
	    ctlr->setmode = ata_sata_setmode;
	}
	else
	    ctlr->setmode = ata_sii_setmode;
    }
    else {
	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ctlr->chip->cfg2 & SIIINTR ? 
			    ata_cmd_intr : ata_cmd_old_intr,
			    ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}

	if ((pci_read_config(dev, 0x51, 1) & 0x08) != 0x08) {
	    device_printf(dev, "HW has secondary channel disabled\n");
	    ctlr->channels = 1;
	}    

	/* enable interrupt as BIOS might not */
	pci_write_config(dev, 0x71, 0x01, 1);

	ctlr->setmode = ata_cmd_setmode;
    }
    return 0;
}

static int
ata_sii_allocate(device_t dev, struct ata_channel *ch)
{
    struct ata_pci_controller *ctlr = device_get_softc(device_get_parent(dev));
    int unit01 = (ch->unit & 1), unit10 = (ch->unit & 2);
    int i;

    for (i = ATA_DATA; i <= ATA_STATUS; i++) {
	ch->r_io[i].res = ctlr->r_res2;
	ch->r_io[i].offset = 0x80 + i + (unit01 << 6) + (unit10 << 8);
    }
    ch->r_io[ATA_ALTSTAT].res = ctlr->r_res2;
    ch->r_io[ATA_ALTSTAT].offset = 0x8a + (unit01 << 6) + (unit10 << 8);
    ch->r_io[ATA_BMCMD_PORT].res = ctlr->r_res2;
    ch->r_io[ATA_BMCMD_PORT].offset = 0x00 + (unit01 << 3) + (unit10 << 8);
    ch->r_io[ATA_BMSTAT_PORT].res = ctlr->r_res2;
    ch->r_io[ATA_BMSTAT_PORT].offset = 0x02 + (unit01 << 3) + (unit10 << 8);
    ch->r_io[ATA_BMDTP_PORT].res = ctlr->r_res2;
    ch->r_io[ATA_BMDTP_PORT].offset = 0x04 + (unit01 << 3) + (unit10 << 8);
    ch->r_io[ATA_BMDEVSPEC_0].res = ctlr->r_res2;
    ch->r_io[ATA_BMDEVSPEC_0].offset = 0xa1 + (unit01 << 6) + (unit10 << 8);
    ch->r_io[ATA_BMDEVSPEC_1].res = ctlr->r_res2;
    ch->r_io[ATA_BMDEVSPEC_1].offset = 0x100 + (unit01 << 7) + (unit10 << 8);
    ch->r_io[ATA_IDX_ADDR].res = ctlr->r_res2;

    if (ctlr->chip->max_dma >= ATA_SA150)
	ch->flags |= ATA_NO_SLAVE;

    if ((ctlr->chip->cfg2 & SIIBUG) && ch->dma) {
	ch->dma->boundary = 16 * DEV_BSIZE;
	ch->dma->max_iosize = 15 * DEV_BSIZE;
    }

    ata_generic_hw(ch);

    return 0;
}

static void
ata_sii_reset(struct ata_channel *ch)
{
    ATA_IDX_OUTL(ch, ATA_BMDEVSPEC_1, 0x00000001);
    DELAY(25000);
    ATA_IDX_OUTL(ch, ATA_BMDEVSPEC_1, 0x00000000);
    ata_udelay(1000000);
}

static void
ata_sii_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < ctlr->channels; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ATA_IDX_INB(ch, ATA_BMDEVSPEC_0) & 0x08) {
	    if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
		int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

		if (!(bmstat & ATA_BMSTAT_INTERRUPT))
		    continue;
		ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
		DELAY(1);
	    }
	    ctlr->interrupt[unit].function(ch);
	}
    }
}

static void
ata_cmd_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    u_int8_t reg71;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (((reg71 = pci_read_config(device_get_parent(ch->dev), 0x71, 1)) &
	     (ch->unit ? 0x08 : 0x04))) {
	    pci_write_config(device_get_parent(ch->dev), 0x71,
			     reg71 & ~(ch->unit ? 0x04 : 0x08), 1);
	    if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
		int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

		if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		    ATA_BMSTAT_INTERRUPT)
		    continue;
		ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
		DELAY(1);
	    }
	    ctlr->interrupt[unit].function(ch);
	}
    }
}

static void
ata_cmd_old_intr(void *data)
{
    struct ata_pci_controller *ctlr = data;
    struct ata_channel *ch;
    int unit;

    /* implement this as a toggle instead to balance load XXX */
    for (unit = 0; unit < 2; unit++) {
	if (!(ch = ctlr->interrupt[unit].argument))
	    continue;
	if (ch->dma && (ch->dma->flags & ATA_DMA_ACTIVE)) {
	    int bmstat = ATA_IDX_INB(ch, ATA_BMSTAT_PORT) & ATA_BMSTAT_MASK;

	    if ((bmstat & (ATA_BMSTAT_ACTIVE | ATA_BMSTAT_INTERRUPT)) !=
		ATA_BMSTAT_INTERRUPT)
		continue;
	    ATA_IDX_OUTB(ch, ATA_BMSTAT_PORT, bmstat & ~ATA_BMSTAT_ERROR);
	    DELAY(1);
	}
	ctlr->interrupt[unit].function(ch);
    }
}

static void
ata_sii_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int rego = (atadev->channel->unit << 4) + (ATA_DEV(atadev->unit) << 1);
    int mreg = atadev->channel->unit ? 0x84 : 0x80;
    int mask = 0x03 << (ATA_DEV(atadev->unit) << 2);
    int mval = pci_read_config(parent, mreg, 1) & ~mask;
    int error;

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if (ctlr->chip->cfg2 & SIISETCLK) {
	if (mode > ATA_UDMA2 && (pci_read_config(parent, 0x79, 1) &
				 (atadev->channel->unit ? 0x02 : 0x01))) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
    }
    else
	mode = ata_check_80pin(atadev, mode);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (error)
	return;

    if (mode >= ATA_UDMA0) {
	u_int8_t udmatimings[] = { 0xf, 0xb, 0x7, 0x5, 0x3, 0x2, 0x1 };
	u_int8_t ureg = 0xac + rego;

	pci_write_config(parent, mreg,
			 mval | (0x03 << (ATA_DEV(atadev->unit) << 2)), 1);
	pci_write_config(parent, ureg, 
			 (pci_read_config(parent, ureg, 1) & ~0x3f) |
			 udmatimings[mode & ATA_MODE_MASK], 1);

    }
    else if (mode >= ATA_WDMA0) {
	u_int8_t dreg = 0xa8 + rego;
	u_int16_t dmatimings[] = { 0x2208, 0x10c2, 0x10c1 };

	pci_write_config(parent, mreg,
			 mval | (0x02 << (ATA_DEV(atadev->unit) << 2)), 1);
	pci_write_config(parent, dreg, dmatimings[mode & ATA_MODE_MASK], 2);

    }
    else {
	u_int8_t preg = 0xa4 + rego;
	u_int16_t piotimings[] = { 0x328a, 0x2283, 0x1104, 0x10c3, 0x10c1 };

	pci_write_config(parent, mreg,
			 mval | (0x01 << (ATA_DEV(atadev->unit) << 2)), 1);
	pci_write_config(parent, preg, piotimings[mode & ATA_MODE_MASK], 2);
    }
    atadev->mode = mode;
}

static void
ata_cmd_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    mode = ata_check_80pin(atadev, mode);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (!error) {
	int treg = 0x54 + ((devno < 3) ? (devno << 1) : 7);
	int ureg = atadev->channel->unit ? 0x7b : 0x73;

	if (mode >= ATA_UDMA0) {	
	    int udmatimings[][2] = { { 0x31,  0xc2 }, { 0x21,  0x82 },
				     { 0x11,  0x42 }, { 0x25,  0x8a },
				     { 0x15,  0x4a }, { 0x05,  0x0a } };

	    u_int8_t umode = pci_read_config(parent, ureg, 1);

	    umode &= ~(atadev->unit == ATA_MASTER ? 0x35 : 0xca);
	    umode |= udmatimings[mode & ATA_MODE_MASK][ATA_DEV(atadev->unit)];
	    pci_write_config(parent, ureg, umode, 1);
	}
	else if (mode >= ATA_WDMA0) { 
	    int dmatimings[] = { 0x87, 0x32, 0x3f };

	    pci_write_config(parent, treg, dmatimings[mode & ATA_MODE_MASK], 1);
	    pci_write_config(parent, ureg, 
			     pci_read_config(parent, ureg, 1) &
			     ~(atadev->unit == ATA_MASTER ? 0x35 : 0xca), 1);
	}
	else {
	   int piotimings[] = { 0xa9, 0x57, 0x44, 0x32, 0x3f };
	    pci_write_config(parent, treg,
			     piotimings[(mode & ATA_MODE_MASK) - ATA_PIO0], 1);
	    pci_write_config(parent, ureg, 
			     pci_read_config(parent, ureg, 1) &
			     ~(atadev->unit == ATA_MASTER ? 0x35 : 0xca), 1);
	}
	atadev->mode = mode;
    }
}

/*
 * SiS chipset support functions
 */
int
ata_sis_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_SIS964_S,0x00, SISSATA,   0, ATA_SA150, "SiS 964" }, /* south */
     { ATA_SIS964,  0x00, SIS133NEW, 0, ATA_UDMA6, "SiS 964" }, /* south */
     { ATA_SIS963,  0x00, SIS133NEW, 0, ATA_UDMA6, "SiS 963" }, /* south */
     { ATA_SIS962,  0x00, SIS133NEW, 0, ATA_UDMA6, "SiS 962" }, /* south */

     { ATA_SIS745,  0x00, SIS100NEW, 0, ATA_UDMA5, "SiS 745" }, /* 1chip */
     { ATA_SIS735,  0x00, SIS100NEW, 0, ATA_UDMA5, "SiS 735" }, /* 1chip */
     { ATA_SIS733,  0x00, SIS100NEW, 0, ATA_UDMA5, "SiS 733" }, /* 1chip */
     { ATA_SIS730,  0x00, SIS100OLD, 0, ATA_UDMA5, "SiS 730" }, /* 1chip */

     { ATA_SIS635,  0x00, SIS100NEW, 0, ATA_UDMA5, "SiS 635" }, /* 1chip */
     { ATA_SIS633,  0x00, SIS100NEW, 0, ATA_UDMA5, "SiS 633" }, /* unknown */
     { ATA_SIS630,  0x30, SIS100OLD, 0, ATA_UDMA5, "SiS 630S"}, /* 1chip */
     { ATA_SIS630,  0x00, SIS66,     0, ATA_UDMA4, "SiS 630" }, /* 1chip */
     { ATA_SIS620,  0x00, SIS66,     0, ATA_UDMA4, "SiS 620" }, /* 1chip */

     { ATA_SIS550,  0x00, SIS66,     0, ATA_UDMA5, "SiS 550" },
     { ATA_SIS540,  0x00, SIS66,     0, ATA_UDMA4, "SiS 540" },
     { ATA_SIS530,  0x00, SIS66,     0, ATA_UDMA4, "SiS 530" },

     { ATA_SIS5513, 0xc2, SIS33,     1, ATA_UDMA2, "SiS 5513" },
     { ATA_SIS5513, 0x00, SIS33,     1, ATA_WDMA2, "SiS 5513" },
     { 0, 0, 0, 0, 0, 0 }};
    char buffer[64];
    int found = 0;

    if (!(idx = ata_find_chip(dev, ids, -pci_get_slot(dev)))) 
	return ENXIO;

    if (idx->cfg2 && !found) {
	u_int8_t reg57 = pci_read_config(dev, 0x57, 1);

	pci_write_config(dev, 0x57, (reg57 & 0x7f), 1);
	if (pci_read_config(dev, PCIR_DEVVENDOR, 4) == ATA_SIS5518) {
	    found = 1;
	    idx->cfg1 = SIS133NEW;
	    idx->max_dma = ATA_UDMA6;
	    sprintf(buffer, "SiS 962/963 %s controller",
		    ata_mode2str(idx->max_dma));
	}
	pci_write_config(dev, 0x57, reg57, 1);
    }
    if (idx->cfg2 && !found) {
	u_int8_t reg4a = pci_read_config(dev, 0x4a, 1);

	pci_write_config(dev, 0x4a, (reg4a | 0x10), 1);
	if (pci_read_config(dev, PCIR_DEVVENDOR, 4) == ATA_SIS5517) {
	    struct ata_chip_id id[] =
		{{ ATA_SISSOUTH, 0x10, 0, 0, 0, "" }, { 0, 0, 0, 0, 0, 0 }};

	    found = 1;
	    if (ata_find_chip(dev, id, pci_get_slot(dev))) {
		idx->cfg1 = SIS133OLD;
		idx->max_dma = ATA_UDMA6;
	    }
	    else {
		idx->cfg1 = SIS100NEW;
		idx->max_dma = ATA_UDMA5;
	    }
	    sprintf(buffer, "SiS 961 %s controller",ata_mode2str(idx->max_dma));
	}
	pci_write_config(dev, 0x4a, reg4a, 1);
    }
    if (!found)
	sprintf(buffer,"%s %s controller",idx->text,ata_mode2str(idx->max_dma));

    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_sis_chipinit;
    return 0;
}

static int
ata_sis_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;
    
    switch (ctlr->chip->cfg1) {
    case SIS33:
	break;
    case SIS66:
    case SIS100OLD:
	pci_write_config(dev, 0x52, pci_read_config(dev, 0x52, 1) & ~0x04, 1);
	break;
    case SIS100NEW:
    case SIS133OLD:
	pci_write_config(dev, 0x49, pci_read_config(dev, 0x49, 1) & ~0x01, 1);
	break;
    case SIS133NEW:
	pci_write_config(dev, 0x50, pci_read_config(dev, 0x50, 2) | 0x0008, 2);
	pci_write_config(dev, 0x52, pci_read_config(dev, 0x52, 2) | 0x0008, 2);
	break;
    case SISSATA:
	pci_write_config(dev, PCIR_COMMAND,
			 pci_read_config(dev, PCIR_COMMAND, 2) & ~0x0400, 2);
	ctlr->setmode = ata_sata_setmode;
	return 0;
    default:
	return ENXIO;
    }
    ctlr->setmode = ata_sis_setmode;
    return 0;
}

static void
ata_sis_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int error;

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if (ctlr->chip->cfg1 == SIS133NEW) {
	if (mode > ATA_UDMA2 &&
	    pci_read_config(parent, atadev->channel->unit?0x52:0x50,2)&0x8000){
		ata_prtdev(atadev,
		    "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
    }
    else {
	if (mode > ATA_UDMA2 &&
	    pci_read_config(parent, 0x48, 1)&(atadev->channel->unit?0x20:0x10)){
		ata_prtdev(atadev,
		    "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
    }

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "",
		   ata_mode2str(mode), ctlr->chip->text);
    if (!error) {
	switch (ctlr->chip->cfg1) {
	case SIS133NEW: {
	    u_int32_t timings[] = 
		{ 0x28269008, 0x0c266008, 0x04263008, 0x0c0a3008, 0x05093008,
		  0x22196008, 0x0c0a3008, 0x05093008, 0x050939fc, 0x050936ac,
		  0x0509347c, 0x0509325c, 0x0509323c, 0x0509322c, 0x0509321c};
	    u_int32_t reg;

	    reg = (pci_read_config(parent, 0x57, 1)&0x40?0x70:0x40)+(devno<<2);
	    pci_write_config(parent, reg, timings[ata_mode2idx(mode)], 4);
	    break;
	    }
	case SIS133OLD: {
	    u_int16_t timings[] =
	     { 0x00cb, 0x0067, 0x0044, 0x0033, 0x0031, 0x0044, 0x0033, 0x0031,
	       0x8f31, 0x8a31, 0x8731, 0x8531, 0x8331, 0x8231, 0x8131 };
		  
	    u_int16_t reg = 0x40 + (devno << 1);

	    pci_write_config(parent, reg, timings[ata_mode2idx(mode)], 2);
	    break;
	    }
	case SIS100NEW: {
	    u_int16_t timings[] =
		{ 0x00cb, 0x0067, 0x0044, 0x0033, 0x0031, 0x0044, 0x0033,
		  0x0031, 0x8b31, 0x8731, 0x8531, 0x8431, 0x8231, 0x8131 };
	    u_int16_t reg = 0x40 + (devno << 1);

	    pci_write_config(parent, reg, timings[ata_mode2idx(mode)], 2);
	    break;
	    }
	case SIS100OLD:
	case SIS66:
	case SIS33: {
	    u_int16_t timings[] =
		{ 0x0c0b, 0x0607, 0x0404, 0x0303, 0x0301, 0x0404, 0x0303,
		  0x0301, 0xf301, 0xd301, 0xb301, 0xa301, 0x9301, 0x8301 };
	    u_int16_t reg = 0x40 + (devno << 1);

	    pci_write_config(parent, reg, timings[ata_mode2idx(mode)], 2);
	    break;
	    }
	}
	atadev->mode = mode;
    }
}

/* VIA chipsets */
int
ata_via_ident(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    struct ata_chip_id *idx;
    static struct ata_chip_id ids[] =
    {{ ATA_VIA82C586, 0x02, VIA33,  0x00,   ATA_UDMA2, "VIA 82C586B" },
     { ATA_VIA82C586, 0x00, VIA33,  0x00,   ATA_WDMA2, "VIA 82C586" },
     { ATA_VIA82C596, 0x12, VIA66,  VIACLK, ATA_UDMA4, "VIA 82C596B" },
     { ATA_VIA82C596, 0x00, VIA33,  0x00,   ATA_UDMA2, "VIA 82C596" },
     { ATA_VIA82C686, 0x40, VIA100, VIABUG, ATA_UDMA5, "VIA 82C686B"},
     { ATA_VIA82C686, 0x10, VIA66,  VIACLK, ATA_UDMA4, "VIA 82C686A" },
     { ATA_VIA82C686, 0x00, VIA33,  0x00,   ATA_UDMA2, "VIA 82C686" },
     { ATA_VIA8231,   0x00, VIA100, VIABUG, ATA_UDMA5, "VIA 8231" },
     { ATA_VIA8233,   0x00, VIA100, 0x00,   ATA_UDMA5, "VIA 8233" },
     { ATA_VIA8233C,  0x00, VIA100, 0x00,   ATA_UDMA5, "VIA 8233C" },
     { ATA_VIA8233A,  0x00, VIA133, 0x00,   ATA_UDMA6, "VIA 8233A" },
     { ATA_VIA8235,   0x00, VIA133, 0x00,   ATA_UDMA6, "VIA 8235" },
     { ATA_VIA8237,   0x00, VIA133, 0x00,   ATA_UDMA6, "VIA 8237" },
     { 0, 0, 0, 0, 0, 0 }};
    static struct ata_chip_id new_ids[] =
    {{ ATA_VIA6410, 0x00, 0x00,	  0x00,	  ATA_UDMA6, "VIA 6410" },
     { ATA_VIA6420, 0x00, 0x00,	  0x00,	  ATA_SA150, "VIA 6420" },
     { 0, 0, 0, 0, 0, 0 }};
    char buffer[64];

    if (pci_get_devid(dev) == ATA_VIA82C571) {
	if (!(idx = ata_find_chip(dev, ids, -99))) 
	    return ENXIO;
    }
    else {
	if (!(idx = ata_match_chip(dev, new_ids))) 
	    return ENXIO;
    }

    sprintf(buffer, "%s %s controller", idx->text, ata_mode2str(idx->max_dma));
    device_set_desc_copy(dev, buffer);
    ctlr->chip = idx;
    ctlr->chipinit = ata_via_chipinit;
    return 0;
}

static int
ata_via_chipinit(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);

    if (ata_setup_interrupt(dev))
	return ENXIO;
    
    if (ctlr->chip->max_dma >= ATA_SA150) {
	pci_write_config(dev, PCIR_COMMAND,
			 pci_read_config(dev, PCIR_COMMAND, 2) & ~0x0400, 2);
	ctlr->setmode = ata_sata_setmode;
	return 0;
    }

    /* prepare for ATA-66 on the 82C686a and 82C596b */
    if (ctlr->chip->cfg2 & VIACLK)
	pci_write_config(dev, 0x50, 0x030b030b, 4);	  

    /* the southbridge might need the data corruption fix */
    if (ctlr->chip->cfg2 & VIABUG)
	ata_via_southbridge_fixup(dev);

    /* set fifo configuration half'n'half */
    pci_write_config(dev, 0x43, 
		     (pci_read_config(dev, 0x43, 1) & 0x90) | 0x2a, 1);

    /* set status register read retry */
    pci_write_config(dev, 0x44, pci_read_config(dev, 0x44, 1) | 0x08, 1);

    /* set DMA read & end-of-sector fifo flush */
    pci_write_config(dev, 0x46, 
		     (pci_read_config(dev, 0x46, 1) & 0x0c) | 0xf0, 1);

    /* set sector size */
    pci_write_config(dev, 0x60, DEV_BSIZE, 2);
    pci_write_config(dev, 0x68, DEV_BSIZE, 2);

    ctlr->setmode = ata_via_family_setmode;
    return 0;
}

static void
ata_via_southbridge_fixup(device_t dev)
{
    device_t *children;
    int nchildren, i;

    if (device_get_children(device_get_parent(dev), &children, &nchildren))
	return;

    for (i = 0; i < nchildren; i++) {
	if (pci_get_devid(children[i]) == ATA_VIA8363 ||
	    pci_get_devid(children[i]) == ATA_VIA8371 ||
	    pci_get_devid(children[i]) == ATA_VIA8662 ||
	    pci_get_devid(children[i]) == ATA_VIA8361) {
	    u_int8_t reg76 = pci_read_config(children[i], 0x76, 1);

	    if ((reg76 & 0xf0) != 0xd0) {
		device_printf(dev,
		"Correcting VIA config for southbridge data corruption bug\n");
		pci_write_config(children[i], 0x75, 0x80, 1);
		pci_write_config(children[i], 0x76, (reg76 & 0x0f) | 0xd0, 1);
	    }
	    break;
	}
    }
    free(children, M_TEMP);
}

/* common code for VIA, AMD & nVidia */
static void
ata_via_family_setmode(struct ata_device *atadev, int mode)
{
    device_t parent = device_get_parent(atadev->channel->dev);
    struct ata_pci_controller *ctlr = device_get_softc(parent);
    u_int8_t timings[] = { 0xa8, 0x65, 0x42, 0x22, 0x20, 0x42, 0x22, 0x20,
			   0x20, 0x20, 0x20, 0x20, 0x20, 0x20 };
    int modes[][7] = {
	{ 0xc2, 0xc1, 0xc0, 0x00, 0x00, 0x00, 0x00 },	/* VIA ATA33 */
	{ 0xee, 0xec, 0xea, 0xe9, 0xe8, 0x00, 0x00 },	/* VIA ATA66 */
	{ 0xf7, 0xf6, 0xf4, 0xf2, 0xf1, 0xf0, 0x00 },	/* VIA ATA100 */
	{ 0xf7, 0xf7, 0xf6, 0xf4, 0xf2, 0xf1, 0xf0 },	/* VIA ATA133 */
	{ 0xc2, 0xc1, 0xc0, 0xc4, 0xc5, 0xc6, 0xc7 }};	/* AMD/nVIDIA */
    int devno = (atadev->channel->unit << 1) + ATA_DEV(atadev->unit);
    int reg = 0x53 - devno;
    int error;

    mode = ata_limit_mode(atadev, mode, ctlr->chip->max_dma);

    if (ctlr->chip->cfg2 & AMDCABLE) {
	if (mode > ATA_UDMA2 &&
	    !(pci_read_config(parent, 0x42, 1) & (1 << devno))) {
	    ata_prtdev(atadev,
		       "DMA limited to UDMA33, non-ATA66 cable or device\n");
	    mode = ATA_UDMA2;
	}
    }
    else 
	mode = ata_check_80pin(atadev, mode);

    if (ctlr->chip->cfg2 & NVIDIA)
	reg += 0x10;

    if (ctlr->chip->cfg1 != VIA133)
	pci_write_config(parent, reg - 0x08, timings[ata_mode2idx(mode)], 1);

    error = ata_controlcmd(atadev, ATA_SETFEATURES, ATA_SF_SETXFER, 0, mode);

    if (bootverbose)
	ata_prtdev(atadev, "%ssetting %s on %s chip\n",
		   (error) ? "FAILURE " : "", ata_mode2str(mode),
		   ctlr->chip->text);
    if (!error) {
	if (mode >= ATA_UDMA0)
	    pci_write_config(parent, reg,
			     modes[ctlr->chip->cfg1][mode & ATA_MODE_MASK], 1);
	else
	    pci_write_config(parent, reg, 0x8b, 1);
	atadev->mode = mode;
    }
}

/* misc functions */
static struct ata_chip_id *
ata_find_chip(device_t dev, struct ata_chip_id *index, int slot)
{
    device_t *children;
    int nchildren, i;

    if (device_get_children(device_get_parent(dev), &children, &nchildren))
	return 0;

    while (index->chipid != 0) {
	for (i = 0; i < nchildren; i++) {
	    if (((slot >= 0 && pci_get_slot(children[i]) == slot) || 
		 (slot < 0 && pci_get_slot(children[i]) <= -slot)) &&
		pci_get_devid(children[i]) == index->chipid &&
		pci_get_revid(children[i]) >= index->chiprev) {
		free(children, M_TEMP);
		return index;
	    }
	}
	index++;
    }
    free(children, M_TEMP);
    return NULL;
}

static struct ata_chip_id *
ata_match_chip(device_t dev, struct ata_chip_id *index)
{
    while (index->chipid != 0) {
	if (pci_get_devid(dev) == index->chipid &&
	    pci_get_revid(dev) >= index->chiprev)
	    return index;
	index++;
    }
    return NULL;
}

static int
ata_setup_interrupt(device_t dev)
{
    struct ata_pci_controller *ctlr = device_get_softc(dev);
    int rid = ATA_IRQ_RID;

    if (!ata_legacy(dev)) {
	if (!(ctlr->r_irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid,
						   RF_SHAREABLE | RF_ACTIVE))) {
	    device_printf(dev, "unable to map interrupt\n");
	    return ENXIO;
	}
	if ((bus_setup_intr(dev, ctlr->r_irq, ATA_INTR_FLAGS,
			    ata_generic_intr, ctlr, &ctlr->handle))) {
	    device_printf(dev, "unable to setup interrupt\n");
	    return ENXIO;
	}
    }
    return 0;
}

struct ata_serialize {
    struct mtx	locked_mtx;
    int		locked_ch;
    int		restart_ch;
};

static int
ata_serialize(struct ata_channel *ch, int flags)
{
    struct ata_pci_controller *ctlr =
	device_get_softc(device_get_parent(ch->dev));
    struct ata_serialize *serial;
    static int inited = 0;
    int res;

    if (!inited) {
	ctlr->driver = malloc(sizeof(struct ata_serialize),
			      M_TEMP, M_NOWAIT | M_ZERO);
	serial = ctlr->driver;
	mtx_init(&serial->locked_mtx, "ATA serialize lock", NULL, MTX_DEF); 
	serial->locked_ch = -1;
	serial->restart_ch = -1;
	inited = 1;
    }
    else
	serial = ctlr->driver;

    mtx_lock(&serial->locked_mtx);
    switch (flags) {
    case ATA_LF_LOCK:
	if (serial->locked_ch == -1)
	    serial->locked_ch = ch->unit;
	if (serial->locked_ch != ch->unit)
	    serial->restart_ch = ch->unit;
	break;

    case ATA_LF_UNLOCK:
        if (serial->locked_ch == ch->unit) {
            serial->locked_ch = -1;
            if (serial->restart_ch != -1) {
                if (ctlr->interrupt[serial->restart_ch].argument) {
                    mtx_unlock(&serial->locked_mtx);
                    ata_start(ctlr->interrupt[serial->restart_ch].argument);
                    mtx_lock(&serial->locked_mtx);
                }
                serial->restart_ch = -1;
            }
        }
	break;

    case ATA_LF_WHICH:
	break;
    }
    res = serial->locked_ch;
    mtx_unlock(&serial->locked_mtx);
    return res;
}

static int
ata_check_80pin(struct ata_device *atadev, int mode)
{
    if (mode > ATA_UDMA2 && !(atadev->param->hwres & ATA_CABLE_ID)) {
	ata_prtdev(atadev,"DMA limited to UDMA33, non-ATA66 cable or device\n");
	mode = ATA_UDMA2;
    }
    return mode;
}

static int
ata_mode2idx(int mode)
{
    if ((mode & ATA_DMA_MASK) == ATA_UDMA0)
	 return (mode & ATA_MODE_MASK) + 8;
    if ((mode & ATA_DMA_MASK) == ATA_WDMA0)
	 return (mode & ATA_MODE_MASK) + 5;
    return (mode & ATA_MODE_MASK) - ATA_PIO0;
}
