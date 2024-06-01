/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32V003_PINCTRL_H__
#define __CH32V003_PINCTRL_H__

#define CH32V003_PINMUX_PORT_PA 0
#define CH32V003_PINMUX_PORT_PC 1
#define CH32V003_PINMUX_PORT_PD 2

/*
 * Defines the starting bit for the remap field. Note that the I2C1 and USART1 fields are not
 * contigious.
 */
#define CH32V003_PINMUX_SPI1_RM    0
#define CH32V003_PINMUX_I2C1_RM    1
#define CH32V003_PINMUX_I2C1_RM1   23
#define CH32V003_PINMUX_USART1_RM  2
#define CH32V003_PINMUX_USART1_RM1 21
#define CH32V003_PINMUX_TIM1_RM    6
#define CH32V003_PINMUX_TIM2_RM    8

/* Port number with 0-2 */
#define CH32V003_PINCTRL_PORT_SHIFT    0
/* Pin number 0-15 */
#define CH32V003_PINCTRL_PIN_SHIFT     2
/* Base remap bit 0-31 */
#define CH32V003_PINCTRL_RM_BASE_SHIFT 6
/* Function remapping ID 0-3 */
#define CH32V003_PINCTRL_RM_SHIFT      11

#define CH32V003_PINMUX_DEFINE(port, pin, rm, remapping)                                           \
	((CH32V003_PINMUX_PORT_##port << CH32V003_PINCTRL_PORT_SHIFT) |                            \
	 (pin << CH32V003_PINCTRL_PIN_SHIFT) |                                                     \
	 (CH32V003_PINMUX_##rm##_RM << CH32V003_PINCTRL_RM_BASE_SHIFT) |                           \
	 (remapping << CH32V003_PINCTRL_RM_SHIFT))

#define TIM1_ETR_PC5_0  CH32V003_PINMUX_DEFINE(PC, 5, TIM1, 0)
#define TIM1_ETR_PC5_1  CH32V003_PINMUX_DEFINE(PC, 5, TIM1, 1)
#define TIM1_ETR_PD4_2  CH32V003_PINMUX_DEFINE(PD, 4, TIM1, 2)
#define TIM1_ETR_PC2_3  CH32V003_PINMUX_DEFINE(PC, 2, TIM1, 3)
#define TIM1_CH1_PD2_0  CH32V003_PINMUX_DEFINE(PD, 2, TIM1, 0)
#define TIM1_CH1_PC6_1  CH32V003_PINMUX_DEFINE(PC, 6, TIM1, 1)
#define TIM1_CH1_PD2_2  CH32V003_PINMUX_DEFINE(PD, 2, TIM1, 2)
#define TIM1_CH1_PC4_3  CH32V003_PINMUX_DEFINE(PC, 4, TIM1, 3)
#define TIM1_CH2_PA1_0  CH32V003_PINMUX_DEFINE(PA, 1, TIM1, 0)
#define TIM1_CH2_PC7_1  CH32V003_PINMUX_DEFINE(PC, 7, TIM1, 1)
#define TIM1_CH2_PA1_2  CH32V003_PINMUX_DEFINE(PA, 1, TIM1, 2)
#define TIM1_CH2_PC7_3  CH32V003_PINMUX_DEFINE(PC, 7, TIM1, 3)
#define TIM1_CH3_PC3_0  CH32V003_PINMUX_DEFINE(PC, 3, TIM1, 0)
#define TIM1_CH3_PC0_1  CH32V003_PINMUX_DEFINE(PC, 0, TIM1, 1)
#define TIM1_CH3_PC3_2  CH32V003_PINMUX_DEFINE(PC, 3, TIM1, 2)
#define TIM1_CH3_PC5_3  CH32V003_PINMUX_DEFINE(PC, 5, TIM1, 3)
#define TIM1_CH4_PC4_0  CH32V003_PINMUX_DEFINE(PC, 4, TIM1, 0)
#define TIM1_CH4_PD3_1  CH32V003_PINMUX_DEFINE(PD, 3, TIM1, 1)
#define TIM1_CH4_PC4_2  CH32V003_PINMUX_DEFINE(PC, 4, TIM1, 2)
#define TIM1_CH4_PD4_3  CH32V003_PINMUX_DEFINE(PD, 4, TIM1, 3)
#define TIM1_BKIN_PC2_0 CH32V003_PINMUX_DEFINE(PC, 2, TIM1, 0)
#define TIM1_BKIN_PC1_1 CH32V003_PINMUX_DEFINE(PC, 1, TIM1, 1)
#define TIM1_BKIN_PC2_2 CH32V003_PINMUX_DEFINE(PC, 2, TIM1, 2)
#define TIM1_BKIN_PC1_3 CH32V003_PINMUX_DEFINE(PC, 1, TIM1, 3)
#define TIM1_CH1N_PD0_0 CH32V003_PINMUX_DEFINE(PD, 0, TIM1, 0)
#define TIM1_CH1N_PC3_1 CH32V003_PINMUX_DEFINE(PC, 3, TIM1, 1)
#define TIM1_CH1N_PD0_2 CH32V003_PINMUX_DEFINE(PD, 0, TIM1, 2)
#define TIM1_CH1N_PC3_3 CH32V003_PINMUX_DEFINE(PC, 3, TIM1, 3)
#define TIM1_CH2N_PA2_0 CH32V003_PINMUX_DEFINE(PA, 2, TIM1, 0)
#define TIM1_CH2N_PC4_1 CH32V003_PINMUX_DEFINE(PC, 4, TIM1, 1)
#define TIM1_CH2N_PA2_2 CH32V003_PINMUX_DEFINE(PA, 2, TIM1, 2)
#define TIM1_CH2N_PD2_3 CH32V003_PINMUX_DEFINE(PD, 2, TIM1, 3)
#define TIM1_CH3N_PD1_0 CH32V003_PINMUX_DEFINE(PD, 1, TIM1, 0)
#define TIM1_CH3N_PD1_1 CH32V003_PINMUX_DEFINE(PD, 1, TIM1, 1)
#define TIM1_CH3N_PD1_2 CH32V003_PINMUX_DEFINE(PD, 1, TIM1, 2)
#define TIM1_CH3N_PC6_3 CH32V003_PINMUX_DEFINE(PC, 6, TIM1, 3)

#define TIM2_ETR_PD4_0 CH32V003_PINMUX_DEFINE(PD, 4, TIM2, 0)
#define TIM2_ETR_PC5_1 CH32V003_PINMUX_DEFINE(PC, 5, TIM2, 1)
#define TIM2_ETR_PC1_2 CH32V003_PINMUX_DEFINE(PC, 1, TIM2, 2)
#define TIM2_ETR_PC1_3 CH32V003_PINMUX_DEFINE(PC, 1, TIM2, 3)
#define TIM2_CH1_PD4_0 CH32V003_PINMUX_DEFINE(PD, 4, TIM2, 0)
#define TIM2_CH1_PC5_1 CH32V003_PINMUX_DEFINE(PC, 5, TIM2, 1)
#define TIM2_CH1_PC1_2 CH32V003_PINMUX_DEFINE(PC, 1, TIM2, 2)
#define TIM2_CH1_PC1_3 CH32V003_PINMUX_DEFINE(PC, 1, TIM2, 3)
#define TIM2_CH2_PD3_0 CH32V003_PINMUX_DEFINE(PD, 3, TIM2, 0)
#define TIM2_CH2_PC2_1 CH32V003_PINMUX_DEFINE(PC, 2, TIM2, 1)
#define TIM2_CH2_PD3_2 CH32V003_PINMUX_DEFINE(PD, 3, TIM2, 2)
#define TIM2_CH2_PC7_3 CH32V003_PINMUX_DEFINE(PC, 7, TIM2, 3)
#define TIM2_CH3_PC0_0 CH32V003_PINMUX_DEFINE(PC, 0, TIM2, 0)
#define TIM2_CH3_PD2_1 CH32V003_PINMUX_DEFINE(PD, 2, TIM2, 1)
#define TIM2_CH3_PC0_2 CH32V003_PINMUX_DEFINE(PC, 0, TIM2, 2)
#define TIM2_CH3_PD6_3 CH32V003_PINMUX_DEFINE(PD, 6, TIM2, 3)
#define TIM2_CH4_PD7_0 CH32V003_PINMUX_DEFINE(PD, 7, TIM2, 0)
#define TIM2_CH4_PC1_1 CH32V003_PINMUX_DEFINE(PC, 1, TIM2, 1)
#define TIM2_CH4_PD7_2 CH32V003_PINMUX_DEFINE(PD, 7, TIM2, 2)
#define TIM2_CH4_PD5_3 CH32V003_PINMUX_DEFINE(PD, 5, TIM2, 3)

#define USART1_CK_PD4_0  CH32V003_PINMUX_DEFINE(PD, 4, USART1, 0)
#define USART1_CK_PD7_1  CH32V003_PINMUX_DEFINE(PD, 7, USART1, 1)
#define USART1_CK_PD7_2  CH32V003_PINMUX_DEFINE(PD, 7, USART1, 2)
#define USART1_CK_PC5_3  CH32V003_PINMUX_DEFINE(PC, 5, USART1, 3)
#define USART1_TX_PD5_0  CH32V003_PINMUX_DEFINE(PD, 5, USART1, 0)
#define USART1_TX_PD0_1  CH32V003_PINMUX_DEFINE(PD, 0, USART1, 1)
#define USART1_TX_PD6_2  CH32V003_PINMUX_DEFINE(PD, 6, USART1, 2)
#define USART1_TX_PC0_3  CH32V003_PINMUX_DEFINE(PC, 0, USART1, 3)
#define USART1_RX_PD6_0  CH32V003_PINMUX_DEFINE(PD, 6, USART1, 0)
#define USART1_RX_PD1_1  CH32V003_PINMUX_DEFINE(PD, 1, USART1, 1)
#define USART1_RX_PD5_2  CH32V003_PINMUX_DEFINE(PD, 5, USART1, 2)
#define USART1_RX_PC1_3  CH32V003_PINMUX_DEFINE(PC, 1, USART1, 3)
#define USART1_CTS_PD3_0 CH32V003_PINMUX_DEFINE(PD, 3, USART1, 0)
#define USART1_CTS_PC3_1 CH32V003_PINMUX_DEFINE(PC, 3, USART1, 1)
#define USART1_CTS_PC6_2 CH32V003_PINMUX_DEFINE(PC, 6, USART1, 2)
#define USART1_CTS_PC6_3 CH32V003_PINMUX_DEFINE(PC, 6, USART1, 3)
#define USART1_RTS_PC2_0 CH32V003_PINMUX_DEFINE(PC, 2, USART1, 0)
#define USART1_RTS_PC2_1 CH32V003_PINMUX_DEFINE(PC, 2, USART1, 1)
#define USART1_RTS_PC7_2 CH32V003_PINMUX_DEFINE(PC, 7, USART1, 2)
#define USART1_RTS_PC7_3 CH32V003_PINMUX_DEFINE(PC, 7, USART1, 3)

#define SPI1_NSS_PC1_0  CH32V003_PINMUX_DEFINE(PC, 1, SPI1, 0)
#define SPI1_NSS_PC0_1  CH32V003_PINMUX_DEFINE(PC, 0, SPI1, 1)
#define SPI1_SCK_PC5_0  CH32V003_PINMUX_DEFINE(PC, 5, SPI1, 0)
#define SPI1_SCK_PC5_1  CH32V003_PINMUX_DEFINE(PC, 5, SPI1, 1)
#define SPI1_MISO_PC7_0 CH32V003_PINMUX_DEFINE(PC, 7, SPI1, 0)
#define SPI1_MISO_PC7_1 CH32V003_PINMUX_DEFINE(PC, 7, SPI1, 1)
#define SPI1_MOSI_PC6_0 CH32V003_PINMUX_DEFINE(PC, 6, SPI1, 0)
#define SPI1_MOSI_PC6_1 CH32V003_PINMUX_DEFINE(PC, 6, SPI1, 1)

#define I2C1_SCL_PC2_0 CH32V003_PINMUX_DEFINE(PC, 2, I2C1, 0)
#define I2C1_SCL_PD1_1 CH32V003_PINMUX_DEFINE(PD, 1, I2C1, 1)
#define I2C1_SCL_PC5_2 CH32V003_PINMUX_DEFINE(PC, 5, I2C1, 2)
#define I2C1_SDA_PC1_0 CH32V003_PINMUX_DEFINE(PC, 1, I2C1, 0)
#define I2C1_SDA_PD0_1 CH32V003_PINMUX_DEFINE(PD, 0, I2C1, 1)
#define I2C1_SDA_PC6_2 CH32V003_PINMUX_DEFINE(PC, 6, I2C1, 2)

#endif /* __CH32V003_PINCTRL_H__ */
