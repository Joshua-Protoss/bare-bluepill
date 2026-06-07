MEMORY
{
    flash (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

PROVIDE(_stack = ORIGIN(ram) + LENGTH(ram));
ENTRY(reset_handler);

SECTIONS 
{
    .text : {
        KEEP(*(.isr_vector))
        *(.text*)
        . = ALIGN(4);
        *(.rodata*)
        . = ALIGN(4);
    } >flash
    . = ALIGN(4);

    .data : {
        _data = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .; 
    } >ram AT >flash
    _sidata = LOADADDR(.data);

    .bss : {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    }>ram

}