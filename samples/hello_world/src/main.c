/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int main(void)
{
#ifndef CONFIG_XTENSA_TENSILICA_NX
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
#else
	int a = 0, b = 10;
	for (a = 0; a < 12; a++){
	    if (a == b){
		break;
	    }
	}
#endif
	return 0;
}
