OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
SECTIONS
{
	. = 0x00000000;
	start = .;
	.init : { *(.init) }
	.text : { *(.text) }
	.data : { *(.data) }
	__bss_start__ = .;
	.bss : { *(.bss) }
	__bss_end__ = .;
	.rodata : { *(.rodata) }
	end = .;
}
