/*
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdio.h>
#include <zephyr/arch/xtensa/irq.h>
#include <zephyr/sys/__assert.h>

#include <kernel_arch_func.h>

#include <xtensa_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

/**
 * @internal
 *
 * @brief Set an interrupt's priority
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * The priority is verified if ASSERT_ON is enabled. The maximum number of
 * priority levels is a little complex, as there are some hardware priority
 * levels which are reserved: three for various types of exceptions, and
 * possibly one additional to support zero latency interrupts.
 *
 * Valid values are from 1 to 6. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them.
 * ISR installed with priority 0 interrupts cannot make kernel calls.
 */
void z_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	__ASSERT(prio < XCHAL_EXCM_LEVEL + 1,
		 "invalid priority %d! values must be less than %d\n",
		 prio, XCHAL_EXCM_LEVEL + 1);
	/* TODO: Write code to set priority if this is ever possible on
	 * Xtensa
	 */
}

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifndef CONFIG_MULTI_LEVEL_INTERRUPTS
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	ARG_UNUSED(flags);
	ARG_UNUSED(priority);

	z_isr_install(irq, routine, parameter);
	return irq;
}
#else /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
int z_arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			       void (*routine)(const void *parameter),
			       const void *parameter, uint32_t flags)
{
	return z_soc_irq_connect_dynamic(irq, priority, routine, parameter,
					 flags);
}
#endif /* !CONFIG_MULTI_LEVEL_INTERRUPTS */
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

// #define CLIC_INTIE_OFFSET(b,i)                  ((b) + UINT32_C(0x001) + UINT32_C(4)*(i))
// #define MCLICINTIE(i)                           (CLIC_INTIE_OFFSET(UINT32_C(0x1000),i))//MMIO_M_CLIC_BASE_ADDR)

// static inline uint8_t xthal_mmio_ld8(uint32_t addr)
// {
//   uint8_t rv;
//   __asm__ volatile ("_lb %0,%1,0" : "=r" (rv) : "r" (addr));
//   return rv;
// }


/*
 * xthal_interrupt_enabled
 *
 * Returns 1 if specified interrupt is enabled, else 0.
 */
//static inline unsigned int xthal_interrupt_enabled(uint32_t intnum)
// static inline unsigned int xthal_interrupt_enabled(uint32_t intnum)
// {
//     return xthal_mmio_ld8(MCLICINTIE(intnum));
// }

/* Interrupt controller register addresses (ERI) */

#define	IC_REGBASE			UINT32_C(0x00120000)
#define	INTCTRL_ENABLE			UINT32_C(0x1)
/* Per-interrupt control/status registers */

#define	IC_CTRLBASE			(IC_REGBASE + UINT32_C(0x2000))
#define	IC_CTRLREG(num)			(IC_CTRLBASE + (UINT32_C(4) * (num)))

inline uint32_t XTHAL_RER(uint32_t reg) // Not supportable on RNX, not documented, but used internally. (Deprecate?)
{
    //return XT_RER(reg);
	return (reg);
}

void z_irq_spurious(const void *arg)
{
 #ifndef XCHAL_HAVE_XEA3
	int irqs, ie;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));

	LOG_ERR(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p",
		(void *)irqs, (void *)ie);
 #endif
	xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

int xtensa_irq_is_enabled(unsigned int irq)
{
#ifndef XCHAL_HAVE_XEA3
	uint32_t ie;
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	__asm__ volatile("rsr.intenable_alt %0" : "=r"(ie));
	return (ie & (1 << irq)) != 0U;
#else
	//uint32_t ie;
	//ie = xthal_interrupt_enabled(irq);
	//return (ie & (1 << irq)) != 0U;
	if (irq < ((uint32_t) XCHAL_NUM_INTERRUPTS))
	{
		return ((((uint32_t) XTHAL_RER(IC_CTRLREG(irq))) & INTCTRL_ENABLE)
				!= 0U) ? 1U : 0U;
	}
	return 0;
#endif
}
