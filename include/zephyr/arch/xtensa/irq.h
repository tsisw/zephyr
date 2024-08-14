/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_
#define ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <xtensa/config/core-isa.h>

#define CONFIG_GEN_IRQ_START_VECTOR 0

/**
 * @cond INTERNAL_HIDDEN
 */

/*
 * Call this function to enable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be enabled.
 */
static inline void z_xt_ints_on(unsigned int mask)
{
#ifdef XCHAL_HAVE_INTERRUPTS
	int val;
	__asm__ volatile("rsr.intenable %0" : "=r"(val));
	val |= mask;
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(val));
#endif
}
#define	PS_DI_MASK			0x00000008
#define	PS_DI				PS_DI_MASK

/* Interrupt controller register addresses (ERI) */

#define	IC_REGBASE			UINT32_C(0x00120000)
#define	INTCTRL_ENABLE			UINT32_C(0x1)
#define	INTCTRL_ENABLE_WRITE		UINT32_C(0x00000100)
/* Per-interrupt control/status registers */

#define	IC_CTRLBASE			(IC_REGBASE + UINT32_C(0x2000))
#define	IC_CTRLREG(num)			(IC_CTRLBASE + (UINT32_C(4) * (num)))



/*
 * Call this function to disable the specified interrupts.
 *
 * mask     - Bit mask of interrupts to be disabled.
 */
static inline void z_xt_ints_off(unsigned int mask)
{
#pragma no_reorder
#ifndef CONFIG_XTENSA_TENSILICA_NX
	int val;

	__asm__ volatile("rsr.intenable %0" : "=r"(val));
	val &= ~mask;
	__asm__ volatile("wsr.intenable %0; rsync" : : "r"(val));
#else
	 uint32_t val = PS_DI;
     //XT_XPS(val, val);
     //return val;	
#endif
}
inline void XTHAL_WER(uint32_t reg, uint32_t val) // Not supportable on RNX, not documented, but used internally. (Deprecate?)
{
    /* NOTE That the argument order is reversed! */
    //XT_WER(val, reg);
}

/*
 * Call this function to set the specified (s/w) interrupt.
 */
static inline void z_xt_set_intset(unsigned int arg)
{
#ifndef CONFIG_XTENSA_TENSILICA_NX
	__asm__ volatile("wsr.intset %0; rsync" : : "r"(arg));
#else
	if (arg < ((uint32_t) XCHAL_NUM_INTERRUPTS))
    {
        uint32_t addr = IC_CTRLREG(arg);
        uint32_t rval = (INTCTRL_ENABLE_WRITE | INTCTRL_ENABLE);

        // Set the 'enable' bit and the 'write-enable' bit for it.
        XTHAL_WER(addr, rval);
    }
	//ARG_UNUSED(arg);
#endif
}

/**
 * INTERNAL_HIDDEN @endcond
 */

#ifdef CONFIG_MULTI_LEVEL_INTERRUPTS

/* for _soc_irq_*() */
#include <soc.h>

#ifdef CONFIG_2ND_LEVEL_INTERRUPTS
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
#define CONFIG_NUM_IRQS (XCHAL_NUM_INTERRUPTS +\
			(CONFIG_NUM_2ND_LEVEL_AGGREGATORS +\
			CONFIG_NUM_3RD_LEVEL_AGGREGATORS) *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#else
#define CONFIG_NUM_IRQS (XCHAL_NUM_INTERRUPTS +\
			CONFIG_NUM_2ND_LEVEL_AGGREGATORS *\
			CONFIG_MAX_IRQ_PER_AGGREGATOR)
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
#else
#define CONFIG_NUM_IRQS XCHAL_NUM_INTERRUPTS
#endif /* CONFIG_2ND_LEVEL_INTERRUPTS */

void z_soc_irq_init(void);
void z_soc_irq_enable(unsigned int irq);
void z_soc_irq_disable(unsigned int irq);
int z_soc_irq_is_enabled(unsigned int irq);

#define arch_irq_enable(irq)	z_soc_irq_enable(irq)
#define arch_irq_disable(irq)	z_soc_irq_disable(irq)

#define arch_irq_is_enabled(irq)	z_soc_irq_is_enabled(irq)

#ifdef CONFIG_DYNAMIC_INTERRUPTS
extern int z_soc_irq_connect_dynamic(unsigned int irq, unsigned int priority,
				     void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags);
#endif

#else

#define CONFIG_NUM_IRQS XCHAL_NUM_INTERRUPTS

#define arch_irq_enable(irq)	xtensa_irq_enable(irq)
#define arch_irq_disable(irq)	xtensa_irq_disable(irq)

#define arch_irq_is_enabled(irq)	xtensa_irq_is_enabled(irq)

#endif

/**
 * @brief Enable interrupt on Xtensa core.
 *
 * @param irq Interrupt to be enabled.
 */
static ALWAYS_INLINE void xtensa_irq_enable(uint32_t irq)
{
	z_xt_ints_on(1 << irq);
}

/**
 * @brief Disable interrupt on Xtensa core.
 *
 * @param irq Interrupt to be disabled.
 */
static ALWAYS_INLINE void xtensa_irq_disable(uint32_t irq)
{
	z_xt_ints_off(1 << irq);
}

/** Implementation of @ref arch_irq_lock. */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;

#ifndef CONFIG_XTENSA_TENSILICA_NX
	__asm__ volatile("rsil %0, %1"
			 : "=r"(key) : "i"(XCHAL_EXCM_LEVEL) : "memory");
	return key;
#else
	return 0;
#endif
}

/** Implementation of @ref arch_irq_unlock. */
static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	__asm__ volatile("wsr.ps %0; rsync"
			 :: "r"(key) : "memory");
}

/** Implementation of @ref arch_irq_unlocked. */
static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return (key & 0xf) == 0; /* INTLEVEL field */
}

/**
 * @brief Query if an interrupt is enabled on Xtensa core.
 *
 * @param irq Interrupt to be queried.
 *
 * @return True if interrupt is enabled, false otherwise.
 */
int xtensa_irq_is_enabled(unsigned int irq);

#include <zephyr/irq.h>

#endif /* ZEPHYR_INCLUDE_ARCH_XTENSA_XTENSA_IRQ_H_ */
