/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/coreasm.h>
#include <xtensa/corebits.h>
#include <xtensa/cacheasm.h>
#include <xtensa/cacheattrasm.h>
#include <xtensa/xtensa-xer.h>
#include <xtensa/xdm-regs.h>
#include <xtensa/config/specreg.h>
#include <xtensa/specreg.h>
#include <xtensa/config/system.h>  /* for XSHAL_USE_ABSOLUTE_LITERALS only */
#include <xtensa/xtruntime-core-state.h>

//#if 1
#ifndef XCHAL_HAVE_XEA3
/*
 * The following reset vector avoids initializing certain registers already
 * initialized by processor reset.  But it does initialize some of them
 * anyway, for minimal support of warm restart (restarting in software by
 * jumping to the reset vector rather than asserting hardware reset).
 */

	.begin	literal_prefix	.ResetVector
	.section		.ResetVector.text, "ax"

	.align	4
	.global	__start
__start:

#if (!XCHAL_HAVE_HALT || defined(XTOS_UNPACK)) && XCHAL_HAVE_IMEM_LOADSTORE
	/*
	 *  NOTE:
	 *
	 *  IMPORTANT:  If you move the _ResetHandler portion to a section
	 *  other than .ResetVector.text that is outside the range of
	 *  the reset vector's 'j' instruction, the _ResetHandler symbol
	 *  and a more elaborate j/movi/jx sequence are needed in
	 *  .ResetVector.text to dispatch to the new location.
	 */
	j	_ResetHandler

	.size	__start, . - __start

#if XCHAL_HAVE_HALT
	/*
	 *  Xtensa TX: reset vector segment is only 4 bytes, so must place the
	 *  unpacker code elsewhere in the memory that contains the reset
	 *  vector.
	 */
#if XCHAL_RESET_VECTOR_VADDR == XCHAL_INSTRAM0_VADDR
	.section .iram0.text, "ax"
#elif XCHAL_RESET_VECTOR_VADDR == XCHAL_INSTROM0_VADDR
	.section .irom0.text, "ax"
#elif XCHAL_RESET_VECTOR_VADDR == XCHAL_URAM0_VADDR
	.section .uram0.text, "ax"
#else
#warning "Xtensa TX reset vector not at start of iram0, irom0, or uram0 -- ROMing LSPs may not work"
	.text
#endif
#endif /* XCHAL_HAVE_HALT */

	.extern	__memctl_default

	.align	4

	/* tells the assembler/linker to place literals here */
	.literal_position
	.align	4
	.global	_ResetHandler
_ResetHandler:
#endif

#if !XCHAL_HAVE_HALT

	/*
	 *  Even if the processor supports the non-PC-relative L32R option,
	 *  it will always start up in PC-relative mode.  We take advantage of
	 *  this, and use PC-relative mode at least until we're sure the .lit4
	 *  section is in place (which is sometimes only after unpacking).
	 */
	.begin	no-absolute-literals

	/*
	 * If we have dynamic cache way support, init the caches as soon
	 * as we can, which is now. Except, if we are waking up from a
	 * PSO event, then we need to do this slightly later.
	 */
#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
# if XCHAL_HAVE_PSO_CDM && !XCHAL_HAVE_PSO_FULL_RETENTION
	 /* Do this later on in the code -- see below */
# else
	movi	a0, __memctl_default
	wsr	a0, MEMCTL
# endif
#endif

	/*
	 * If we have PSO support, then we must check for a warm start with
	 * caches left powered on. If the caches had been left powered on,
	 * we must restore the state of MEMCTL to the saved state if any.
	 * Note that MEMCTL may not be present depending on config.
	 */
#if XCHAL_HAVE_PSO_CDM && !XCHAL_HAVE_PSO_FULL_RETENTION
	/* Read PWRSTAT */
	movi	a2, XDM_MISC_PWRSTAT
	/* Save area address - retained for later */
	/*movi	a3, _xtos_pso_savearea*/
	/* Signature for compare - retained for later */
	movi	a5, CORE_STATE_SIGNATURE
	 /* PWRSTAT value - retained for later */
	rer	a7, a2
	/* Now bottom 2 bits are core wakeup and cache power lost */
	extui	a4, a7, 1, 2
	/* a4==1 means PSO wakeup, caches did not lose power */
	bnei	a4, 1, .Lcold_start
	/* Load save area signature field */
	l32i	a4, a3, CS_SA_signature
	sub	a4, a4, a5
	/* If signature mismatch then do cold start */
	bnez	a4, .Lcold_start
#if XCHAL_USE_MEMCTL
	/* Load saved MEMCTL value */
	l32i	a4, a3, CS_SA_memctl
	movi	a0, ~MEMCTL_INV_EN
	/* Clear invalidate bit */
	and	a0, a4, a0
	wsr	a0, MEMCTL
#endif
	j	.Lwarm_start

.Lcold_start:
#ifndef XCHAL_HAVE_XEA3
#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
	/*
	 * Enable and invalidate all ways of both caches. If there is no
	 * dynamic way support then this write will have no effect.
	 */
	movi	a0, __memctl_default
	wsr	a0, MEMCTL
#endif
#endif

.Lwarm_start:

#endif
	/* a0 is always 0 in this code, used to initialize lots of things */
	movi	a0, 0

#ifndef XCHAL_HAVE_XEA3
/* technically this should be under !FULL_RESET, assuming hard reset */
#if XCHAL_HAVE_INTERRUPTS
	/* make sure that interrupts are shut off (*before* we lower
	 * PS.INTLEVEL and PS.EXCM!)
	 */
	wsr	a0, INTENABLE
	 //movi    a0, PS_DI_MASK
     //xps     a0, a0
#endif
#endif
#if !XCHAL_HAVE_FULL_RESET

/* pre-LX2 cores only */
#if XCHAL_HAVE_CCOUNT && (XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RB_2006_0)
	/* not really necessary, but nice; best done very early */
	wsr	a0, CCOUNT
#endif

	/*
	 * For full MMU configs, put page table at an unmapped virtual address.
	 * This ensures that accesses outside the static maps result
	 * in miss exceptions rather than random behaviour.
	 * Assumes XCHAL_SEG_MAPPABLE_VADDR == 0 (true in released MMU).
	 */
#if XCHAL_ITLB_ARF_WAYS > 0 || XCHAL_DTLB_ARF_WAYS > 0
	wsr	a0, PTEVADDR
#endif

	/*
	 * Debug initialization
	 *
	 * NOTE: DBREAKCn must be initialized before the combination of these
	 * two things: any load/store, and a lowering of PS.INTLEVEL below
	 * DEBUG_LEVEL.  The processor already resets IBREAKENABLE
	 * appropriately.
	 */
#if XCHAL_HAVE_DEBUG
#if XCHAL_NUM_DBREAK
#if XCHAL_NUM_DBREAK >= 2
	wsr	a0, DBREAKC1
#endif
	wsr	a0, DBREAKC0
	dsync			 /* wait for WSRs to DBREAKCn to complete */
#endif /* XCHAL_NUM_DBREAK */

/* pre-LX cores only */
# if XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RA_2004_1
	/*
	 *  Starting in Xtensa LX, ICOUNTLEVEL resets to zero (not 15), so no
	 *  need to initialize it.  Prior to that we do, otherwise we get an
	 *  ICOUNT exception, 2^32 instructions after reset.
	 */

	/* are we being debugged? (detected by ICOUNTLEVEL not 15, or dropped
	 * below 12)
	 */
	rsr	a2, ICOUNTLEVEL
	/* if so, avoid initializing ICOUNTLEVEL which drops single-steps
	 * through here
	 * */
	bltui	a2, 12, 1f
	 /* avoid ICOUNT exceptions */
	wsr	a0, ICOUNTLEVEL
	/* wait for WSR to ICOUNTLEVEL to complete */
	isync
1:
#endif
#endif /* XCHAL_HAVE_DEBUG */

#endif /* !XCHAL_HAVE_FULL_RESET */

#if XCHAL_HAVE_ABSOLUTE_LITERALS
	/* Technically, this only needs to be done under !FULL_RESET,
	 * assuming hard reset:
	 */
	wsr	a0, LITBASE
	rsync
#endif

#if XCHAL_HAVE_PSO_CDM && ! XCHAL_HAVE_PSO_FULL_RETENTION
	/*
	 * If we're powering up from a temporary power shut-off (PSO),
	 * restore state saved just prior to shut-off. Note that the
	 * MEMCTL register was already restored earlier, and as a side
	 * effect, registers a3, a5, a7 are now preloaded with values
	 * that we will use here.
	 * a3 - pointer to save area base address (_xtos_pso_savearea)
	 * a5 - saved state signature (CORE_STATE_SIGNATURE)
	 * a7 - contents of PWRSTAT register
	 */

	/* load save area signature */
	l32i	a4, a3, CS_SA_signature
	/* compare signature with expected one */
	sub	a4, a4, a5
# if XTOS_PSO_TEST
	/* pretend PSO warm start with warm caches */
	movi	a7, PWRSTAT_WAKEUP_RESET
# endif
	/* wakeup from PSO? (branch if not) */
	bbci.l	a7, PWRSTAT_WAKEUP_RESET_SHIFT, 1f
	/* Yes, wakeup from PSO.  Check whether state was properly saved.
	 * speculatively clear PSO-wakeup bit  */
	addi	a5, a7, - PWRSTAT_WAKEUP_RESET
	/* if state not saved (corrupted?), mark as cold start */
	movnez	a7, a5, a4
	/* if state not saved, just continue with reset */
	bnez	a4, 1f
	/* Wakeup from PSO with good signature.  Now check cache status:
	 * if caches warm, restore now  */
	bbci.l	a7, PWRSTAT_CACHES_LOST_POWER_SHIFT, .Lpso_restore
	/* Caches got shutoff.  Continue reset, we'll end up initializing
	 * caches, and check again later for PSO.
	 */
# if XCHAL_HAVE_PRID && XCHAL_HAVE_S32C1I
	j	.Ldonesync	 /* skip reset sync, only done for cold start */
# endif
1:	/*  Cold start.  (Not PSO wakeup.)  Proceed with normal full reset. */
#endif

#if XCHAL_HAVE_PRID && XCHAL_HAVE_S32C1I
	/* Core 0 initializes the XMP synchronization variable, if present.
	 * This operation needs to happen as early as possible in the startup
	 * sequence so that the other cores can be released from reset.
	 */
	.weak _ResetSync
	movi 	a2, _ResetSync	 /* address of sync variable */
	rsr.prid a3		 /* core and multiprocessor ID */
	extui 	a3, a3, 0, 8	 /* extract core ID (FIXME: need proper
				  * constants for PRID bits to extract) */
	beqz	a2, .Ldonesync	 /* skip if no sync variable */
	bnez	a3, .Ldonesync	 /* only do this on core 0 */
	s32i	a0, a2, 0	 /* clear sync variable */
.Ldonesync:
#endif
#if XCHAL_HAVE_EXTERN_REGS && XCHAL_HAVE_MP_RUNSTALL
	/* On core 0, this releases other cores.  On other cores this has no
	 * effect, because runstall control is unconnected
	 */
	movi	a2, XER_MPSCORE
	wer	a0, a2
#endif

	/*
	 * For processors with relocatable vectors, apply any alternate
	 * vector base given to xt-genldscripts, which sets the
	 * _memmap_vecbase_reset symbol accordingly.
	 */
#if XCHAL_HAVE_VECBASE
	/* note: absolute symbol, not a ptr */
	movi	a2, _memmap_vecbase_reset
	wsr	a2, vecbase
#endif

/* have ATOMCTL ? */
#if XCHAL_HAVE_S32C1I && (XCHAL_HW_MIN_VERSION >= XTENSA_HWVERSION_RC_2009_0)
#if XCHAL_DCACHE_IS_COHERENT
	/* MX -- internal for writeback, RCW otherwise */
	movi	a3, 0x25
#else
	/* non-MX -- always RCW */
	movi	a3, 0x15
#endif /* XCHAL_DCACHE_IS_COHERENT */
	wsr	a3, ATOMCTL
#endif

#if XCHAL_HAVE_INTERRUPTS && XCHAL_HAVE_DEBUG
	/* lower PS.INTLEVEL here to make reset vector easier to debug */
#ifndef XCHAL_HAVE_XEA3
	rsil	a2, 1
	// Save the current PS register
    //rsr.ps a0
	// Disable interrupts up to level 1
    //movi a2, 0x00000200   // Set INTLEVEL to 1 (bit[7:6] in PS)
    //wsr.ps a2
#endif
#endif

	/* If either of the caches does not have dynamic way support, then
	 * use the old (slow) method to init them. If the cache is absent
	 * the macros will expand to empty.
	 */
#if ! XCHAL_HAVE_ICACHE_DYN_WAYS
	icache_reset	a2, a3
#endif
#if ! XCHAL_HAVE_DCACHE_DYN_WAYS
	dcache_reset	a2, a3
#endif

#if XCHAL_HAVE_PSO_CDM && ! XCHAL_HAVE_PSO_FULL_RETENTION
	/* Here, a7 still contains status from the power status register,
	 * or zero if signature check failed.
	 */

	/* wakeup from PSO with good signature? */
	bbci.l	a7, PWRSTAT_WAKEUP_RESET_SHIFT, .Lcoldstart
	/* Yes, wakeup from PSO.  Caches had been powered down, now are
	 * initialized.
	 */
.Lpso_restore:
	/* Assume memory still initialized, so all code still unpacked etc.
	 * So we can just jump/call to relevant state restore code (wherever
	 * located).
	 */

	/* make shutoff routine return zero */
	movi	a2, 0
	/*movi	a3, _xtos_pso_savearea*/
	/* Here, as below for _start, call0 is used as an unlimited-range
	 * jump.
	 */
	/*call0	_xtos_core_restore_nw*/
	/*  (does not return) */
.Lcoldstart:
#endif

#if XCHAL_HAVE_PREFETCH
	/* Enable cache prefetch if present.  */
	movi	a2, XCHAL_CACHE_PREFCTL_DEFAULT
	wsr	a2, PREFCTL
#endif

	/*
	 *  Now setup the memory attributes.  On some cores this "enables"
	 *  caches.  We do this ahead of unpacking, so it can proceed more
	 *  efficiently.
	 *
	 *  The _memmap_cacheattr_reset symbol's value (address) is defined by
	 *  the LSP's linker script, as generated by xt-genldscripts.  If
	 *  defines 4-bit attributes for eight 512MB regions.
	 *
	 *  (NOTE:  for cores with the older MMU v1 or v2, or without any
	 *  memory protection mechanism, the following code has no effect.)
	 */
#if XCHAL_HAVE_MPU
	/*  If there's an empty background map, setup foreground maps to mimic
	 *  region protection:
	 */
# if XCHAL_MPU_ENTRIES >= 8 && XCHAL_MPU_BACKGROUND_ENTRIES <= 2
	.pushsection .rodata, "a"
	.global _xtos_mpu_attribs
	.align 4
_xtos_mpu_attribs:
	/*  Illegal	(---) */
	.word   0x00006000+XCHAL_MPU_ENTRIES-8
	/* Writeback	(rwx Cacheable Non-shareable wb rd-alloc wr-alloc) */
	.word   0x000F7700+XCHAL_MPU_ENTRIES-8
	/* WBNA		(rwx Cacheable Non-shareable wb rd-alloc) */
	.word   0x000D5700+XCHAL_MPU_ENTRIES-8
	/* Writethru	(rwx Cacheable Non-shareable wt rd-alloc) */
	.word   0x000C4700+XCHAL_MPU_ENTRIES-8
	/* Bypass	(rwx Device non-interruptible system-shareable) */
	.word   0x00006700+XCHAL_MPU_ENTRIES-8
	.popsection

	/*
	 * We assume reset state:  all MPU entries zeroed and disabled.
	 * Otherwise we'd need a loop to zero everything.
	 */
	/* note: absolute symbol, not a ptr */
	movi	a2, _memmap_cacheattr_reset
	movi	a3, _xtos_mpu_attribs
	movi	a4, 0x20000000	/* 512 MB delta */
	movi	a6, 8
	movi	a7, 1		/* MPU entry vaddr 0, with valid bit set */
	movi	a9, 0		/* cacheadrdis value */
	/* enable everything temporarily while MPU updates */
	wsr.cacheadrdis a9

	/* Write eight MPU entries, from the last one going backwards
	 * (entries n-1 thru n-8)
	 */
2:	extui	a8, a2, 28, 4	/* get next attribute nibble (msb first) */
	extui	a5, a8, 0, 2	/* lower two bit indicate whether cached */
	slli	a9, a9, 1	/* add a bit to cacheadrdis... */
	addi	a10, a9, 1	/* set that new bit if... */
	moveqz	a9, a10, a5	/* ... that region is non-cacheable */
	addx4	a5, a8, a3	/* index into _xtos_mpu_attribs table */
	addi	a8, a8, -5	/* make valid attrib indices negative */
	movgez	a5, a3, a8	/* if not valid attrib, use Illegal */
	l32i	a5, a5, 0	/* load access rights, memtype from table
				 * entry
				 */
	slli	a2, a2, 4
	sub	a7, a7, a4	/* next 512MB region (last to first) */
	addi	a6, a6, -1
	add	a5, a5, a6	/* add the index */
	wptlb	a5, a7		/* write the MPU entry */
	bnez	a6, 2b		/* loop until done */
# else
	/* default value of CACHEADRDIS for bgnd map */
	movi	a9, XCHAL_MPU_BG_CACHEADRDIS
# endif
	wsr.cacheadrdis a9			 /* update cacheadrdis */
#elif XCHAL_HAVE_CACHEATTR || XCHAL_HAVE_MIMIC_CACHEATTR \
		|| XCHAL_HAVE_XLT_CACHEATTR \
		|| (XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)
	/* note: absolute symbol, not a ptr */
	movi	a2, _memmap_cacheattr_reset
	/* set CACHEATTR from a2 (clobbers a3-a8) */
	cacheattr_set
#endif

	/* Now that caches are initialized, cache coherency can be enabled. */
#if XCHAL_DCACHE_IS_COHERENT
# if XCHAL_HAVE_EXTERN_REGS && XCHAL_HAVE_MX && \
		(XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RE_2012_0)
	/* Opt into coherence for MX (for backward compatibility / testing). */
	movi	a3, 1
	movi	a2, XER_CCON
	wer	a3, a2
# endif
#endif

	/* Enable zero-overhead loop instr buffer, and snoop responses, if
	 * configured.  If HW erratum 453 fix is to be applied, then don't
	 * enable loop instr buffer.
	 */
#if XCHAL_USE_MEMCTL && XCHAL_SNOOP_LB_MEMCTL_DEFAULT
	movi	a3, XCHAL_SNOOP_LB_MEMCTL_DEFAULT
	rsr	a2, MEMCTL
	or	a2, a2, a3
	wsr	a2, MEMCTL
#endif

	/* Caches are all up and running, clear PWRCTL.ShutProcOffOnPWait. */
#if XCHAL_HAVE_PSO_CDM
	movi	a2, XDM_MISC_PWRCTL
	movi	a4, ~PWRCTL_CORE_SHUTOFF
	rer	a3, a2
	and	a3, a3, a4
	wer	a3, a2
#endif

#endif /* !XCHAL_HAVE_HALT */

	/*
	 *  Unpack code and data (eg. copy ROMed segments to RAM, vectors into
	 *  their proper location, etc).
	 */

#if defined(XTOS_UNPACK)
	movi	a2, _rom_store_table
	beqz	a2, unpackdone
unpack:	l32i	a3, a2, 0	 /* start vaddr */
	l32i	a4, a2, 4	 /* end vaddr */
	l32i	a5, a2, 8	 /* store vaddr */
	addi	a2, a2, 12
	bgeu	a3, a4, upnext	 /* skip unless start < end */
uploop:	l32i 	a6, a5, 0
	addi	a5, a5, 4
	s32i	a6, a3, 0
	addi	a3, a3, 4
	bltu	a3, a4, uploop
	j	unpack
upnext:	bnez	a3, unpack
	bnez	a5, unpack
#endif /* XTOS_UNPACK */

unpackdone:

#if defined(XTOS_UNPACK) || defined(XTOS_MP)
	/*
	 * If writeback caches are configured and enabled, unpacked data must
	 * be written out to memory before trying to execute it:
	 */
	dcache_writeback_all	a2, a3, a4, 0
	/* ensure data written back is visible to i-fetch */
	icache_sync		a2
	/*
	 * Note:  no need to invalidate the i-cache after the above, because
	 * we already invalidated it further above and did not execute
	 * anything within unpacked regions afterwards.  [Strictly speaking,
	 * if an unpacked region follows this code very closely, it's possible
	 * for cache-ahead to have cached a bit of that unpacked region, so in
	 * the future we may need to invalidate the entire i-cache here again
	 * anyway.]
	 */
#endif


#if !XCHAL_HAVE_HALT	/* skip for TX */

	/*
	 *  Now that we know the .lit4 section is present (if got unpacked)
	 *  (and if absolute literals are used), initialize LITBASE to use it.
	 */
#if XCHAL_HAVE_ABSOLUTE_LITERALS && XSHAL_USE_ABSOLUTE_LITERALS
	/*
	 *  Switch from PC-relative to absolute (litbase-relative) L32R mode.
	 *  Set LITBASE to 256 kB beyond the start of the literals in .lit4
	 *  (aligns to the nearest 4 kB boundary, LITBASE does not have bits
	 *  1..11) and set the enable bit (_lit4_start is assumed 4-byte
	 *  aligned).
	 */
	movi	a2, _lit4_start + 0x40001
	wsr	a2, LITBASE
	rsync
#endif /* have and use absolute literals */
	/* we can now start using absolute literals */
	.end	no-absolute-literals

	/* Technically, this only needs to be done pre-LX2, assuming hard
	 * reset:
	 */
# if XCHAL_HAVE_WINDOWED && defined(__XTENSA_WINDOWED_ABI__)
	/* Windowed register init, so we can call windowed code (eg. C code). */
	movi	a1, 1
#ifndef XCHAL_HAVE_XEA3
	wsr	a1, WINDOWSTART
#endif
	/*
	 *  The processor always clears WINDOWBASE at reset, so no need to
	 *  clear it here.  It resets WINDOWSTART to 1 starting with LX2.0/X7.0
	 *  (RB-2006.0).  However, assuming hard reset is not yet always
	 *  practical, so do this anyway:
	 */
#ifndef XCHAL_HAVE_XEA3
	wsr	a0, WINDOWBASE
#endif
	rsync
	movi	a0, 0			 /* possibly a different a0, clear it */
# endif

/* only pre-LX2 needs this */
#if XCHAL_HW_MIN_VERSION < XTENSA_HWVERSION_RB_2006_0
	/* Coprocessor option initialization */
# if XCHAL_HAVE_CP
	/*
	 * To allow creating new coprocessors using TC that are not known
	 * at GUI build time without having to explicitly enable them,
	 * all CPENABLE bits must be set, even though they may not always
	 * correspond to a coprocessor.
	 */
	movi	a2, 0xFF	 /* enable *all* bits, to allow dynamic TIE */
	wsr	a2, CPENABLE
# endif

	/*
	 * Floating point coprocessor option initialization (at least
	 * rounding mode, so that floating point ops give predictable results)
	 */
# if XCHAL_HAVE_FP && !XCHAL_HAVE_VECTORFPU2005
/* floating-point control register (user register number) */
#  define FCR	232
/* floating-point status register (user register number) */
#  define FSR	233
	/* wait for WSR to CPENABLE to complete before accessing FP coproc
	 * state
	 */
	rsync
	wur	a0, FCR	/* clear FCR (default rounding mode, round-nearest) */
	wur	a0, FSR	/* clear FSR */
# endif
#endif /* pre-LX2 */


	/*
	 *  Initialize memory error handler address.
	 *  Putting this address in a register allows multiple instances of
	 *  the same configured core (with separate program images but shared
	 *  code memory, thus forcing memory error vector to be shared given
	 *  it is not VECBASE relative) to have the same memory error vector,
	 *  yet each have their own handler and associated data save area.
	 */
#ifndef XCHAL_HAVE_XEA3
#if XCHAL_HAVE_MEM_ECC_PARITY
	movi	a4, _MemErrorHandler
	wsr	a4, MESAVE
#endif
#endif

	/*
	 *  Initialize medium and high priority interrupt dispatchers:
	 */
#if HAVE_XSR

/*  For asm macros; works for positive a,b smaller than 1000:  */
# define GREATERTHAN(a,b)	(((b)-(a)) & ~0xFFF)

# ifndef XCHAL_DEBUGLEVEL		/* debug option not selected? */
#  define XCHAL_DEBUGLEVEL	99	/* bogus value outside 2..6 */
# endif

	.macro	init_vector	level
	  .if GREATERTHAN(XCHAL_NUM_INTLEVELS+1,\level)
	    .if XCHAL_DEBUGLEVEL-\level
	      .weak   _Level&level&FromVector
	      movi    a4, _Level&level&FromVector
	      wsr     a4, EXCSAVE+\level
	      .if GREATERTHAN(\level,XCHAL_EXCM_LEVEL)
		movi    a5, _Pri_&level&_HandlerAddress
		s32i    a4, a5, 0
		/*  If user provides their own handler, that handler might
		 *  not provide its own _Pri_<n>_HandlerAddress variable for
		 *  linking handlers.  In that case, the reference below
		 *  would pull in the XTOS handler anyway, causing a conflict.
		 *  To avoid that, provide a weak version of it here:
		 */
		.pushsection .data, "aw"
		.global  _Pri_&level&_HandlerAddress
		.weak   _Pri_&level&_HandlerAddress
		.align	4
		_Pri_&level&_HandlerAddress: .space 4
		.popsection
	      .endif
	    .endif
	  .endif
	.endm

	init_vector	2
	init_vector	3
	init_vector	4
	init_vector	5
	init_vector	6

#endif /*HAVE_XSR*/


	/*
	 *  Complete reset initialization outside the vector, to avoid
	 *  requiring a vector that is larger than necessary.  This 2nd-stage
	 *  startup code sets up the C Run-Time (CRT) and calls main().
	 *
	 *  Here we use call0 not because we expect any return, but because the
	 *  assembler/linker dynamically sizes call0 as needed (with
	 *  -mlongcalls) which it doesn't with j or jx.  Note:  This needs to
	 *  be call0 regardless of the selected ABI.
	 */
	call0	_start		 /* jump to _start (in crt1-*.S) */
	/* does not return */

#else /* XCHAL_HAVE_HALT */

	j	_start	/* jump to _start (in crt1-*.S) */
			/* (TX has max 64kB IRAM, so J always in range) */

	 /* Paranoia -- double-check requirements / assumptions of this Xtensa
	  * TX code:
	  */
# if !defined(__XTENSA_CALL0_ABI__) || !XCHAL_HAVE_FULL_RESET \
		|| XCHAL_HAVE_INTERRUPTS || XCHAL_HAVE_CCOUNT \
		|| XCHAL_DTLB_ARF_WAYS || XCHAL_HAVE_DEBUG \
		|| XCHAL_HAVE_S32C1I || XCHAL_HAVE_ABSOLUTE_LITERALS \
		|| XCHAL_DCACHE_SIZE || XCHAL_ICACHE_SIZE || XCHAL_HAVE_PIF \
		|| XCHAL_HAVE_WINDOWED
#  error "Halt architecture (Xtensa TX) requires: call0 ABI, all flops reset, no exceptions or interrupts, no TLBs, no debug, no S32C1I, no LITBASE, no cache, no PIF, no windowed regs"
# endif

#endif /* XCHAL_HAVE_HALT */


#if (!XCHAL_HAVE_HALT || defined(XTOS_UNPACK)) && XCHAL_HAVE_IMEM_LOADSTORE
	.size	_ResetHandler, . - _ResetHandler
#else
	.size	__start, . - __start
#endif

	.text
	.global xthals_hw_configid0, xthals_hw_configid1
	.global xthals_release_major, xthals_release_minor
	.end	literal_prefix

#else /*XEA3 : XOS assembly code */
#if XCHAL_HAVE_MPU
#include <xtensa/mpuasm.h>
#endif


# if XCHAL_HAVE_CACHEADRDIS
#  define IFCAD
# else
#  define IFCAD #
# endif

//-----------------------------------------------------------------------------
// Reset Vector.
//-----------------------------------------------------------------------------

	.section	.ResetVector.text, "ax"
	.global	_ResetVector
//_ResetVector:
	//j	_ResetHandler
	//l32r a0, _ResetHandler  // Load the address of _ResetHandler
    //jx a0   
_ResetVector:	J 1f
	.align	4
.Lhand:	.word	_ResetHandler
1:	L32R	a0, .Lhand
	JX	a0	

//-----------------------------------------------------------------------------
// Reset Handler.
//
// IMPORTANT: this MUST be in J instruction range of .ResetVector.text .
// Otherwise the vector needs to trampoline, like this, or similar:
//	_ResetVector:	J 1f
//		.align	4
//	.Lhand:	.word	_ResetHandler
//	1:	L32R	a0, .Lhand
//		JX	a0
//-----------------------------------------------------------------------------

	.section	.ResetHandler.text, "ax"
	.literal_position		// literals go here

	.extern		__memctl_default
	.extern		__memctl_default_post
	.extern		__prefctl_default
	.weakref	_interrupt_table, _xt_interrupt_table

	//  For MPU empty background map -- see XCHAL_HAVE_MPU code further below.
	//  Cannot put this in .rodata (not unpacked before MPU init).
#if XCHAL_HAVE_MPU && XCHAL_MPU_ENTRIES >= 8 && XCHAL_MPU_BACKGROUND_ENTRIES <= 2
	.global _xtos_mpu_attribs
	.align 4
_xtos_mpu_attribs:
	.word   0x00006000+XCHAL_MPU_ENTRIES-8	// Illegal	(---)
	.word   0x000F7700+XCHAL_MPU_ENTRIES-8	// Writeback	(rwx Cacheable Non-shareable wb rd-alloc wr-alloc)
	.word   0x000D5700+XCHAL_MPU_ENTRIES-8	// WBNA		(rwx Cacheable Non-shareable wb rd-alloc)
	.word   0x000C4700+XCHAL_MPU_ENTRIES-8	// Writethru	(rwx Cacheable Non-shareable wt rd-alloc)
	.word   0x00006700+XCHAL_MPU_ENTRIES-8	// Bypass	(rwx Device non-interruptible system-shareable)
#endif


	.align	4
        .global	_ResetHandler
_ResetHandler:

	/*
	 *  Even if the processor supports the non-PC-relative L32R option,
	 *  it will always start up in PC-relative mode.  We take advantage of
	 *  this, and use PC-relative mode at least until we're sure the .lit4
	 *  section is in place (which is sometimes only after unpacking).
	 */
	.begin	no-absolute-literals

	/* There are times when we need some special-purpose code to run
	 * very early, e.g. a power-on self-test. Such code is usually
	 * supplied by the developer. We do not want to force the developer
	 * to recompile this file (or the XTOS library) so a user hook is
	 * provided for here. If a user hook is found it is called, and if
	 * the hook function returns here then normal startup is resumed.
	 * The hook function could of course take over the entire boot
	 * process and never return here.
	 *
	 * IMPORTANT: Any user hook applied here must make NO assumptions
	 * about the state of the core or the system. Notice that no core
	 * init has been done at this point.
	 */

	.weak	__reset_hook		// optional user hook

	movi	a2, __reset_hook
	beqz	a2, 1f			// no user hook
	callx0	a2			// execute user hook
1:

#if XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_DCACHE_DYN_WAYS
	// If we have dynamic cache way support, init the caches as soon
	// as we can.  (PSO wakeup, however, needs to be processed first.)
	// Enable and invalidate all ways of both caches. If there is no
	// dynamic way support then this write will have no effect.

	movi	a0, __memctl_default
	wsr.memctl	a0
#endif

	// Keep zero in a0, used to initialize lots of things.
	movi	a0, 0

#if XCHAL_HAVE_L2_CACHE
	.extern _Xthal_L2_ctrl_init

	//  For single-image multicore, only core 0 will do L2 init.
#if XCHAL_HAVE_PRID
	xt_core_id a3
	bnez	a3, 1f
#endif
	movi	a5, XCHAL_L2_REGS_PADDR		// L2 controller base address
	movi	a3, _Xthal_L2_ctrl_init		// software-configured L2 setup (with TAGINIT set)
	s32i	a3, a5, L2CC_REG_RAM_CTRL_INIT
1:
#endif

#if XCHAL_HAVE_INTERRUPTS
	// Use the address of the interrupt handler table as the ISB/ITB base.
	// Handler table must be named exactly the same for non-XTOS applications
	// as well, for this to work. Else, set properly in user-supplied code.
# if XCHAL_HAVE_ITB
	movi	a4, _interrupt_table
	wsr.itb	a4
# elif XCHAL_HAVE_ISB
	movi	a4, _interrupt_table
	wsr.isb	a4
# endif
#endif

#if XCHAL_HAVE_VECBASE
	// Vectors are relocatable, set up vecbase to point to dispatch code.
#ifndef XCHAL_HAVE_XEA3
	movi	a2, _DispatchVector		// Start of dispatch code
#endif
	wsr.vecbase	a2
#endif

	// If either of the caches does not have dynamic way support, then
	// use the old (slow) method to init them. If the cache is absent
	// the macros will expand to empty.

#if !(XCHAL_HAVE_ICACHE_DYN_WAYS || XCHAL_HAVE_CME_DOWNGRADES)
	icache_reset	a2, a3
#endif

#if XCHAL_HAVE_DCACHE_DYN_WAYS || XCHAL_HAVE_CME_DOWNGRADES
	dcache_reset_data	a2, a3, a4
#else
	dcache_reset	a2, a3, a4 
#endif

#if XCHAL_HAVE_L2_CACHE
        //  For multicore, only core 0 will initialize L2
#if XCHAL_HAVE_PRID
	xt_core_id a3
	bnez	a3, 2f
#endif

#if ! XCHAL_DCACHE_IS_COHERENT
	// If the core is not coherent and the L2 cache size is 0
	// Then we can skip waiting for completion
	movi	a4, Xthal_L2cache_size
	l32i	a4, a4, 0
	beqz	a4, 2f
#endif
	//  Wait for L2 cache initialization to complete.
	//  If coherent then both the L2 Cache and RAM tags
	//  are initialized
	movi	a4, XCHAL_L2_REGS_PADDR
1:
	memw
	l32i	a3, a4, L2CC_REG_STATUS			// init done ?
	bbci.l	a3, L2CC_ST_INIT_DONE_SHIFT, 1b		// busy wait until done
#if XCHAL_DCACHE_IS_COHERENT
	movi	a3, ((1 << XCHAL_L2CC_NUM_CORES)-1) << 8 // enable coherence for all cores
	s32i	a3, a4, L2CC_REG_COHERENCE_CTRL // Store to L2 Coherence Control Register
#endif
2:
#endif

#if (XCHAL_HAVE_ICACHE_DYN_ENABLE && ! XCHAL_HAVE_ICACHE_DYN_WAYS) || \
    (XCHAL_HAVE_DCACHE_DYN_ENABLE && ! XCHAL_HAVE_DCACHE_DYN_WAYS)
	// Caches need to be both initialized and enabled.  Now enable them.
	movi	a2, __memctl_default
	wsr.memctl	a2
#endif

#if XCHAL_HAVE_PREFETCH
	/* Set up PREFCTL register. By default enables cache prefetch. */
	movi	a2, __prefctl_default
	wsr.prefctl	a2
#endif

#if XCHAL_HAVE_WINDOWED 
# ifdef __XTENSA_CALL0_ABI__
	// If we have windowing support in hw, then WB at reset will
	// have WB.S set to '01'. Disable windowing if ABI is call0.
	wsr.wb  a0
# else
	// Still set initial value in case it was a soft reset.
	movi	a2, 1
	slli	a2, a2, 30
	wsr.wb	a2
# endif
	rsync
	movi	a0, 0				// possibly a different a0, clear it
#endif

	/*
	 *  Now setup the memory attributes.  On some cores this "enables" caches.
	 *  We do this ahead of unpacking, so it can proceed more efficiently.
	 *
	 *  The _memmap_cacheattr_reset symbol's value (address) is defined
	 *  by the LSP's linker script, as generated by xt-genldscripts.
	 *  If defines 4-bit attributes for eight 512MB regions.
	 *
	 *  (NOTE:  for cores with the older MMU v1 or v2, or without any memory
	 *   protection mechanism, the following code has no effect.)
	 */
#if XCHAL_HAVE_MPU
	// If there is a user-provided MPU table, then we will program the MPU
	// with it now. Can't call xthal_write_map_raw() because code sections
	// haven't been unpacked yet. For romable images, the MPU table values
	// and the table size must reside in a section that does not need to be
	// unpacked (.ResetHandler.text or .srom.text).
	// NOTE: This will set CACHEADRDIS to all zeros, because computing a
	// useful nonzero value from the user settings is too complex and slow
	// to implement here.

	.weak	__xt_mpu_init_table		// Table of MPU entries
	.weak	__xt_mpu_init_table_size	// Number of entries in table
IFCAD	.weak	__xt_mpu_init_cacheadrdis	// value for cacheadrdis

	movi	a2, __xt_mpu_init_table		// non-zero if user defined
	movi	a3, __xt_mpu_init_table_size	// non-zero if user defined
IFCAD	movi	a4, __xt_mpu_init_cacheadrdis	// non-zero if user defined
	beqz	a2, .Lno_user_mpu
	beqz	a3, .Lno_user_mpu
	l32i	a3, a3, 0
	beqz	a3, .Lno_user_mpu		// skip if size = 0
	mpu_write_map	a2, a3, a9, a10, a11, a12, a13, a14, a15
IFCAD	beqz	a4, .Lno_default_mpu
IFCAD	l32i	a4, a4, 0
IFCAD	wsr.cacheadrdis	a4
	j	.Lno_default_mpu

.Lno_user_mpu:
	// If there's an empty background map, setup foreground maps to mimic
	// region protection.


# if XCHAL_MPU_ENTRIES >= 8 && XCHAL_MPU_BACKGROUND_ENTRIES <= 2
	// We assume reset state:  all MPU entries zeroed and disabled.
	// Otherwise we'd need a loop to zero everything.

	movi	a2, _memmap_cacheattr_reset	// note: absolute symbol, not a ptr
	movi	a3, _xtos_mpu_attribs		// see literal area at start of reset vector
	movi	a4, 0x20000000			// 512 MB delta
	movi	a6, 8
	movi	a7, 1				// MPU entry vaddr 0, with valid bit set
IFCAD	movi	a9, 0				// cacheadrdis value
IFCAD	wsr.cacheadrdis a9			// enable everything temporarily while MPU updates

        // Write eight MPU entries, from the last one going backwards (entries n-1 thru n-8)
	//
2:	extui	a8, a2, 28, 4			// get next attribute nibble (msb first)
IFCAD	extui	a5, a8, 0, 2			// lower two bit indicate whether cached
IFCAD	slli	a9, a9, 1			// add a bit to cacheadrdis...
IFCAD	addi	a10, a9, 1			// set that new bit if...
IFCAD	moveqz	a9, a10, a5			// ... that region is non-cacheable
	addx4	a5, a8, a3			// index into _xtos_mpu_attribs table
	addi	a8, a8, -5			// make valid attrib indices negative
	movgez	a5, a3, a8			// if not valid attrib, use Illegal
	l32i	a5, a5, 0			// load access rights, memtype from table entry
	slli	a2, a2, 4
	sub	a7, a7, a4			// next 512MB region (last to first)
	addi	a6, a6, -1
	add	a5, a5, a6			// add the index
	wptlb	a5, a7				// write the MPU entry
	bnez	a6, 2b				// loop until done
# else
IFCAD	movi	a9, XCHAL_MPU_BG_CACHEADRDIS	// default value of CACHEADRDIS for bgnd map
# endif
IFCAD	wsr.cacheadrdis a9			// update cacheadrdis
.Lno_default_mpu:
#elif XCHAL_HAVE_CACHEATTR || XCHAL_HAVE_MIMIC_CACHEATTR || XCHAL_HAVE_XLT_CACHEATTR \
		|| (XCHAL_HAVE_PTP_MMU && XCHAL_HAVE_SPANNING_WAY)
	movi	a2, _memmap_cacheattr_reset	/* note: absolute symbol, not a ptr */
	cacheattr_set				/* set CACHEATTR from a2 (clobbers a3-a8) */
#endif

	/*  Now that caches are initialized, cache coherency can be enabled.
	only enable coherence in memctrl register here ... L2 is done separately */
	cache_coherence_on_memctl a2, a3

	/*  Post-init MEMCTL setup. By default this includes snoop enable, loop    */
	/*  buffer enable and branch prediction enable. Can be overridden by user. */
#if XCHAL_USE_MEMCTL
	rsr.memctl	a2
	movi	a3, __memctl_default_post
	or	a2, a2, a3
	wsr.memctl	a2
#endif

	/*
	 * At this point we can unpack code and data (e.g. copy segments from
	 * ROM to RAM, vectors into their proper location, etc.). However,
	 *
	 * 1) the destination of the unpack may require some setup,
	 *    for instance a DDR controller may need to be initialized
	 *    and enabled before anything is unpacked into DDR.
	 * 2) users may wish to provide their own unpack code which works
	 *    faster or in a different way than the default unpack code.
	 *
	 * To support such uses, we provide a user hook at this point.
	 * If the user hook function is defined, then it is called from
	 * here, and its return value (in a2) is checked. If the return
	 * value is non-zero, then we assume that code unpacking has been
	 * completed. The user hook function must be written in assembly
	 * and should make minimal assumptions about system state.
	 */

	.weak	__reset_user_init

	movi	a2, __reset_user_init
	beqz	a2, 1f			// no user hook
	callx0	a2			// execute user hook
	movi	a0, 0			// ensure a0 continues to hold 0
	bnez	a2, unpackdone		// if a2 != 0 then unpack is done
1:

#if defined(XTOS_UNPACK)
	movi	a2, _rom_store_table
	beqz	a2, unpackdone
unpack:	l32i	a3, a2, 0	// start vaddr
	l32i	a4, a2, 4	// end vaddr
	l32i	a5, a2, 8	// store vaddr
	addi	a2, a2, 12
	bgeu	a3, a4, upnext	// skip unless start < end
uploop:	l32i 	a6, a5, 0
	addi	a5, a5, 4
	s32i	a6, a3, 0
	addi	a3, a3, 4
	bltu	a3, a4, uploop
	j	unpack
upnext:	bnez	a3, unpack
	bnez	a5, unpack
#endif /* XTOS_UNPACK */

unpackdone:

#if defined(XTOS_UNPACK) || defined(XTOS_MP)
	/*
	 *  If writeback caches are configured and enabled, unpacked data must be
	 *  written out to memory before trying to execute it:
	 */
	dcache_writeback_all	a2, a3, a4, 0
	icache_sync		a2	// ensure data written back is visible to i-fetch
	/*
	 *  Note:  no need to invalidate the i-cache after the above, because we
	 *  already invalidated it further above and did not execute anything within
	 *  unpacked regions afterwards.  [Strictly speaking, if an unpacked region
	 *  follows this code very closely, it's possible for cache-ahead to have
	 *  cached a bit of that unpacked region, so in the future we may need to
	 *  invalidate the entire i-cache here again anyway.]
	 */
#endif

	.end	no-absolute-literals		// we can now start using absolute literals

	// Set MS.DISPST to Normal, clear other MS fields

	wsr.ms  a0
	rsync

	/*
	 *  Complete reset initialization outside the vector,
	 *  to avoid requiring a vector that is larger than necessary.
	 *  This 2nd-stage startup code sets up the C Run-Time (CRT) and calls main().
	 *  
	 *  Here we use call0 not because we expect any return, but
	 *  because the assembler/linker dynamically sizes call0 as
	 *  needed (with -mlongcalls) which it doesn't with j or jx.
	 *  Note:  This needs to be call0 regardless of the selected ABI.
	 */
	call0	_start		// jump to _start (in crt1-*.S)
	/* does not return */


	.size	_ResetHandler, . - _ResetHandler

	.text
	.global xthals_hw_configid0, xthals_hw_configid1
	.global xthals_release_major, xthals_release_minor


#endif
