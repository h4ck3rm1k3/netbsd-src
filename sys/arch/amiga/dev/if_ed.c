/*	$NetBSD: if_ed.c,v 1.62 2010/04/05 07:19:29 joerg Exp $ */

/*
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 *
 * Currently supports the Hydra Systems ethernet card.
 */

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ed.c,v 1.62 2010/04/05 07:19:29 joerg Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>
#include <dev/ic/dp8390reg.h>
#include <amiga/dev/if_edreg.h>

#define HYDRA_MANID	2121
#define HYDRA_PRODID	1

#define ASDG_MANID	1023
#define ASDG_PRODID	254

/*
 * ed_softc: per line info and status
 */
struct ed_softc {
	struct	device sc_dev;
	struct	isr sc_isr;

	struct	ethercom sc_ethercom;	/* ethernet common */

	u_char	volatile *nic_addr;	/* NIC (DS8390) I/O address */

	u_char	cr_proto;	/* values always set in CR */

	void *	mem_start;	/* NIC memory start address */
	void *	mem_end;	/* NIC memory end address */
	u_long	mem_size;	/* total NIC memory size */
	void *	mem_ring;	/* start of RX ring-buffer (in NIC mem) */

	u_char	xmit_busy;	/* transmitter is busy */
	u_char	txb_cnt;	/* number of transmit buffers */
	u_char	txb_inuse;	/* number of TX buffers currently in-use*/

	u_char	txb_new;	/* pointer to where new buffer will be added */
	u_char	txb_next_tx;	/* pointer to next buffer ready to xmit */
	u_short	txb_len[8];	/* buffered xmit buffer lengths */
	u_char	tx_page_start;	/* first page of TX buffer area */
	u_char	rec_page_start;	/* first page of RX ring-buffer */
	u_char	rec_page_stop;	/* last page of RX ring-buffer */
	u_char	next_packet;	/* pointer to next unread RX packet */
};

int ed_zbus_match(struct device *, struct cfdata *, void *);
void ed_zbus_attach(struct device *, struct device *, void *);
int edintr(void *);
int ed_ioctl(struct ifnet *, u_long, void *);
void ed_start(struct ifnet *);
void ed_watchdog(struct ifnet *);
void ed_reset(struct ed_softc *);
void ed_init(struct ed_softc *);
void ed_stop(struct ed_softc *);
void ed_getmcaf(struct ethercom *, u_long *);
u_short ed_put(struct ed_softc *, struct mbuf *, void *);

#define inline	/* XXX for debugging porpoises */

void ed_get_packet(struct ed_softc *, void *, u_short);
static inline void ed_rint(struct ed_softc *);
static inline void ed_xmit(struct ed_softc *);
static inline void *ed_ring_copy(struct ed_softc *, void *, void *,
					u_short);

static inline void NIC_PUT(struct ed_softc *, int, u_char);
static inline u_char NIC_GET(struct ed_softc *, int);
static inline void word_copy(void *, void *, int);
static inline void word_zero(void *, int);
struct mbuf *ed_ring_to_mbuf(struct ed_softc *, void *, struct mbuf *,
					u_short);

CFATTACH_DECL(ed_zbus, sizeof(struct ed_softc),
    ed_zbus_match, ed_zbus_attach, NULL, NULL);

static inline void
NIC_PUT(struct ed_softc *sc, int off, u_char val)
{
	sc->nic_addr[off * 2] = val;
#ifdef not_def
	/*
	 * This was being used to *slow* access to the bus.  I don't
	 * believe it is needed but I'll leave it around incase probelms
	 * pop-up
	 */
	(void)ciaa.pra;
#endif
}

static inline u_char
NIC_GET(struct ed_softc *sc, int off)
{
	register u_char val;

	val = sc->nic_addr[off * 2];
#ifdef not_def
	/*
	 * This was being used to *slow* access to the bus.  I don't
	 * believe it is needed but I'll leave it around incase probelms
	 * pop-up
	 */
	(void)ciaa.pra;
#endif
	return (val);
}

/*
 * Memory copy, copies word at time.
 */
static inline void
word_copy(void *a, void *b, int len)
{
	u_short *x = (u_short *)a,
		*y = (u_short *)b;

	len >>= 1;
	while (len--)
		*y++ = *x++;
}

/*
 * zero memory, one word at time.
 */
static inline void
word_zero(void *a, int len)
{
	u_short *x = (u_short *)a;

	len >>= 1;
	while (len--)
		*x++ = 0;
}

int
ed_zbus_match(struct device *parent, struct cfdata *cfp, void *aux)
{
	struct zbus_args *zap = aux;

	if (zap->manid == HYDRA_MANID && zap->prodid == HYDRA_PRODID)
		return (1);
	else if (zap->manid == ASDG_MANID && zap->prodid == ASDG_PRODID)
		return (1);
	return (0);
}

void
ed_zbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ed_softc *sc = (void *)self;
	struct zbus_args *zap = aux;
	struct cfdata *cf = device_cfdata(&sc->sc_dev);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	volatile u_char *prom;
	int i;
	u_int8_t myaddr[ETHER_ADDR_LEN];

	if (zap->manid == HYDRA_MANID) {
		sc->mem_start = zap->va;
		sc->mem_size = 16384;
		sc->nic_addr = (u_char *)sc->mem_start + HYDRA_NIC_BASE;
		prom = (u_char *)sc->mem_start + HYDRA_ADDRPROM;
	} else {
		sc->mem_start = (u_char *)zap->va + 0x8000;
		sc->mem_size = 16384;
		sc->nic_addr = (volatile u_char *)zap->va + ASDG_NIC_BASE;
		prom = (volatile u_char *)sc->nic_addr + ASDG_ADDRPROM;
	}
	sc->cr_proto = ED_CR_RD2;
	sc->tx_page_start = 0;

	sc->mem_end = (u_char *)sc->mem_start + sc->mem_size;

	/*
	 * Use one xmit buffer if < 16k, two buffers otherwise (if not told
	 * otherwise).
	 */
	if ((sc->mem_size < 16384) || zap->manid == ASDG_MANID
	    || (cf->cf_flags & ED_FLAGS_NO_MULTI_BUFFERING))
		sc->txb_cnt = 1;
	else
		sc->txb_cnt = 2;

	sc->rec_page_start = sc->tx_page_start + sc->txb_cnt * ED_TXBUF_SIZE;
	sc->rec_page_stop = sc->tx_page_start + (sc->mem_size >> ED_PAGE_SHIFT);

	sc->mem_ring =
	    (u_char *)sc->mem_start + 
		((sc->txb_cnt * ED_TXBUF_SIZE) << ED_PAGE_SHIFT);

	/*
	 * Interrupts must be inactive when reading the prom, as the interrupt
	 * line is shared with one of its address lines.
	 */

	NIC_PUT(sc, ED_P0_IMR, 0x00); /* disable ints */
	NIC_PUT(sc, ED_P0_ISR, 0xff); /* clear ints */

	/*
	 * read the ethernet address from the board
	 */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		myaddr[i] = *(prom + 2 * i);

	/* Set interface to stopped condition (reset). */
	ed_stop(sc);

	/* Initialize ifnet structure. */
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ed_start;
	ifp->if_ioctl = ed_ioctl;
	ifp->if_watchdog = ed_watchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

	/* Print additional info when attached. */
	printf(": address %s\n", ether_sprintf(myaddr));

	sc->sc_isr.isr_intr = edintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);
}

/*
 * Reset interface.
 */
void
ed_reset(struct ed_softc *sc)
{
	int s;

	s = splnet();
	ed_stop(sc);
	ed_init(sc);
	splx(s);
	log(LOG_ERR, "%s: reset\n", sc->sc_dev.dv_xname);
}

/*
 * Take interface offline.
 */
void
ed_stop(struct ed_softc *sc)
{
	int n = 5000;

	/* Stop everything on the interface, and select page 0 registers. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	/*
	 * Wait for interface to enter stopped state, but limit # of checks to
	 * 'n' (about 5ms).  It shouldn't even take 5us on modern DS8390's, but
	 * just in case it's an old one.
	 */
	while (((NIC_GET(sc, ED_P0_ISR) & ED_ISR_RST) == 0) && --n);
}

/*
 * Device timeout/watchdog routine.  Entered if the device neglects to generate
 * an interrupt after a transmit has been started on it.
 */
void
ed_watchdog(struct ifnet *ifp)
{
	struct ed_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;

	ed_reset(sc);
}

/*
 * Initialize device.
 */
void
ed_init(struct ed_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i, s;
	u_long mcaf[2];

	/*
	 * Initialize the NIC in the exact order outlined in the NS manual.
	 * This init procedure is "mandatory"...don't change what or when
	 * things happen.
	 */
	s = splnet();

	/* Reset transmitter flags. */
	sc->xmit_busy = 0;
	ifp->if_timer = 0;

	sc->txb_inuse = 0;
	sc->txb_new = 0;
	sc->txb_next_tx = 0;

	/* Set interface for page 0, remote DMA complete, stopped. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	/*
	 * Set FIFO threshold to 8, No auto-init Remote DMA, byte
	 * order=68k, word-wide DMA xfers,
	 * XXX changed to use 2 word threshhold
	 */
	NIC_PUT(sc, ED_P0_DCR,
	    ED_DCR_FT0 | ED_DCR_WTS | ED_DCR_LS | ED_DCR_BOS);

	/* Clear remote byte count registers. */
	NIC_PUT(sc, ED_P0_RBCR0, 0);
	NIC_PUT(sc, ED_P0_RBCR1, 0);

	/* Tell RCR to do nothing for now. */
	NIC_PUT(sc, ED_P0_RCR, ED_RCR_MON);

	/* Place NIC in internal loopback mode. */
	NIC_PUT(sc, ED_P0_TCR, ED_TCR_LB0);

	/* Initialize receive buffer ring. */
	NIC_PUT(sc, ED_P0_BNRY, sc->rec_page_start);
	NIC_PUT(sc, ED_P0_PSTART, sc->rec_page_start);
	NIC_PUT(sc, ED_P0_PSTOP, sc->rec_page_stop);

	/*
	 * Clear all interrupts.  A '1' in each bit position clears the
	 * corresponding flag.
	 */
	NIC_PUT(sc, ED_P0_ISR, 0xff);

	/*
	 * Enable the following interrupts: receive/transmit complete,
	 * receive/transmit error, and Receiver OverWrite.
	 *
	 * Counter overflow and Remote DMA complete are *not* enabled.
	 */
	NIC_PUT(sc, ED_P0_IMR,
	    ED_IMR_PRXE | ED_IMR_PTXE | ED_IMR_RXEE | ED_IMR_TXEE |
	    ED_IMR_OVWE);

	/* Program command register for page 1. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_1 | ED_CR_STP);

	/* Copy out our station address. */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		NIC_PUT(sc, ED_P1_PAR0 + i, CLLADDR(ifp->if_sadl)[i]);

	/* Set multicast filter on chip. */
	ed_getmcaf(&sc->sc_ethercom, mcaf);
	for (i = 0; i < 8; i++)
		NIC_PUT(sc, ED_P1_MAR0 + i, ((u_char *)mcaf)[i]);

	/*
	 * Set current page pointer to one page after the boundary pointer, as
	 * recommended in the National manual.
	 */
	sc->next_packet = sc->rec_page_start + 1;
	NIC_PUT(sc, ED_P1_CURR, sc->next_packet);

	/* Program command register for page 0. */
	NIC_PUT(sc, ED_P1_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	i = ED_RCR_AB | ED_RCR_AM;
	if (ifp->if_flags & IFF_PROMISC) {
		/*
		 * Set promiscuous mode.  Multicast filter was set earlier so
		 * that we should receive all multicast packets.
		 */
		i |= ED_RCR_PRO | ED_RCR_AR | ED_RCR_SEP;
	}
	NIC_PUT(sc, ED_P0_RCR, i);

	/* Take interface out of loopback. */
	NIC_PUT(sc, ED_P0_TCR, 0);

	/* Fire up the interface. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set 'running' flag, and clear output active flag. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* ...and attempt to start output. */
	ed_start(ifp);

	splx(s);
}

/*
 * This routine actually starts the transmission on the interface.
 */
static inline void
ed_xmit(struct ed_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_short len;

	len = sc->txb_len[sc->txb_next_tx];

	/* Set NIC for page 0 register access. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set TX buffer start page. */
	NIC_PUT(sc, ED_P0_TPSR, sc->tx_page_start +
	    sc->txb_next_tx * ED_TXBUF_SIZE);

	/* Set TX length. */
	NIC_PUT(sc, ED_P0_TBCR0, len);
	NIC_PUT(sc, ED_P0_TBCR1, len >> 8);

	/* Set page 0, remote DMA complete, transmit packet, and *start*. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_TXP | ED_CR_STA);
	sc->xmit_busy = 1;

	/* Point to next transmit buffer slot and wrap if necessary. */
	sc->txb_next_tx++;
	if (sc->txb_next_tx == sc->txb_cnt)
		sc->txb_next_tx = 0;

	/* Set a timer just in case we never hear from the board again. */
	ifp->if_timer = 2;
}

/*
 * Start output on interface.
 * We make two assumptions here:
 *  1) that the current priority is set to splnet _before_ this code
 *     is called *and* is returned to the appropriate priority after
 *     return
 *  2) that the IFF_OACTIVE flag is checked before this code is called
 *     (i.e. that the output part of the interface is idle)
 */
void
ed_start(struct ifnet *ifp)
{
	struct ed_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	void *buffer;
	int len;

outloop:
	/*
	 * First, see if there are buffered packets and an idle transmitter -
	 * should never happen at this point.
	 */
	if (sc->txb_inuse && (sc->xmit_busy == 0)) {
		printf("%s: packets buffered, but transmitter idle\n",
		    sc->sc_dev.dv_xname);
		ed_xmit(sc);
	}

	/* See if there is room to put another packet in the buffer. */
	if (sc->txb_inuse == sc->txb_cnt) {
		/* No room.  Indicate this to the outside world and exit. */
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IF_DEQUEUE(&ifp->if_snd, m);
	if (m == 0) {
		/*
		 * We are using the !OACTIVE flag to indicate to the outside
		 * world that we can accept an additional packet rather than
		 * that the transmitter is _actually_ active.  Indeed, the
		 * transmitter may be active, but if we haven't filled all the
		 * buffers with data then we still want to accept more.
		 */
		ifp->if_flags &= ~IFF_OACTIVE;
		return;
	}

	/* Copy the mbuf chain into the transmit buffer. */
	m0 = m;

	/* txb_new points to next open buffer slot. */
	buffer = (char*)sc->mem_start +
		((sc->txb_new * ED_TXBUF_SIZE) << ED_PAGE_SHIFT);

	len = ed_put(sc, m, buffer);

	sc->txb_len[sc->txb_new] = len;
	sc->txb_inuse++;

	/* Point to next buffer slot and wrap if necessary. */
	if (++sc->txb_new == sc->txb_cnt)
		sc->txb_new = 0;

	if (sc->xmit_busy == 0)
		ed_xmit(sc);

	/* Tap off here if there is a BPF listener. */
	bpf_mtap(ifp, m0);

	m_freem(m0);

	/* Loop back to the top to possibly buffer more packets. */
	goto outloop;
}

/*
 * Ethernet interface receiver interrupt.
 */
static inline void
ed_rint(struct ed_softc *sc)
{
	struct ifnet *ifp;
	void *packet_ptr;
	u_short len;
	u_char nlen;
	u_char boundary, current;
	struct ed_ring packet_hdr;

	ifp = &sc->sc_ethercom.ec_if;
loop:
	/* Set NIC to page 1 registers to get 'current' pointer. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_1 | ED_CR_STA);

	/*
	 * 'sc->next_packet' is the logical beginning of the ring-buffer - i.e.
	 * it points to where new data has been buffered.  The 'CURR' (current)
	 * register points to the logical end of the ring-buffer - i.e. it
	 * points to where additional new data will be added.  We loop here
	 * until the logical beginning equals the logical end (or in other
	 * words, until the ring-buffer is empty).
	 */
	current = NIC_GET(sc, ED_P1_CURR);
	if (sc->next_packet == current)
		return;

	/* Set NIC to page 0 registers to update boundary register. */
	NIC_PUT(sc, ED_P1_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	do {
		/* Get pointer to this buffer's header structure. */
		packet_ptr = (char*)sc->mem_ring +
		    ((sc->next_packet - sc->rec_page_start) << ED_PAGE_SHIFT);

		/*
		 * The byte count includes a 4 byte header that was added by
		 * the NIC.
		 */
		packet_hdr = *(struct ed_ring *)packet_ptr;
		packet_hdr.count = ((packet_hdr.count >> 8) & 0xff)
		     | ((packet_hdr.count & 0xff) << 8);
		len = packet_hdr.count;
		/*
		 * Try do deal with old, buggy chips that sometimes duplicate
		 * the low byte of the length into the high byte.  We do this
		 * by simply ignoring the high byte of the length and always
		 * recalculating it.
		 *
		 * NOTE: sc->next_packet is pointing at the current packet.
		 */
		if (packet_hdr.next_packet >= sc->next_packet)
			nlen = (packet_hdr.next_packet - sc->next_packet);
		else
			nlen = ((packet_hdr.next_packet - sc->rec_page_start) +
				(sc->rec_page_stop - sc->next_packet));
		--nlen;
		if ((len & ED_PAGE_MASK) + sizeof(packet_hdr) > ED_PAGE_SIZE)
			--nlen;
		len = (len & ED_PAGE_MASK) | (nlen << ED_PAGE_SHIFT);
#ifdef DIAGNOSTIC
		if (len != packet_hdr.count) {
			printf("%s: length does not match next packet pointer\n",
			    sc->sc_dev.dv_xname);
			printf("%s: len %04x nlen %04x start %02x first %02x curr %02x next %02x stop %02x\n",
			    sc->sc_dev.dv_xname, packet_hdr.count, len,
			    sc->rec_page_start, sc->next_packet, current,
			    packet_hdr.next_packet, sc->rec_page_stop);
		}
#endif

		/*
		 * Be fairly liberal about what we allow as a "reasonable"
		 * length so that a [crufty] packet will make it to BPF (and
		 * can thus be analyzed).  Note that all that is really
		 * important is that we have a length that will fit into one
		 * mbuf cluster or less; the upper layer protocols can then
		 * figure out the length from their own length field(s).
		 *
		 * MCLBYTES may be less than a valid packet len.  Thus
		 * we use a constant that is large enough.
		 */
		if (len <= 2048 &&
		    packet_hdr.next_packet >= sc->rec_page_start &&
		    packet_hdr.next_packet < sc->rec_page_stop) {
			/* Go get packet. */
			ed_get_packet(sc, (char*)packet_ptr +
			    sizeof(struct ed_ring),
			    len - sizeof(struct ed_ring));
			++ifp->if_ipackets;
		} else {
			/* Really BAD.  The ring pointers are corrupted. */
#ifdef DEBUG_AMIGA_IF_ED
			log(LOG_ERR,
			    "%s: NIC memory corrupt - invalid packet length %d\n",
			    sc->sc_dev.dv_xname, len);
#endif
			++ifp->if_ierrors;
			ed_reset(sc);
			return;
		}

		/* Update next packet pointer. */
		sc->next_packet = packet_hdr.next_packet;

		/*
		 * Update NIC boundary pointer - being careful to keep it one
		 * buffer behind (as recommended by NS databook).
		 */
		boundary = sc->next_packet - 1;
		if (boundary < sc->rec_page_start)
			boundary = sc->rec_page_stop - 1;
		NIC_PUT(sc, ED_P0_BNRY, boundary);
	} while (sc->next_packet != current);

	goto loop;
}

/* Ethernet interface interrupt processor. */
int
edintr(void *arg)
{
	struct ed_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_char isr;

	/* Set NIC to page 0 registers. */
	NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	isr = NIC_GET(sc, ED_P0_ISR);
	if (!isr)
		return (0);

	/* Loop until there are no more new interrupts. */
	for (;;) {
		/*
		 * Reset all the bits that we are 'acknowledging' by writing a
		 * '1' to each bit position that was set.
		 * (Writing a '1' *clears* the bit.)
		 */
		NIC_PUT(sc, ED_P0_ISR, isr);

		/*
		 * Handle transmitter interrupts.  Handle these first because
		 * the receiver will reset the board under some conditions.
		 */
		if (isr & (ED_ISR_PTX | ED_ISR_TXE)) {
			u_char collisions = NIC_GET(sc, ED_P0_NCR) & 0x0f;

			/*
			 * Check for transmit error.  If a TX completed with an
			 * error, we end up throwing the packet away.  Really
			 * the only error that is possible is excessive
			 * collisions, and in this case it is best to allow the
			 * automatic mechanisms of TCP to backoff the flow.  Of
			 * course, with UDP we're screwed, but this is expected
			 * when a network is heavily loaded.
			 */
			(void) NIC_GET(sc, ED_P0_TSR);
			if (isr & ED_ISR_TXE) {
				/*
				 * Excessive collisions (16).
				 */
				if ((NIC_GET(sc, ED_P0_TSR) & ED_TSR_ABT)
				    && (collisions == 0)) {
					/*
					 * When collisions total 16, the P0_NCR
					 * will indicate 0, and the TSR_ABT is
					 * set.
					 */
					collisions = 16;
				}

				/* Update output errors counter. */
				++ifp->if_oerrors;
			} else {
				/*
				 * Update total number of successfully
				 * transmitted packets.
				 */
				++ifp->if_opackets;
			}

			/* Reset TX busy and output active flags. */
			sc->xmit_busy = 0;
			ifp->if_flags &= ~IFF_OACTIVE;

			/* Clear watchdog timer. */
			ifp->if_timer = 0;

			/*
			 * Add in total number of collisions on last
			 * transmission.
			 */
			ifp->if_collisions += collisions;

			/*
			 * Decrement buffer in-use count if not zero (can only
			 * be zero if a transmitter interrupt occurred while not
			 * actually transmitting).
			 * If data is ready to transmit, start it transmitting,
			 * otherwise defer until after handling receiver.
			 */
			if (sc->txb_inuse && --sc->txb_inuse)
				ed_xmit(sc);
		}

		/* Handle receiver interrupts. */
		if (isr & (ED_ISR_PRX | ED_ISR_RXE | ED_ISR_OVW)) {
			/*
			 * Overwrite warning.  In order to make sure that a
			 * lockup of the local DMA hasn't occurred, we reset
			 * and re-init the NIC.  The NSC manual suggests only a
			 * partial reset/re-init is necessary - but some chips
			 * seem to want more.  The DMA lockup has been seen
			 * only with early rev chips - Methinks this bug was
			 * fixed in later revs.  -DG
			 */
			if (isr & ED_ISR_OVW) {
				++ifp->if_ierrors;
#ifdef DIAGNOSTIC
				log(LOG_WARNING,
				    "%s: warning - receiver ring buffer overrun\n",
				    sc->sc_dev.dv_xname);
#endif
				/* Stop/reset/re-init NIC. */
				ed_reset(sc);
			} else {
				/*
				 * Receiver Error.  One or more of: CRC error,
				 * frame alignment error FIFO overrun, or
				 * missed packet.
				 */
				if (isr & ED_ISR_RXE) {
					++ifp->if_ierrors;
#ifdef ED_DEBUG
					printf("%s: receive error %x\n",
					    sc->sc_dev.dv_xname,
					    NIC_GET(sc, ED_P0_RSR));
#endif
				}

				/*
				 * Go get the packet(s).
				 * XXX - Doing this on an error is dubious
				 * because there shouldn't be any data to get
				 * (we've configured the interface to not
				 * accept packets with errors).
				 */
				ed_rint(sc);
			}
		}

		/*
		 * If it looks like the transmitter can take more data, attempt
		 * to start output on the interface.  This is done after
		 * handling the receiver to give the receiver priority.
		 */
		if ((ifp->if_flags & IFF_OACTIVE) == 0)
			ed_start(ifp);

		/*
		 * Return NIC CR to standard state: page 0, remote DMA
		 * complete, start (toggling the TXP bit off, even if was just
		 * set in the transmit routine, is *okay* - it is 'edge'
		 * triggered from low to high).
		 */
		NIC_PUT(sc, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

		/*
		 * If the Network Talley Counters overflow, read them to reset
		 * them.  It appears that old 8390's won't clear the ISR flag
		 * otherwise - resulting in an infinite loop.
		 */
		if (isr & ED_ISR_CNT) {
			(void) NIC_GET(sc, ED_P0_CNTR0);
			(void) NIC_GET(sc, ED_P0_CNTR1);
			(void) NIC_GET(sc, ED_P0_CNTR2);
		}

		isr = NIC_GET(sc, ED_P0_ISR);
		if (!isr)
			return (1);
	}
}

/*
 * Process an ioctl request.  This code needs some work - it looks pretty ugly.
 */
int
ed_ioctl(register struct ifnet *ifp, u_long cmd, void *data)
{
	struct ed_softc *sc = ifp->if_softc;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			ed_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		/* XXX - This code is probably wrong. */
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
			/* Set new address. */
			ed_init(sc);
			break;
		    }
#endif
		default:
			ed_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/* XXX if ed_init() would ed_stop(, 0), first, perhaps we
		 * can re-use the code in ether_ioctl().
		 */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			ed_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			ed_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			ed_stop(sc);
			ed_init(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* Update our multicast list. */
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING) {
				ed_stop(sc); /* XXX for ds_setmcaf? */
				ed_init(sc);
			}
			error = 0;
		}
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);
	return (error);
}

/*
 * Retrieve packet from shared memory and send to the next level up via
 * ether_input().  If there is a BPF listener, give a copy to BPF, too.
 */
void
ed_get_packet(struct ed_softc *sc, void *buf, u_short len)
{
	struct mbuf *m;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	/* round length to word boundry */
	len = (len + 1) & ~1;

	/* Allocate a header mbuf. */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = len;
	m->m_len = 0;

	/* The following silliness is to make NFS happy. */
#define EROUND	((sizeof(struct ether_header) + 3) & ~3)
#define EOFF	(EROUND - sizeof(struct ether_header))

	/*
	 * The following assumes there is room for the ether header in the
	 * header mbuf.
	 */
	m->m_data += EOFF;

	word_copy(buf, mtod(m, void *), sizeof(struct ether_header));
	buf = (char*)buf + sizeof(struct ether_header);
	m->m_len += sizeof(struct ether_header);
	len -= sizeof(struct ether_header);

	/* Pull packet off interface. */
	if (ed_ring_to_mbuf(sc, buf, m, len) == 0) {
		m_freem(m);
		return;
	}

	bpf_mtap(ifp, m);

	(*ifp->if_input)(ifp, m);
}

/*
 * Supporting routines.
 */

/*
 * Given a source and destination address, copy 'amount' of a packet from the
 * ring buffer into a linear destination buffer.  Takes into account ring-wrap.
 */
static inline void *
ed_ring_copy(struct ed_softc *sc, void *src, void *dst, u_short amount)
{
	u_short tmp_amount;

	/* Does copy wrap to lower addr in ring buffer? */
	if ((char*)src + amount > (char*)sc->mem_end) {
		tmp_amount = (char*)sc->mem_end - (char*)src;

		/* Copy amount up to end of NIC memory. */
		word_copy(src, dst, tmp_amount);

		amount -= tmp_amount;
		src = sc->mem_ring;
		dst = (char*)dst + tmp_amount;
	}

	word_copy(src, dst, amount);

	return ((char*)src + amount);
}

/*
 * Copy data from receive buffer to end of mbuf chain allocate additional mbufs
 * as needed.  Return pointer to last mbuf in chain.
 * sc = ed info (softc)
 * src = pointer in ed ring buffer
 * dst = pointer to last mbuf in mbuf chain to copy to
 * amount = amount of data to copy
 */
struct mbuf *
ed_ring_to_mbuf(struct ed_softc *sc, void *src, struct mbuf *dst,
                u_short total_len)
{
	register struct mbuf *m = dst;

	/* Round the length to a word boundary. */
	/* total_len = (total_len + 1) & ~1; */

	while (total_len) {
		register u_short amount = min(total_len, M_TRAILINGSPACE(m));

		if (amount == 0) {
			/*
			 * No more data in this mbuf; alloc another.
			 *
			 * If there is enough data for an mbuf cluster, attempt
			 * to allocate one of those, otherwise, a regular mbuf
			 * will do.
			 * Note that a regular mbuf is always required, even if
			 * we get a cluster - getting a cluster does not
			 * allocate any mbufs, and one is needed to assign the
			 * cluster to.  The mbuf that has a cluster extension
			 * can not be used to contain data - only the cluster
			 * can contain data.
			 */
			dst = m;
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0)
				return (0);

			if (total_len >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);

			m->m_len = 0;
			dst->m_next = m;
			amount = min(total_len, M_TRAILINGSPACE(m));
		}

		src = ed_ring_copy(sc, src, mtod(m, char *) + m->m_len,
		    amount);

		m->m_len += amount;
		total_len -= amount;
	}
	return (m);
}

/*
 * Compute the multicast address filter from the list of multicast addresses we
 * need to listen to.
 */
void
ed_getmcaf(struct ethercom *ac, u_long *af)
{
	struct ifnet *ifp = &ac->ec_if;
	struct ether_multi *enm;
	register u_char *cp, c;
	register u_long crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 6 bits as an
	 * index into the 64 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC) {
		ifp->if_flags |= IFF_ALLMULTI;
		af[0] = af[1] = 0xffffffff;
		return;
	}

	af[0] = af[1] = 0;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			af[0] = af[1] = 0xffffffff;
			return;
		}

		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if (((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01)) {
					crc <<= 1;
					crc ^= 0x04c11db6 | 1;
				} else
					crc <<= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Turn on the corresponding bit in the filter. */
		af[crc >> 5] |= 1 << ((crc & 0x1f) ^ 0);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}

/*
 * Copy packet from mbuf to the board memory
 *
 * Currently uses an extra buffer/extra memory copy,
 * unless the whole packet fits in one mbuf.
 *
 */
u_short
ed_put(struct ed_softc *sc, struct mbuf *m, void *buf)
{
	u_char *data, savebyte[2];
	int len, wantbyte;
	u_short totlen;

	totlen = wantbyte = 0;

	for (; m != 0; m = m->m_next) {
		data = mtod(m, u_char *);
		len = m->m_len;
		totlen += len;
		if (len > 0) {
			/* Finish the last word. */
			if (wantbyte) {
				savebyte[1] = *data;
				word_copy(savebyte, buf, 2);
				buf = (char*)buf + 2;
				data++;
				len--;
				wantbyte = 0;
			}
			/* Output contiguous words. */
			if (len > 1) {
				word_copy(data, buf, len);
				buf = (char*)buf + (len & ~1);
				data += len & ~1;
				len &= 1;
			}
			/* Save last byte, if necessary. */
			if (len == 1) {
				savebyte[0] = *data;
				wantbyte = 1;
			}
		}
	}

	if (wantbyte) {
		savebyte[1] = 0;
		word_copy(savebyte, buf, 2);
		buf = (char*)buf + 2;
		totlen++;
	}
	if (totlen < ETHER_MIN_LEN - ETHER_CRC_LEN) {
		word_zero(buf, ETHER_MIN_LEN - ETHER_CRC_LEN - totlen);
		totlen = ETHER_MIN_LEN - ETHER_CRC_LEN;
	}

	return (totlen);
}
