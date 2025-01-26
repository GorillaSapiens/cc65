;
; Startup code for cc65 (supervision version)
;

        .export         _exit
        .export         __STARTUP__ : absolute = 1      ; Mark as startup

        .import         _main
        .import         initlib, donelib, copydata
        .import         zerobss
        .import         __RAM_START__, __RAM_SIZE__     ; Linker generated
        .import         __STACKSIZE__                   ; Linker generated

        .include "zeropage.inc"

        .export _sv_irq_counter
        .export _sv_nmi_counter

.bss

_sv_irq_counter:        .byte 0
_sv_nmi_counter:        .byte 0

.code

reset:
	cld ; clear decimal mode
	sei ; disable interrupts

	; set the CPUSTACK register
	ldx #$FF
	txs

        jsr     zerobss

        ; Initialize data.
        jsr     copydata

        lda     #<(__RAM_START__ + __RAM_SIZE__)
        ldx     #>(__RAM_START__ + __RAM_SIZE__)
        sta     sp
        stx     sp+1            ; Set argument stack ptr
        jsr     initlib
        jsr     _main
_exit:  jsr     donelib
exit:   jmp     exit

.proc   nmi
        inc     _sv_nmi_counter
        rti
.endproc

.proc   irq
        inc     _sv_irq_counter
        rti
.endproc

; Removing this segment gives only a warning.
        .segment "FFF0"

.proc pre_reset
        jmp     reset
.endproc

        .segment "VECTORS"
.word   nmi
.word   pre_reset
.word   irq
