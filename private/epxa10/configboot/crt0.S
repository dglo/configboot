/*
 * crt0.S
 *
 * Copyright (c) Altera Corporation 2002.
 * All rights reserved.
 *
 * This file defines the function _start, which is the entry point for the 
 * image. This is called by the Altera boot loader once the boot loader 
 * has initialised the EPXA10, and loaded the image into RAM.
 *
 * _start performs the initialisation for the C run time environment, and 
 * then calls main().
 *
 * The c initialisation includes:
 * 1. Turn on the instruction cache
 * 2. Turn on the instruction cache and MMU, see below for details on the 
 *    mapping
 * 3. Setup the stack for all modes
 * 4. Clear BSS
 * 5. Initialise the UART
 * 6. Switch to User mode with IRQ's enabled, FIQs disabled
 * 7. Branch to main
 *
 */
#define Mode_USR	0x10
#define Mode_FIQ	0x11
#define Mode_IRQ	0x12
#define Mode_SVC	0x13
#define Mode_ABT	0x17
#define Mode_UNDEF	0x1B
#define Mode_SYS	0x1F /* available on ARM Arch 4 and later */

#define I_Bit		0x80 /* when I bit is set, IRQ is disabled */
#define F_Bit		0x40 /* when F bit is set, FIQ is disabled */

	/* System memory locations
	 */
#define RAM_Limit       (128*1024)

#define SVC_Stack	RAM_Limit	/* 8K SVC stack at top of memory */
#define IRQ_Stack	RAM_Limit-8192	/* followed by  1k IRQ stack */
#define ABT_Stack	IRQ_Stack-1024	/* followed by  1k ABT stack */
#define FIQ_Stack	ABT_Stack-1024	/* followed by	1k FIQ stack */
#define UNDEF_Stack	FIQ_Stack-1024	/* followed by	1k UNDEF stack */
#define USR_Stack	UNDEF_Stack-1024	/* followed by  USR stack */

	.section .init
				
	b       Boot
	b	UdefHnd
	b	SwiHnd
	b	PabtHnd
	b	DabtHnd
	b	Unexpected
	b	IrqHnd
	b	FiqHnd

Unexpected:
UdefHnd:
SwiHnd:
IrqHnd:
PabtHnd:
DabtHnd:
FiqHnd:
	b	Unexpected

Boot:
	/* Turn on the instruction cache */
	mrc	p15,0,r0,c1,c0,0
	ldr	r1,=0x1078
	orrs	r0,r0,r1
	mcr	p15,0,r0,c1,c0,0

	/* Initialise stack pointer registers */

	/* Enter SVC mode and set up the SVC stack pointer */
	msr     cpsr_c, #Mode_SVC | I_Bit | F_Bit /* No interrupts */
	ldr     sp, =SVC_Stack

	/* Enter IRQ mode and set up the IRQ stack pointer */
	msr     cpsr_c, #Mode_IRQ | I_Bit | F_Bit /* No interrupts */
	ldr     sp, =IRQ_Stack

	/* Enter FIQ mode and set up the FIQ stack pointer */
	msr     cpsr_c, #Mode_FIQ | I_Bit | F_Bit /* No interrupts */
	ldr     sp, =FIQ_Stack

	/* Enter UNDEF mode and set up the UNDEF stack pointer */
	msr	cpsr_c, #Mode_UNDEF | I_Bit | F_Bit /* No interrupts */
	ldr	sp, =UNDEF_Stack

	/* Enter ABT mode and set up the ABT stack pointer */
	msr	cpsr_c, #Mode_ABT | I_Bit | F_Bit /* No interrupts */
	ldr	sp, =ABT_Stack

	/* Now change to User mode, and set up User mode stack. */
	/*
	msr     cpsr_c, #Mode_USR | I_Bit | F_Bit
	ldr     sp, =USR_Stack
	*/
	
	/* clear the frame pointer */
	mov fp, #0

	/* clear bss */
	ldr r0, =__bss_start__
	mov r1, #0
	ldr r2, =__bss_end__
	sub r2, r2, r0
	bl memset

	/* Enter SVC mode and use the USR stack pointer */
	msr     cpsr_c, #Mode_SVC | F_Bit /* interrupts on */
	ldr     sp, =USR_Stack

	/* call the c entry point */
	bl	main

	/* Loop forever, just in case we return from main */
finished:
	b finished

	/* Place the literal table here */
	.ltorg








