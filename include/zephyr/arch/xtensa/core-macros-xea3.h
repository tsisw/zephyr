/*
 * xtensa/core-macros.h -- C specific definitions
 *                         that depend on CORE configuration
 */

/*
 * Copyright (c) 2012-2021 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CORE_MACROS_XEA3_H
#define CORE_MACROS_XEA3_H

#ifndef CORE_MACROS_H
#error "Do not include this file directly. Include <xtensa/core-macros.h>."
#endif

#if XCHAL_HAVE_LX || XCHAL_HAVE_XEA2 || XCHAL_HAVE_XEA5 || !XCHAL_HAVE_XEA3 || XCHAL_HAVE_CACHE_BLOCK
#error "Unsupported processor configuration"
#endif

/* NOTE: MISRA Rule 8.13 "A pointer should point to a const-qualified type whenever
 possible" is triggered many times in this file. However, many of these functions
 take a pointer and may modify memory contents at that address (e.g. by datacache
 invalidation). So they are not actually pointing to const types. */
/* parasoft-begin-suppress MISRA2012-RULE-8_13_a "MISRA Rule 8.13 disabled for this file." */

/* parasoft-begin-suppress MISRA2012-RULE-20_7 "MISRA Rule 20.7 disabled for this file." */

#include <xtensa/xtensa-types.h>
#include <xtensa/config/core.h>
#include <xtensa/corebits.h>
#include <xtensa/tie/xt_core.h>

#if (XCHAL_DCACHE_SIZE > 0)
#include <xtensa/tie/xt_datacache.h>
#endif
#if XCHAL_HAVE_INTERRUPTS
#include <xtensa/tie/xt_interrupt.h>
#endif
#if XCHAL_HAVE_CCOUNT
#include <xtensa/tie/xt_timer.h>
#endif
#if XCHAL_HAVE_EXTERN_REGS
#include <xtensa/tie/xt_externalregisters.h>
#endif
#if XCHAL_HAVE_CP
#include <xtensa/tie/xt_coprocessors.h>
#endif

#include <xtensa/intctrl.h>
#if XCHAL_HAVE_L2
#include <xtensa/L2-cc-regs.h>
#endif

/*  Only define things for C code.  */
#if !defined(_ASMLANGUAGE) && !defined(_NOCLANGUAGE) && !defined(__ASSEMBLER__)

/***************************   CACHE   ***************************/

/* parasoft-begin-suppress MISRA2012-DIR-4_4 "Not code". */
/* 
 *
 *	void xthal_icache_line_invalidate(void *addr);
 *	void xthal_icache_line_lock(const void *addr);
 *	void xthal_icache_line_unlock(const void *addr);
 *	void xthal_icache_sync(void);
 *
 *      void xthal_dcache_line_invalidate(void *addr);
 *	void xthal_dcache_line_writeback(const void *addr);
 *	void xthal_dcache_line_writeback_inv(const void *addr);
 *	void xthal_dcache_line_lock(const void *addr);
 *	void xthal_dcache_line_unlock(const void *addr);
 *	void xthal_dcache_sync(void);
 *	void xthal_dcache_line_prefetch_for_write(void *addr);
 *	void xthal_dcache_line_prefetch_for_read(void *addr);
 *
 *
 * All block downgrade functions exist in two forms: with and without
 * the 'max' parameter: This parameter allows compiler to optimize
 * the functions whenever the parameter is smaller than the cache size.
 *
 *	xthal_dcache_block_invalidate(void *addr, uint32_t size);
 *	xthal_dcache_block_writeback(void *addr, uint32_t size);
 *	xthal_dcache_block_writeback_inv(void *addr, uint32_t size);
 *	xthal_dcache_block_invalidate_max(void *addr, uint32_t size, uint32_t max);
 *	xthal_dcache_block_writeback_max(void *addr, uint32_t size, uint32_t max);
 *	xthal_dcache_block_writeback_inv_max(void *addr, uint32_t size, uint32_t max);
 *
 *      xthal_dcache_block_prefetch_for_read(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_for_write(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_modify(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_read_write(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_for_read_grp(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_for_write_grp(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_modify_grp(void *addr, uint32_t size);
 *      xthal_dcache_block_prefetch_read_write_grp(void *addr, uint32_t size)
 *
 *	xthal_dcache_block_wait();
 *	xthal_dcache_block_required_wait();
 *	xthal_dcache_block_abort();
 *	xthal_dcache_block_prefetch_end();
 *	xthal_dcache_block_newgrp();
 */
/* parasoft-end-suppress MISRA2012-DIR-4_4 "Not code". */

/***   INSTRUCTION CACHE   ***/

/*
 * Even if a config doesn't have caches, an isync is still needed
 * when instructions in any memory are modified, whether by a loader
 * or self-modifying code.  Therefore, this macro always produces
 * an isync, whether or not an icache is present.
 */
XT_INLINE void xthal_icache_sync(void)
{
    /* flush instruction prefetch pipeline */
    __asm__ __volatile__("isync":::"memory");
}

XT_INLINE void xthal_icache_line_invalidate(void* addr)
{
#if XCHAL_ICACHE_SIZE > 0
    /* invalidate instruction cache at addr */
    __asm__ __volatile__("ihi %0, 0" :: "a"(addr) : "memory");
    /* flush instruction prefetch pipeline */
    xthal_icache_sync();
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_icache_line_lock(const void* addr)
{
#if XCHAL_ICACHE_SIZE > 0 && XCHAL_ICACHE_LINE_LOCKABLE
    /* lock instruction cache line */
    __asm__ __volatile__("ipfl %0, 0" :: "a"(addr) : "memory");
    xthal_icache_sync();
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_icache_line_unlock(const void* addr)
{
#if XCHAL_ICACHE_SIZE > 0 && XCHAL_ICACHE_LINE_LOCKABLE
    /* unlock instruction cache line */
    __asm__ __volatile__("ihu %0, 0" :: "a"(addr) : "memory");
    xthal_icache_sync();
#else
    UNUSED (addr);
#endif
}

/***   DATA CACHE   ***/

#if !XCHAL_HAVE_L2_CACHE /* No L2 */

XT_INLINE void xthal_dcache_line_invalidate(void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* invalidate data cache line */
    __asm__ __volatile__("dhi %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_dcache_line_writeback(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* flush data cache line to main memory */
    __asm__ __volatile__("dhwb %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_dcache_line_writeback_inv(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* flush data cache line to main memory and mark cache line as invalid */
    __asm__ __volatile__("dhwbi %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

#endif /* !XCHAL_HAVE_L2_CACHE */

XT_INLINE void xthal_dcache_line_lock(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_DCACHE_LINE_LOCKABLE
    /* lock line into datache */
    __asm__ __volatile__("dpfl %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED(addr);
#endif
}

XT_INLINE void xthal_dcache_line_unlock(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_DCACHE_LINE_LOCKABLE
    /* unlock line from data cache */
    __asm__ __volatile__("dhu %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED(addr);
#endif
}

XT_INLINE void xthal_dcache_line_prefetch_for_read(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    XT_DPFR(xthal_cvt_const_voidp_to_int32p(addr), 0);
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_dcache_line_prefetch_for_write(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_DCACHE_IS_WRITEBACK
    /* data cache prefetch for write */
    XT_DPFW(xthal_cvt_const_voidp_to_int32p(addr), 0);
#else
    UNUSED (addr);
#endif
}

/*****  L1 Only datacache operations *****/

XT_INLINE void xthal_dcache_L1_line_invalidate(void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* invalidate data cache line */
    __asm__ __volatile__("dhi %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_dcache_L1_line_writeback(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* flush data cache line to main memory */
    __asm__ __volatile__("dhwb %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

XT_INLINE void xthal_dcache_L1_line_writeback_inv(const void* addr)
{
#if (XCHAL_DCACHE_SIZE > 0)
    /* flush data cache line to main memory and mark cache line as invalid */
    __asm__ __volatile__("dhwbi %0, 0" :: "a"(addr) : "memory");
#else
    UNUSED (addr);
#endif
}

/*****   Block Operations   *****/

XT_INLINE void xthal_dcache_block_invalidate(void* addr, uint32_t size)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    XT_DHI_B(xthal_cvt_voidp_to_int32p(addr), (int32_t) size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    xthal_dcache_region_invalidate(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
#endif
}

XT_INLINE void xthal_dcache_block_invalidate_max(void* addr, uint32_t size,
        uint32_t max)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    UNUSED (max);
    xthal_dcache_block_invalidate(addr, size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    UNUSED(max);
    xthal_dcache_region_invalidate(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
    UNUSED (max);
#endif
}

XT_INLINE void xthal_dcache_block_writeback(void* addr, uint32_t size)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    XT_DHWB_B(xthal_cvt_voidp_to_int32p(addr), (int32_t) size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    xthal_dcache_region_writeback(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
#endif
}

XT_INLINE void xthal_dcache_block_writeback_max(void* addr, uint32_t size,
        uint32_t max)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    UNUSED (max);
    xthal_dcache_block_writeback(addr, size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    UNUSED(max);
    xthal_dcache_region_writeback(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
    UNUSED (max);
#endif
}

XT_INLINE void xthal_dcache_block_writeback_inv(void* addr, uint32_t size)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    XT_DHWBI_B(xthal_cvt_voidp_to_int32p(addr), (int32_t) size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    xthal_dcache_region_writeback_inv(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
#endif
}

XT_INLINE void xthal_dcache_block_writeback_inv_max(void* addr, uint32_t size,
        uint32_t max)
{
#if (XCHAL_DCACHE_SIZE > 0) && XCHAL_HAVE_CME_DOWNGRADES
    UNUSED (max);
    xthal_dcache_block_writeback_inv(addr, size);
#elif (XCHAL_DCACHE_SIZE > 0)
    // If there is no cache engine available, we do the downgrade using
    // the normal instructions.
    UNUSED(max);
    xthal_dcache_region_writeback_inv(addr, size);
#else
    UNUSED (addr);
    UNUSED (size);
    UNUSED (max);
#endif
}

XT_INLINE void xthal_dcache_block_prefetch_read_write(void* addr, uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_read_write_grp(void* addr,
        uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_for_read(void* addr, uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_for_read_grp(void* addr,
        uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_for_write(void* addr, uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_modify(void* addr, uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_for_write_grp(void* addr,
        uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

XT_INLINE void xthal_dcache_block_prefetch_modify_grp(void* addr, uint32_t size)
{
    UNUSED(addr);
    UNUSED(size);
}

/* abort all or end optional block cache operations */
XT_INLINE void xthal_dcache_block_abort(void)
{
#if (XCHAL_DCACHE_SIZE > 0) && (XCHAL_HAVE_CME_DOWNGRADES)
    XT_PFEND_A ();
#endif
}

XT_INLINE void xthal_dcache_block_end(void)
{
#if (XCHAL_DCACHE_SIZE > 0) && (XCHAL_HAVE_CME_DOWNGRADES)
    XT_PFEND_O ();
#endif
}

XT_INLINE void xthal_dcache_block_wait(void)
{
#if (XCHAL_DCACHE_SIZE > 0) && (XCHAL_HAVE_CME_DOWNGRADES)
    XT_PFWAIT_A ();
    XT_MEMW();
#endif
}

XT_INLINE void xthal_dcache_block_required_wait(void)
{
#if (XCHAL_DCACHE_SIZE > 0) &&   (XCHAL_HAVE_CME_DOWNGRADES)
    XT_PFWAIT_R ();
    XT_MEMW();
#endif
}

XT_INLINE void xthal_dcache_block_newgrp(void)
{
}

/***************************   INTRINSICS   ***************************/

#if XCHAL_HAVE_EXTERN_REGS

XT_INLINE uint32_t XTHAL_RER(uint32_t reg) // Not supportable on RNX, not documented, but used internally. (Deprecate?)
{
    return XT_RER(reg);
}

XT_INLINE void XTHAL_WER(uint32_t reg, uint32_t val) // Not supportable on RNX, not documented, but used internally. (Deprecate?)
{
    /* NOTE That the argument order is reversed! */
    XT_WER(val, reg);
}

#endif /* XCHAL_HAVE_EXTERN_REGS */

/***************************  MPU   ***************************/

/*
 * Reads and returns the MPU entry at the index specified.
 * If the index is invalid or the storage pointer is NULL,
 * returns -1 indicating an error. On success returns 0.
 */
XT_INLINE int32_t xthal_mpu_get_entry(xthal_MPU_entry * entry, uint32_t index)
{
#if XCHAL_HAVE_MPU
    if ((entry == NULL) || (index >= XCHAL_MPU_ENTRIES)) {
        return -1;
    }
    __asm__ __volatile__("rptlb0 %0, %1\n" : "=a" (entry->as) : "a" (index));
    __asm__ __volatile__("rptlb1 %0, %1\n" : "=a" (entry->at) : "a" (index));
    return 0;
#else
    UNUSED(entry);
    UNUSED(index);
    return -1;
#endif
}

/*
 * Sets a single entry at 'index' within the MPU
 *
 * The caller must ensure that the resulting MPU map is ordered.
 */
XT_INLINE void xthal_mpu_set_entry(xthal_MPU_entry entry)
{
#if XCHAL_HAVE_MPU
    /* update mpu entry .. requires an memw on same cache line */
    __asm__ __volatile__("j 1f\n\t.align 8\n\t1: memw\n\twptlb %0, %1\n\t" : : "a" (entry.at), "a"(entry.as));
#else
    UNUSED (entry);
#endif
}

/***************************   INTERRUPTS   ***************************/

/*
 * xthal_disable_interrupts
 *
 * Disables all interrupts, returns previous state.
 */
XT_INLINE uint32_t xthal_disable_interrupts(void)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    uint32_t val = PS_DI;
    XT_XPS(val, val);
    return val;
#else
    return 0;
#endif
}

/*
 * xthal_enable_interrupts
 *
 * Enables all interrupts, returns previous state.
 */
XT_INLINE uint32_t xthal_enable_interrupts(void)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    uint32_t val = 0;
    uint32_t mask = PS_DI;
    XT_XPS(val, mask);
    return val;
#else
    return 0;
#endif
}

/*
 * xthal_restore_interrupts
 *
 * Restores interrupts using provided flag, which should be the
 * return value from a previous call to xthal_disable_interrupts().
 */
XT_INLINE void xthal_restore_interrupts(uint32_t flag)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    uint32_t mask = PS_DI;
    XT_XPS(flag, mask);
#else
    UNUSED (flag);
#endif
}

/*
 * xthal_intlevel_get
 *
 * Returns the current interrupt priority level.
 */
XT_INLINE uint32_t xthal_intlevel_get(void)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    return XTHAL_RER(ICREG_CURPRI);
#else
    return 0;
#endif
}

/*
 * xthal_intlevel_set
 *
 * Sets the current interrupt priority level.
 * Returns the previous interrupt priority level.
 */
XT_INLINE uint32_t xthal_intlevel_set(uint32_t level)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    uint32_t olevel = XTHAL_RER(ICREG_CURPRI);
    XTHAL_WER(ICREG_CURPRI, level);
    return olevel;
#else
    UNUSED (level);
    return 0;
#endif
}

/*
 * xthal_intlevel_set_min
 *
 * Sets the current interrupt priority level to 'level' only if the new
 * level is higher than the existing priority level. This function will
 * never lower the interrupt priority level from the existing level.
 * Returns the previous interrupt priority level.
 */
XT_INLINE uint32_t xthal_intlevel_set_min(uint32_t level)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    uint32_t olevel = XTHAL_RER(ICREG_CURPRI);

    if (olevel < level)
    {
        XTHAL_WER(ICREG_CURPRI, level);
    }
    return olevel;
#else
    UNUSED (level);
    return 0;
#endif
}

/*
 *  xthal_interrupt_enable
 *
 *  Enables the specified interrupt.
 */
XT_INLINE void xthal_interrupt_enable(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval = (INTCTRL_ENABLE_WRITE | INTCTRL_ENABLE);

        // Set the 'enable' bit and the 'write-enable' bit for it.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
#endif
}

/*
 *  xthal_interrupt_disable
 *
 *  Disables the specified interrupt.
 */
XT_INLINE void xthal_interrupt_disable(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval = INTCTRL_ENABLE_WRITE;

        // Clear the 'enable' bit and set the 'write-enable' bit.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
#endif
}

/*
 * xthal_interrupt_enabled
 *
 * Returns 1 if specified interrupt is enabled, else 0.
 */
XT_INLINE uint32_t xthal_interrupt_enabled(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        return ((((uint32_t) XTHAL_RER(IC_CTRLREG(intnum))) & INTCTRL_ENABLE)
                != 0U) ? 1U : 0U;
    }
#else
    UNUSED (intnum);
#endif
    return 0;
}

/*
 * xthal_interrupt_trigger
 *
 * Triggers the specified interrupt.
 */
XT_INLINE void xthal_interrupt_trigger(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval = (INTCTRL_PEND_WRITE | INTCTRL_PENDING);

        // Set the 'pending' bit and set the 'write-pending' bit.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
#endif
}

/*
 * xthal_interrupt_clear
 *
 * Clears the specified interrupt.
 */
XT_INLINE void xthal_interrupt_clear(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval = INTCTRL_PEND_WRITE;

        // Clear the 'pending' bit and set the 'write-pending' bit.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
#endif
}

/*
 * xthal_interrupt_pending
 *  
 * Returns nonzero if the specified interrupt is pending.
 */
XT_INLINE uint32_t xthal_interrupt_pending(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        return XTHAL_RER(IC_CTRLREG(intnum)) & INTCTRL_PENDING;
    }
    else
    {
        return 0;
    }
#else
    UNUSED (intnum);
    return 0;
#endif
}

/*
 * xthal_interrupt_type
 *
 * Returns the type of the specified interrupt.
 */
XT_INLINE uint32_t xthal_interrupt_type(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS
    return (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS)) ?
            Xthal_inttype[intnum] : (uint32_t) XTHAL_INTTYPE_UNCONFIGURED;
#else
    UNUSED (intnum);
    return (uint32_t) XTHAL_INTTYPE_UNCONFIGURED;
#endif
}

/*
 * xthal_interrupt_pri_get
 *
 * Returns the priority of the specified interrupt.
 */
XT_INLINE uint32_t xthal_interrupt_pri_get(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        return (((uint32_t) XTHAL_RER(IC_CTRLREG(intnum))) & INTCTRL_PRIO_MASK)
                >> INTCTRL_PRIO_SHIFT;
    }
#else
    UNUSED (intnum);
#endif
    return 0;
}

/*
 * xthal_interrupt_pri_set
 *
 * Sets the priority of the specified interrupt (XEA3 only).
 */
XT_INLINE void xthal_interrupt_pri_set(uint32_t intnum, uint8_t pri)
{
#pragma no_reorder
#if (XCHAL_HAVE_INTERRUPTS)
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval = INTCTRL_PRIO_WRITE
                | ((uint32_t) pri << INTCTRL_PRIO_SHIFT);

        // Set the 'priority' field and set the 'write-priority' bit.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
    UNUSED (pri);
#endif
}

/*
 * xthal_interrupt_sens_get
 *
 * Returns the sensitivity of the specified interrupt (edge/level).
 * Nonzero - level, zero = edge.
 */
XT_INLINE uint32_t xthal_interrupt_sens_get(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        return XTHAL_RER(IC_CTRLREG(intnum)) & INTCTRL_SENS;
    }
#else
    UNUSED (intnum);
#endif
    return 0;
}

/*
 * xthal_interrupt_sens_set
 *
 * Sets the sensitivity of the specified interrupt (XEA3 only).
 * sens = 0 => edge, sens = 1 => level.
 * NOTE: Changing the sensitivity also clears any pending interrupt.
 */
XT_INLINE void xthal_interrupt_sens_set(uint32_t intnum, uint8_t sens)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(intnum);
        uint32_t rval =
                ((uint32_t) INTCTRL_PEND_WRITE)
                        | ((uint32_t) INTCTRL_SENS_WRITE)
                        | ((uint32_t) ((
                                (sens != UINT8_C(0)) ? UINT32_C(1) : UINT32_C(0))
                                << INTCTRL_SENS_SHIFT));
        // Set the 'sense' field and clear the 'pending' field.
        XTHAL_WER(addr, rval);
    }
#else
    UNUSED (intnum);
    UNUSED (sens);
#endif
}

/*
 * xthal_interrupt_active
 *
 * Returns the active state of the specified interrupt (XEA3 only).
 * Should be used only for testing.
 */
XT_INLINE uint32_t xthal_interrupt_active(uint32_t intnum)
{
#pragma no_reorder
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        return XTHAL_RER(IC_CTRLREG(intnum)) & INTCTRL_ACTIVE;
    }
#else
    UNUSED (intnum);
#endif
    return 0;
}

/*
 * xthal_interrupt_make_nmi
 *
 * Converts the specified interrupt into an NMI and enables it.
 */
XT_INLINE void xthal_interrupt_make_nmi(uint32_t intnum)
{
#if XCHAL_HAVE_INTERRUPTS
    if (intnum < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        xthal_interrupt_pri_set(intnum, XCHAL_NUM_INTLEVELS);
        xthal_interrupt_enable(intnum);
    }
#else
    UNUSED (intnum);
#endif
}

/*
 * xthal_nmi_lock
 *
 * Locks the NMI state in the interrupt controller so that
 * no further changes can be made to NMI interrupts.
 */
XT_INLINE void xthal_nmi_lock(void)
{
#if XCHAL_HAVE_INTERRUPTS
    XTHAL_WER(ICREG_GLOBCTRL, IC_CTRL_NMILOCK);
#endif
}

/* ********* Timer / CCOUNT *********/

XT_INLINE xt_counter_t xthal_get_ccount(void)
{
#if XCHAL_HAVE_CCOUNT
    return XT_RSR_CCOUNT();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_ccount(xt_counter_t ccount)
{
#if XCHAL_HAVE_CCOUNT
    XT_WSR_CCOUNT(ccount);
#else
    UNUSED (ccount);
#endif
}

XT_INLINE xt_counter_t xthal_get_cycle_count(void)
{
#if XCHAL_HAVE_CCOUNT
    return XT_RSR_CCOUNT();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_count(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT
    XT_WSR_CCOUNT(v);
#else
    UNUSED (v);
#endif
}


/***************************   MISC   ***************************/

/*
 *  inline versions of:
 *    xthal_clear_regcached_code
 *    xthal_get_prid
 */

XT_INLINE void xthal_clear_regcached_code(void)
{
#if XCHAL_HAVE_LOOPS
    XT_WSR_LCOUNT(0);
#endif
}

XT_INLINE uint32_t xthal_get_prid(void)
{
#if XCHAL_HAVE_PRID
    return (uint32_t) XT_RSR_PRID();
#else
    return 0;
#endif
}

/* We provide separate HAL functions to get and set each of the CCOMPARE registers (0-2)
 * because it is very desirable to keep the implementation efficient, and in most cases
 * the user will know which the register to use at compile time.
 *
 * Unfortunately it isn't possible to provide a single function that does a compile time
 * selection of the correct instruction to use without using a preprocessor macro (which
 * we are avoiding).
 */
XT_INLINE xt_counter_t xthal_get_ccompare0(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    return XT_RSR_CCOMPARE0();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_ccompare0(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    XT_WSR_CCOMPARE0(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_ccompare1(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 1)
    return XT_RSR_CCOMPARE1();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_ccompare1(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 1)
    XT_WSR_CCOMPARE1(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_ccompare2(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 2)
    return XT_RSR_CCOMPARE2();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_ccompare2(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 2)
    XT_WSR_CCOMPARE2(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_ccompare3(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 3)
    return XT_RSR_CCOMPARE3();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_ccompare3(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 3)
    XT_WSR_CCOMPARE3(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t
xthal_get_cycle_compare(int32_t n)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    switch (n)
    {
        case 0:
            return XT_RSR_CCOMPARE0();
        case 1:
#if XCHAL_NUM_TIMERS > 1
            return XT_RSR_CCOMPARE1();
#endif
        case 2:
#if XCHAL_NUM_TIMERS > 2
            return XT_RSR_CCOMPARE2();
#endif
        case 3:
#if XCHAL_NUM_TIMERS > 3
            return XT_RSR_CCOMPARE3();
#endif
        default:
            /* counter index out of range, return 0 */
            return 0;
    }
#else
    UNUSED(n);
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_compare(int32_t n, xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    switch (n)
    {
        case 0:
            XT_WSR_CCOMPARE0(v);
            break;
        case 1:
#if XCHAL_NUM_TIMERS > 1
            XT_WSR_CCOMPARE1(v);
#endif
            break;
        case 2:
#if XCHAL_NUM_TIMERS > 2
            XT_WSR_CCOMPARE2(v);
#endif
            break;
        case 3:
#if XCHAL_NUM_TIMERS > 3
            XT_WSR_CCOMPARE3(v);
#endif
            break;
        default:
            /* counter index out of range, do nothing */
            break;
    }
#else
    UNUSED(v);
    UNUSED(n);
#endif
}

XT_INLINE xt_counter_t xthal_get_cycle_compare0(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    return XT_RSR_CCOMPARE0();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_compare0(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 0)
    XT_WSR_CCOMPARE0(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_cycle_compare1(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 1)
    return XT_RSR_CCOMPARE1();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_compare1(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 1)
    XT_WSR_CCOMPARE1(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_cycle_compare2(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 2)
    return XT_RSR_CCOMPARE2();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_compare2(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 2)
    XT_WSR_CCOMPARE2(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_cycle_compare3(void)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 3)
    return XT_RSR_CCOMPARE3();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cycle_compare3(xt_counter_t v)
{
#if XCHAL_HAVE_CCOUNT && (XCHAL_NUM_TIMERS > 3)
    XT_WSR_CCOMPARE3(v);
#else
    UNUSED(v);
#endif
}

XT_INLINE xt_counter_t xthal_get_cpenable(void)
{
#if XCHAL_HAVE_CP
    return XT_RSR_CPENABLE();
#else
    return 0;
#endif
}

XT_INLINE void xthal_set_cpenable(xt_counter_t v)
{
#if XCHAL_HAVE_CP
    XT_WSR_CPENABLE(v);
#else
    UNUSED (v);
#endif
}

#endif /* C code */

/* parasoft-end-suppress MISRA2012-RULE-8_13_a "MISRA Rule 8.13 disabled for this file." */

/* parasoft-end-suppress MISRA2012-RULE-20_7 "MISRA Rule 20.7 disabled for this file." */

#if !XCHAL_HAVE_FUNC_SAFETY
#include <xtensa/core-macros-compat.h>    /* parasoft-suppress MISRA2012-RULE-20_1 "Needed for backward compatibility" */
#endif

#endif /* CORE_MACROS_H */

