@ Minimal GBA startup: header space, stacks, section init, IRQ dispatch.
    .section .crt0, "ax"
    .global _start
    .arm
_start:
    b .Lentry
    .space 188                  @ cartridge header, patched by tools/fixrom.py

.Lentry:
    @ IRQ-mode stack
    mov r0, #0xD2               @ IRQ mode, IRQ+FIQ masked
    msr cpsr_c, r0
    ldr sp, =0x03007FA0
    @ System-mode stack (main program runs here)
    mov r0, #0xDF
    msr cpsr_c, r0
    ldr sp, =0x03007F00

    @ copy .iwram (ARM/hot code) from ROM to IWRAM
    ldr r0, =__iwram_lma
    ldr r1, =__iwram_start
    ldr r2, =__iwram_end
1:  cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 1b

    @ copy .data from ROM to IWRAM
    ldr r0, =__data_lma
    ldr r1, =__data_start
    ldr r2, =__data_end
1:  cmp r1, r2
    ldrlo r3, [r0], #4
    strlo r3, [r1], #4
    blo 1b

    @ zero .bss
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
1:  cmp r0, r1
    strlo r2, [r0], #4
    blo 1b

    @ zero .ewram_bss
    ldr r0, =__ewram_bss_start
    ldr r1, =__ewram_bss_end
    mov r2, #0
1:  cmp r0, r1
    strlo r2, [r0], #4
    blo 1b

    @ install IRQ dispatcher, unmask CPU IRQs, and go
    ldr r0, =isr_arm
    ldr r1, =0x03007FFC
    str r0, [r1]
    mov r0, #0x1F               @ system mode, IRQ+FIQ enabled
    msr cpsr_c, r0
    ldr r0, =main
    bx r0

@ --- IRQ dispatcher (ARM, called by BIOS via [0x03007FFC]) ---
    .arm
    .global isr_arm
isr_arm:
    mov r0, #0x04000000
    add r0, r0, #0x200
    ldrh r1, [r0]               @ IE
    ldrh r2, [r0, #2]           @ IF
    and r1, r1, r2              @ pending
    strh r1, [r0, #2]           @ acknowledge IF
    ldr r2, =0x03007FF8         @ BIOS IntrWait flags
    ldrh r3, [r2]
    orr r3, r3, r1
    strh r3, [r2]

    tst r1, #1                  @ VBlank?
    beq 9f
    ldr r2, =g_frame
    ldr r3, [r2]
    add r3, r3, #1
    str r3, [r2]

    push {r1, lr}
    ldr r2, =vblank_tick        @ thumb C function (audio etc.)
    mov lr, pc
    bx r2
    pop {r1, lr}
9:
    bx lr

    .pool
