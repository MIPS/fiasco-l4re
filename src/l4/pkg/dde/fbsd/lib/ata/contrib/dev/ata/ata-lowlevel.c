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
__FBSDID("$FreeBSD: src/sys/dev/ata/ata-lowlevel.c,v 1.44.2.5 2005/03/24 18:44:27 mdodd Exp $");

#include "opt_ata.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ata.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/sema.h>
#include <sys/taskqueue.h>
#include <vm/uma.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <dev/ata/ata-all.h>

/* prototypes */
static int ata_begin_transaction(struct ata_request *);
static int ata_end_transaction(struct ata_request *);
static void ata_generic_reset(struct ata_channel *);
static int ata_wait(struct ata_device *, u_int8_t);
static void ata_pio_read(struct ata_request *, int);
static void ata_pio_write(struct ata_request *, int);

/* local vars */
static int atadebug = 0;

/*
 * low level ATA functions 
 */
void
ata_generic_hw(struct ata_channel *ch)
{
    ch->hw.begin_transaction = ata_begin_transaction;
    ch->hw.end_transaction = ata_end_transaction;
    ch->hw.reset = ata_generic_reset;
    ch->hw.command = ata_generic_command;
}

/* must be called with ATA channel locked */
static int
ata_begin_transaction(struct ata_request *request)
{
    struct ata_channel *ch = request->device->channel;

    /* safetybelt for HW that went away */
    if (!request->device->param || request->device->channel->flags&ATA_HWGONE) {
	request->retries = 0;
	request->result = ENXIO;
	return ATA_OP_FINISHED;
    }

    ATA_DEBUG_RQ(request, "begin transaction");

    /* disable ATAPI DMA writes if HW doesn't support it */
    if ((ch->flags & ATA_ATAPI_DMA_RO) &&
	((request->flags & (ATA_R_ATAPI | ATA_R_DMA | ATA_R_WRITE)) ==
	 (ATA_R_ATAPI | ATA_R_DMA | ATA_R_WRITE)))
	request->flags &= ~ATA_R_DMA;

    switch (request->flags & (ATA_R_ATAPI | ATA_R_DMA)) {

    /* ATA PIO data transfer and control commands */
    default:
	{
	/* record command direction here as our request might be gone later */
	int write = (request->flags & ATA_R_WRITE);

	    /* issue command */
	    if (ch->hw.command(request->device, request->u.ata.command,
			       request->u.ata.lba, request->u.ata.count,
			       request->u.ata.feature)) {
		ata_prtdev(request->device, "error issueing %s command\n",
			   ata_cmd2str(request));
		request->result = EIO;
		break;
	    }

	    /* device reset doesn't interrupt */
	    if (request->u.ata.command == ATA_ATAPI_RESET) {
		int timeout = 1000000;
		do {
		    DELAY(10);
		    request->status = ATA_IDX_INB(ch, ATA_STATUS);
		} while (request->status & ATA_S_BUSY && timeout--);
		if (request->status & ATA_S_ERROR)
		    request->error = ATA_IDX_INB(ch, ATA_ERROR);
		break;
	    }

	    /* if write command output the data */
	    if (write) {
		if (ata_wait(request->device,
			     (ATA_S_READY | ATA_S_DSC | ATA_S_DRQ)) < 0) {
		    ata_prtdev(request->device,"timeout waiting for write DRQ");
		    request->result = EIO;
		    break;
		}
		ata_pio_write(request, request->transfersize);
	    }
	}
	return ATA_OP_CONTINUES;

    /* ATA DMA data transfer commands */
    case ATA_R_DMA:
	/* check sanity, setup SG list and DMA engine */
	if (ch->dma->load(request->device, request->data, request->bytecount,
			  request->flags & ATA_R_READ)) {
	    ata_prtdev(request->device, "setting up DMA failed\n");
	    request->result = EIO;
	    break;
	}

	/* issue command */
	if (ch->hw.command(request->device, request->u.ata.command,
			   request->u.ata.lba, request->u.ata.count,
			   request->u.ata.feature)) {
	    ata_prtdev(request->device, "error issueing %s command\n",
		       ata_cmd2str(request));
	    request->result = EIO;
	    break;
	}

	/* start DMA engine */
	if (ch->dma->start(ch)) {
	    ata_prtdev(request->device, "error starting DMA\n");
	    request->result = EIO;
	    break;
	}
	return ATA_OP_CONTINUES;

    /* ATAPI PIO commands */
    case ATA_R_ATAPI:
	/* is this just a POLL DSC command ? */
	if (request->u.atapi.ccb[0] == ATAPI_POLL_DSC) {
	    ATA_IDX_OUTB(ch, ATA_DRIVE,
			 ATA_D_IBM | request->device->unit);
	    DELAY(10);
	    if (!(ATA_IDX_INB(ch, ATA_ALTSTAT)&ATA_S_DSC))
		request->result = EBUSY;
	    break;
	}

	/* start ATAPI operation */
	if (ch->hw.command(request->device, ATA_PACKET_CMD,
			   request->transfersize << 8, 0, 0)) {
	    ata_prtdev(request->device, "error issuing ATA PACKET command\n");
	    request->result = EIO;
	    break;
	}

	/* command interrupt device ? just return and wait for interrupt */
	if ((request->device->param->config & ATA_DRQ_MASK) == ATA_DRQ_INTR)
	    return ATA_OP_CONTINUES;

	/* wait for ready to write ATAPI command block */
	{
	    int timeout = 5000; /* might be less for fast devices */
	    while (timeout--) {
		int reason = ATA_IDX_INB(ch, ATA_IREASON);
		int status = ATA_IDX_INB(ch, ATA_STATUS);

		if (((reason & (ATA_I_CMD | ATA_I_IN)) |
		     (status & (ATA_S_DRQ | ATA_S_BUSY))) == ATAPI_P_CMDOUT)
		    break;
		DELAY(20);
	    }
	    if (timeout <= 0) {
		ata_prtdev(request->device,
			   "timeout waiting for ATAPI ready\n");
		request->result = EIO;
		break;
	    }
	}

	/* this seems to be needed for some (slow) devices */
	DELAY(10);

	/* output actual command block */
	ATA_IDX_OUTSW_STRM(ch, ATA_DATA, 
			   (int16_t *)request->u.atapi.ccb,
			   (request->device->param->config & ATA_PROTO_MASK) ==
			   ATA_PROTO_ATAPI_12 ? 6 : 8);
	return ATA_OP_CONTINUES;

    case ATA_R_ATAPI|ATA_R_DMA:
	/* is this just a POLL DSC command ? */
	if (request->u.atapi.ccb[0] == ATAPI_POLL_DSC) {
	    ATA_IDX_OUTB(ch, ATA_DRIVE,
			 ATA_D_IBM | request->device->unit);
	    DELAY(10);
	    if (!(ATA_IDX_INB(ch, ATA_ALTSTAT)&ATA_S_DSC))
		request->result = EBUSY;
	    break;
	}

	/* check sanity, setup SG list and DMA engine */
	if (ch->dma->load(request->device,
						request->data,
						request->bytecount,
						request->flags & ATA_R_READ)) {
	    ata_prtdev(request->device, "setting up DMA failed\n");
	    request->result = EIO;
	    break;
	}

	/* start ATAPI operation */
	if (ch->hw.command(request->device, ATA_PACKET_CMD, 0, 0, ATA_F_DMA)) {
	    ata_prtdev(request->device, "error issuing ATAPI packet command\n");
	    request->result = EIO;
	    break;
	}

	/* wait for ready to write ATAPI command block */
	{
	    int timeout = 5000; /* might be less for fast devices */
	    while (timeout--) {
		int reason = ATA_IDX_INB(ch, ATA_IREASON);
		int status = ATA_IDX_INB(ch, ATA_STATUS);

		if (((reason & (ATA_I_CMD | ATA_I_IN)) |
		     (status & (ATA_S_DRQ | ATA_S_BUSY))) == ATAPI_P_CMDOUT)
		    break;
		DELAY(20);
	    }
	    if (timeout <= 0) {
		ata_prtdev(request->device,"timeout waiting for ATAPI ready\n");
		request->result = EIO;
		break;
	    }
	}

	/* this seems to be needed for some (slow) devices */
	DELAY(10);

	/* output actual command block */
	ATA_IDX_OUTSW_STRM(ch, ATA_DATA, 
			   (int16_t *)request->u.atapi.ccb,
			   (request->device->param->config & ATA_PROTO_MASK) ==
			   ATA_PROTO_ATAPI_12 ? 6 : 8);

	/* start DMA engine */
	if (ch->dma->start(ch)) {
	    request->result = EIO;
	    break;
	}
	return ATA_OP_CONTINUES;
    }

    /* request finish here */
    if (ch->dma && ch->dma->flags & ATA_DMA_LOADED)
	ch->dma->unload(ch);
    return ATA_OP_FINISHED;
}

static int
ata_end_transaction(struct ata_request *request)
{
    struct ata_channel *ch = request->device->channel;
    int length;

    ATA_DEBUG_RQ(request, "end transaction");

    /* clear interrupt and get status */
    request->status = ATA_IDX_INB(ch, ATA_STATUS);

    switch (request->flags & (ATA_R_ATAPI | ATA_R_DMA | ATA_R_CONTROL)) {

    /* ATA PIO data transfer and control commands */
    default:
	/* XXX Doesn't handle the non-PIO case. */
	if (request->flags & ATA_R_TIMEOUT)
	    return ATA_OP_FINISHED;

	/* on control commands read back registers to the request struct */
	if (request->flags & ATA_R_CONTROL) {
	    request->u.ata.count = ATA_IDX_INB(ch, ATA_COUNT);
	    request->u.ata.lba = ATA_IDX_INB(ch, ATA_SECTOR) |
				 (ATA_IDX_INB(ch, ATA_CYL_LSB) << 8) |
				 (ATA_IDX_INB(ch, ATA_CYL_MSB) << 16) |
	    			 ((ATA_IDX_INB(ch, ATA_DRIVE) & 0x0f) << 24);
	}

	/* if we got an error we are done with the HW */
	if (request->status & ATA_S_ERROR) {
	    request->error = ATA_IDX_INB(ch, ATA_ERROR);
	    return ATA_OP_FINISHED;
	}
	
	/* are we moving data ? */
	if (request->flags & (ATA_R_READ | ATA_R_WRITE)) {

	    /* if read data get it */
	    if (request->flags & ATA_R_READ)
		ata_pio_read(request, request->transfersize);

	    /* update how far we've gotten */
	    request->donecount += request->transfersize;

	    /* do we need a scoop more ? */
	    if (request->bytecount > request->donecount) {

		/* set this transfer size according to HW capabilities */
		request->transfersize = 
		    min((request->bytecount - request->donecount),
			request->transfersize);

		/* clear interrupt seen flag as we need to wait again */
		request->flags &= ~ATA_R_INTR_SEEN;

		/* if data write command, output the data */
		if (request->flags & ATA_R_WRITE) {

		    /* if we get an error here we are done with the HW */
		    if (ata_wait(request->device,
				 (ATA_S_READY | ATA_S_DSC | ATA_S_DRQ)) < 0) {
			ata_prtdev(request->device,
				   "timeout waiting for write DRQ");
			request->status = ATA_IDX_INB(ch, ATA_STATUS);
			return ATA_OP_FINISHED;
		    }

		    /* output data and return waiting for new interrupt */
		    ata_pio_write(request, request->transfersize);
		    return ATA_OP_CONTINUES;
		}

		/* if data read command, return & wait for interrupt */
		if (request->flags & ATA_R_READ)
		    return ATA_OP_CONTINUES;
	    }
	}
	/* done with HW */
	return ATA_OP_FINISHED;

    /* ATA DMA data transfer commands */
    case ATA_R_DMA:

	/* stop DMA engine and get status */
	if (ch->dma->stop)
	    request->dmastat = ch->dma->stop(ch);

	/* did we get error or data */
	if (request->status & ATA_S_ERROR)
	    request->error = ATA_IDX_INB(ch, ATA_ERROR);
	else if (request->dmastat & ATA_BMSTAT_ERROR)
	    request->status |= ATA_S_ERROR;
	else
	    request->donecount = request->bytecount;

	/* release SG list etc */
	ch->dma->unload(ch);

	/* done with HW */
	return ATA_OP_FINISHED;

    /* ATAPI PIO commands */
    case ATA_R_ATAPI:
	length = ATA_IDX_INB(ch, ATA_CYL_LSB)|(ATA_IDX_INB(ch, ATA_CYL_MSB)<<8);

	switch ((ATA_IDX_INB(ch, ATA_IREASON) & (ATA_I_CMD | ATA_I_IN)) |
		(request->status & ATA_S_DRQ)) {

	case ATAPI_P_CMDOUT:
	    /* this seems to be needed for some (slow) devices */
	    DELAY(10);

	    if (!(request->status & ATA_S_DRQ)) {
		ata_prtdev(request->device, "command interrupt without DRQ\n");
		request->status = ATA_S_ERROR;
		return ATA_OP_FINISHED;
	    }
	    ATA_IDX_OUTSW_STRM(ch, ATA_DATA, (int16_t *)request->u.atapi.ccb,
			       (request->device->param->config &
				ATA_PROTO_MASK)== ATA_PROTO_ATAPI_12 ? 6 : 8);
	    /* return wait for interrupt */
	    return ATA_OP_CONTINUES;

	case ATAPI_P_WRITE:
	    if (request->flags & ATA_R_READ) {
		request->status = ATA_S_ERROR;
		ata_prtdev(request->device,
			   "%s trying to write on read buffer\n",
			   ata_cmd2str(request));
		return ATA_OP_FINISHED;
	    }
	    ata_pio_write(request, length);
	    request->donecount += length;

	    /* set next transfer size according to HW capabilities */
	    request->transfersize = min((request->bytecount-request->donecount),
					request->transfersize);
	    /* return wait for interrupt */
	    return ATA_OP_CONTINUES;

	case ATAPI_P_READ:
	    if (request->flags & ATA_R_WRITE) {
		request->status = ATA_S_ERROR;
		ata_prtdev(request->device,
			   "%s trying to read on write buffer\n",
			   ata_cmd2str(request));
		return ATA_OP_FINISHED;
	    }
	    ata_pio_read(request, length);
	    request->donecount += length;

	    /* set next transfer size according to HW capabilities */
	    request->transfersize = min((request->bytecount-request->donecount),
					request->transfersize);
	    /* return wait for interrupt */
	    return ATA_OP_CONTINUES;

	case ATAPI_P_DONEDRQ:
	    ata_prtdev(request->device,
		       "WARNING - %s DONEDRQ non conformant device\n",
		       ata_cmd2str(request));
	    if (request->flags & ATA_R_READ) {
		ata_pio_read(request, length);
		request->donecount += length;
	    }
	    else if (request->flags & ATA_R_WRITE) {
		ata_pio_write(request, length);
		request->donecount += length;
	    }
	    else
		request->status = ATA_S_ERROR;
	    /* FALLTHROUGH */

	case ATAPI_P_ABORT:
	case ATAPI_P_DONE:
	    if (request->status & (ATA_S_ERROR | ATA_S_DWF))
		request->error = ATA_IDX_INB(ch, ATA_ERROR);
	    return ATA_OP_FINISHED;

	default:
	    ata_prtdev(request->device, "unknown transfer phase\n");
	    request->status = ATA_S_ERROR;
	}

	/* done with HW */
	return ATA_OP_FINISHED;

    /* ATAPI DMA commands */
    case ATA_R_ATAPI|ATA_R_DMA:

	/* stop the engine and get engine status */
	if (ch->dma->stop)
	    request->dmastat = ch->dma->stop(ch);

	/* did we get error or data */
	if (request->status & (ATA_S_ERROR | ATA_S_DWF))
	    request->error = ATA_IDX_INB(ch, ATA_ERROR);
	else if (request->dmastat & ATA_BMSTAT_ERROR)
	    request->status |= ATA_S_ERROR;
	else
	    request->donecount = request->bytecount;
 
	/* release SG list etc */
	ch->dma->unload(ch);

	/* done with HW */
	return ATA_OP_FINISHED;
    }
}

/* must be called with ATA channel locked */
static void
ata_generic_reset(struct ata_channel *ch)
{
    u_int8_t err = 0, lsb = 0, msb = 0, ostat0, ostat1;
    u_int8_t stat0 = 0, stat1 = 0;
    int mask = 0, timeout;

    /* if DMA functionality present stop it  */
    if (ch->dma) {
        if (ch->dma->stop)
            ch->dma->stop(ch);
        if (ch->dma->flags & ATA_DMA_LOADED)
            ch->dma->unload(ch);
    }

    /* reset host end of channel (if supported) */
    if (ch->reset)
	ch->reset(ch);

    /* do we have any signs of ATA/ATAPI HW being present ? */
    ATA_IDX_OUTB(ch, ATA_DRIVE, ATA_D_IBM | ATA_MASTER);
    DELAY(10);
    ostat0 = ATA_IDX_INB(ch, ATA_STATUS);
    if ((ostat0 & 0xf8) != 0xf8 && ostat0 != 0xa5) {
	stat0 = ATA_S_BUSY;
	mask |= 0x01;
    }

    ATA_IDX_OUTB(ch, ATA_DRIVE, ATA_D_IBM | ATA_SLAVE);
    DELAY(10);	
    ostat1 = ATA_IDX_INB(ch, ATA_STATUS);

    /* in some setups we dont want to test for a slave */
    if (!(ch->flags & ATA_NO_SLAVE)) {
	if ((ostat1 & 0xf8) != 0xf8 && ostat1 != 0xa5) {
	    stat1 = ATA_S_BUSY;
	    mask |= 0x02;
	}
    }

    if (bootverbose)
	ata_printf(ch, -1, "reset tp1 mask=%02x ostat0=%02x ostat1=%02x\n",
		   mask, ostat0, ostat1);

    /* if nothing showed up there is no need to get any further */
    /* SOS is that too strong?, we just might loose devices here XXX */
    ch->devices = 0;
    if (!mask)
	return;

    /* reset (both) devices on this channel */
    ATA_IDX_OUTB(ch, ATA_DRIVE, ATA_D_IBM | ATA_MASTER);
    DELAY(10);
    ATA_IDX_OUTB(ch, ATA_ALTSTAT, ATA_A_IDS | ATA_A_RESET);
    ata_udelay(10000); 
    ATA_IDX_OUTB(ch, ATA_ALTSTAT, ATA_A_IDS);
    ata_udelay(100000);
    ATA_IDX_INB(ch, ATA_ERROR);

    /* wait for BUSY to go inactive */
    for (timeout = 0; timeout < 310; timeout++) {
	if (stat0 & ATA_S_BUSY) {
	    ATA_IDX_OUTB(ch, ATA_DRIVE, ATA_D_IBM | ATA_MASTER);
	    DELAY(10);
    	    err = ATA_IDX_INB(ch, ATA_ERROR);
	    lsb = ATA_IDX_INB(ch, ATA_CYL_LSB);
	    msb = ATA_IDX_INB(ch, ATA_CYL_MSB);
	    stat0 = ATA_IDX_INB(ch, ATA_STATUS);
	    if (bootverbose)
		ata_printf(ch, ATA_MASTER,
			   "stat=0x%02x err=0x%02x lsb=0x%02x msb=0x%02x\n",
			   stat0, err, lsb, msb);
	    if (!(stat0 & ATA_S_BUSY)) {
		if ((err & 0x7f) == ATA_E_ILI) {
		    if (lsb == ATAPI_MAGIC_LSB && msb == ATAPI_MAGIC_MSB) {
			ch->devices |= ATA_ATAPI_MASTER;
		    }
		    else if (stat0 & ATA_S_READY) {
			ch->devices |= ATA_ATA_MASTER;
		    }
		}
		else if ((stat0 & 0x4f) && err == lsb && err == msb) {
		    stat0 |= ATA_S_BUSY;
		}
	    }
	}
	if (!((mask == 0x03) && (stat0 & ATA_S_BUSY)) && (stat1 & ATA_S_BUSY)) {
	    ATA_IDX_OUTB(ch, ATA_DRIVE, ATA_D_IBM | ATA_SLAVE);
	    DELAY(10);
    	    err = ATA_IDX_INB(ch, ATA_ERROR);
	    lsb = ATA_IDX_INB(ch, ATA_CYL_LSB);
	    msb = ATA_IDX_INB(ch, ATA_CYL_MSB);
	    stat1 = ATA_IDX_INB(ch, ATA_STATUS);
	    if (bootverbose)
		ata_printf(ch, ATA_SLAVE,
			   " stat=0x%02x err=0x%02x lsb=0x%02x msb=0x%02x\n",
			   stat1, err, lsb, msb);
	    if (!(stat1 & ATA_S_BUSY)) {
		if ((err & 0x7f) == ATA_E_ILI) {
		    if (lsb == ATAPI_MAGIC_LSB && msb == ATAPI_MAGIC_MSB) {
			ch->devices |= ATA_ATAPI_SLAVE;
		    }
		    else if (stat1 & ATA_S_READY) {
			ch->devices |= ATA_ATA_SLAVE;
		    }
		}
		else if ((stat1 & 0x4f) && err == lsb && err == msb) {
		    stat1 |= ATA_S_BUSY;
		}
	    }
	}
	if (mask == 0x01)	/* wait for master only */
	    if (!(stat0 & ATA_S_BUSY) || (stat0 == 0xff && timeout > 5) ||
		(stat0 == err && lsb == err && msb == err && timeout > 5))
		break;
	if (mask == 0x02)	/* wait for slave only */
	    if (!(stat1 & ATA_S_BUSY) || (stat1 == 0xff && timeout > 5) ||
		(stat1 == err && lsb == err && msb == err && timeout > 5))
		break;
	if (mask == 0x03) {	/* wait for both master & slave */
	    if (!(stat0 & ATA_S_BUSY) && !(stat1 & ATA_S_BUSY))
		break;
	    if ((stat0 == 0xff && timeout > 5) ||
		(stat0 == err && lsb == err && msb == err && timeout > 5))
		mask &= ~0x01;
	    if ((stat1 == 0xff && timeout > 5) ||
		(stat1 == err && lsb == err && msb == err && timeout > 5))
		mask &= ~0x02;
	}
	if (mask == 0 && !(stat0 & ATA_S_BUSY) && !(stat1 & ATA_S_BUSY))
	    break;

	ata_udelay(100000);
    }

    if (bootverbose)
	ata_printf(ch, -1,
		   "reset tp2 stat0=%02x stat1=%02x devices=0x%b\n",
		   stat0, stat1, ch->devices,
		   "\20\4ATAPI_SLAVE\3ATAPI_MASTER\2ATA_SLAVE\1ATA_MASTER");
}

static int
ata_wait(struct ata_device *atadev, u_int8_t mask)
{
    u_int8_t status;
    int timeout = 0;
    
    DELAY(1);

    /* wait 5 seconds for device to get !BUSY */
    while (timeout < 5000000) {
	status = ATA_IDX_INB(atadev->channel, ATA_STATUS);

	/* if drive fails status, reselect the drive just to be sure */
	if (status == 0xff) {
	    ata_prtdev(atadev, "WARNING no status, reselecting device\n");
	    ATA_IDX_OUTB(atadev->channel, ATA_DRIVE, ATA_D_IBM | atadev->unit);
	    DELAY(10);
	    status = ATA_IDX_INB(atadev->channel, ATA_STATUS);
	    if (status == 0xff)
		return -1;
	}

	/* are we done ? */
	if (!(status & ATA_S_BUSY))
	    break;	      

	if (timeout > 1000) {
	    timeout += 1000;
	    DELAY(1000);
	}
	else {
	    timeout += 10;
	    DELAY(10);
	}
    }	 
    if (timeout >= 5000000)	 
	return -1;	    
    if (!mask)	   
	return (status & ATA_S_ERROR);	 

    DELAY(1);
    
    /* wait 50 msec for bits wanted */	   
    timeout = 5000;
    while (timeout--) {	  
	status = ATA_IDX_INB(atadev->channel, ATA_STATUS);
	if ((status & mask) == mask) 
	    return (status & ATA_S_ERROR);	      
	DELAY (10);	   
    }	  
    return -1;	    
}   

int
ata_generic_command(struct ata_device *atadev, u_int8_t command,
	    	    u_int64_t lba, u_int16_t count, u_int16_t feature)
{
    if (atadebug)
	ata_prtdev(atadev, "ata_command: addr=%04lx, command=%02x, "
		   "lba=%jd, count=%d, feature=%d\n",
		   rman_get_start(atadev->channel->r_io[ATA_DATA].res), 
		   command, (intmax_t)lba, count, feature);

    /* select device */
    ATA_IDX_OUTB(atadev->channel, ATA_DRIVE, ATA_D_IBM | atadev->unit);

    /* ready to issue command ? */
    if (ata_wait(atadev, 0) < 0) { 
	ata_prtdev(atadev, "timeout sending command=%02x\n", command);
	return -1;
    }

    /* enable interrupt */
    ATA_IDX_OUTB(atadev->channel, ATA_ALTSTAT, ATA_A_4BIT);

    /* only use 48bit addressing if needed (avoid bugs and overhead) */
    if ((lba >= ATA_MAX_28BIT_LBA || count > 256) && atadev->param && 
	atadev->param->support.command2 & ATA_SUPPORT_ADDRESS48) {

	/* translate command into 48bit version */
	switch (command) {
	case ATA_READ:
	    command = ATA_READ48; break;
	case ATA_READ_MUL:
	    command = ATA_READ_MUL48; break;
	case ATA_READ_DMA:
	    command = ATA_READ_DMA48; break;
	case ATA_READ_DMA_QUEUED:
	    command = ATA_READ_DMA_QUEUED48; break;
	case ATA_WRITE:
	    command = ATA_WRITE48; break;
	case ATA_WRITE_MUL:
	    command = ATA_WRITE_MUL48; break;
	case ATA_WRITE_DMA:
	    command = ATA_WRITE_DMA48; break;
	case ATA_WRITE_DMA_QUEUED:
	    command = ATA_WRITE_DMA_QUEUED48; break;
	case ATA_FLUSHCACHE:
	    command = ATA_FLUSHCACHE48; break;
	default:
	    ata_prtdev(atadev, "can't translate cmd to 48bit version\n");
	    return -1;
	}
	ATA_IDX_OUTB(atadev->channel, ATA_FEATURE, (feature>>8) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_FEATURE, feature & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_COUNT, (count>>8) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_COUNT, count & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_SECTOR, (lba>>24) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_SECTOR, lba & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_LSB, (lba>>32) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_LSB, (lba>>8) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_MSB, (lba>>40) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_MSB, (lba>>16) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_DRIVE, ATA_D_LBA | atadev->unit);
	atadev->channel->flags |= ATA_48BIT_ACTIVE;
    }
    else {
	ATA_IDX_OUTB(atadev->channel, ATA_FEATURE, feature);
	ATA_IDX_OUTB(atadev->channel, ATA_COUNT, count);
	ATA_IDX_OUTB(atadev->channel, ATA_SECTOR, lba & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_LSB, (lba>>8) & 0xff);
	ATA_IDX_OUTB(atadev->channel, ATA_CYL_MSB, (lba>>16) & 0xff);
	if (atadev->flags & ATA_D_USE_CHS)
	    ATA_IDX_OUTB(atadev->channel, ATA_DRIVE,
			 ATA_D_IBM | atadev->unit | ((lba>>24) & 0xf));
	else
	    ATA_IDX_OUTB(atadev->channel, ATA_DRIVE,
			 ATA_D_IBM | ATA_D_LBA | atadev->unit|((lba>>24)&0xf));
	atadev->channel->flags &= ~ATA_48BIT_ACTIVE;
    }

    /* issue command to controller */
    ATA_IDX_OUTB(atadev->channel, ATA_CMD, command);

    return 0;
}

static void
ata_pio_read(struct ata_request *request, int length)
{
    int size = min(request->transfersize, length);
    struct ata_channel *ch = request->device->channel;
    int resid;

    if (ch->flags & ATA_USE_16BIT || (size % sizeof(int32_t)))
	ATA_IDX_INSW_STRM(ch, ATA_DATA,
			  (void*)((uintptr_t)request->data+request->donecount),
			  size / sizeof(int16_t));
    else
	ATA_IDX_INSL_STRM(ch, ATA_DATA,
			  (void*)((uintptr_t)request->data+request->donecount),
			  size / sizeof(int32_t));

    if (request->transfersize < length) {
	ata_prtdev(request->device, "WARNING - %s read data overrun %d>%d\n",
		   ata_cmd2str(request), length, request->transfersize);
	for (resid = request->transfersize; resid < length;
	     resid += sizeof(int16_t))
	    ATA_IDX_INW(ch, ATA_DATA);
    }
}

static void
ata_pio_write(struct ata_request *request, int length)
{
    int size = min(request->transfersize, length);
    struct ata_channel *ch = request->device->channel;
    int resid;

    if (ch->flags & ATA_USE_16BIT || (size % sizeof(int32_t)))
	ATA_IDX_OUTSW_STRM(ch, ATA_DATA,
			   (void*)((uintptr_t)request->data+request->donecount),
			   size / sizeof(int16_t));
    else
	ATA_IDX_OUTSL_STRM(ch, ATA_DATA,
			   (void*)((uintptr_t)request->data+request->donecount),
			   size / sizeof(int32_t));

    if (request->transfersize < length) {
	ata_prtdev(request->device, "WARNING - %s write data underrun %d>%d\n",
		   ata_cmd2str(request), length, request->transfersize);
	for (resid = request->transfersize; resid < length;
	     resid += sizeof(int16_t))
	    ATA_IDX_OUTW(ch, ATA_DATA, 0);
    }
}
