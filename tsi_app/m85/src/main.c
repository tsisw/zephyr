
/*----------------------------------------------------------------------------------------------------- */
/* Copyright (c) 2024 Tsavorite Scalable Intelligence, Inc. All rights reserved.                        */
/*                                                                                                      */
/*                                                                                                      */
/* This file is the confidential and proprietary property of Tsavorite Scalable Intelligence, Inc       */
/* Possession or use of this file requires a written license from Tsavorite Scalable Intelligence, Inc  */
/*----------------------------------------------------------------------------------------------------- */


#include <stddef.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

#define DATA_MAX_DLEN 8
#define LOG_MODULE_NAME m85 
LOG_MODULE_REGISTER(LOG_MODULE_NAME);


int main(void)
{

	/* TSI banner */
	printf("!! ------------------------------------------ !! \n"); 
	printf("!! WELCOME TO TSAVORITE SCALABLE INTELLIGENCE !! \n"); 
	printk("!! ------------------------------------------ !! \n"); 
        LOG_INF("Test Platform: %s",CONFIG_BOARD_TARGET);


        LOG_WRN("Testing on FPGA; Multi module init TBD");

	return 0;
}

