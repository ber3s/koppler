;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Part one of the system initialization code,
;; contains low-level
;; initialization.
;;
;; Copyright 2006 IAR Systems. All rights reserved.
;;
;; $Revision: 14946 $
;;

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION IRQ_STACK:DATA:NOROOT(3)
        SECTION FIQ_STACK:DATA:NOROOT(3)
        SECTION ABT_STACK:DATA:NOROOT(3)
        SECTION SVC_STACK:DATA:NOROOT(3)
        SECTION UND_STACK:DATA:NOROOT(3)
        SECTION CSTACK:DATA:NOROOT(3)

;
; The module in this file are included in the libraries, and may be
; replaced by any user-defined modules that define the PUBLIC symbol
; __iar_program_start or a user defined start symbol.
;
; To override the cstartup defined in the library, simply add your
; modified version to the workbench project.

        SECTION .intvec:CODE:NOROOT(2)

        PUBLIC  __vector
        PUBLIC  __vector_0x14
        PUBLIC  __iar_program_start
        EXTERN IRQ_Handler

        ARM
        ARM
__vector:
        ;;
        ldr   pc,[pc,#+24]              ;; Reset
        B   .                           ;; Undefined instructions
        B   .                           ;; Software interrupt (SWI/SVC)
        B   .                           ;; Prefetch abort
        B   .                           ;; Data abort
__vector_0x14:
        DC32  0                         ;; RESERVED
        ldr   pc,[pc,#+24]              ;; IRQ
        B   .                           ;; FIQ

        DC32  __iar_program_start       ;; Reset
        DC32  0                         ;; Undefined instructions
        DC32  0                         ;; Software interrupt (SWI/SVC)
        DC32  0                         ;; Prefetch abort
        DC32  0                         ;; Data abort
        DC32  0                         ;; RESERVED
        DC32  IRQ_Handler           ;; IRQ
        DC32  0                         ;; FIQ


; --------------------------------------------------
; ?cstartup -- low-level system initialization code.
;
; After a reser execution starts here, the mode is ARM, supervisor
; with interrupts disabled.
;



        SECTION .text:CODE:NOROOT(2)

;        PUBLIC  ?cstartup
        EXTERN  ?main
        REQUIRE __vector

        ARM

__iar_program_start:
?cstartup:

;
; Add initialization needed before setup of stackpointers here.
;
; LPC2148 Errata
; Date: August 5, 2005
; Document Release: Version 1.0
; Device Affected: LPC2148
; Incorrect read of data from SRAM after Reset and MAM is not enabled or partially enabled MAM.1
; Init MAM before acsses to SRAM
MAMCR    DEFINE 0xE01FC000      ; MAM Control Register
MAMTIM   DEFINE 0xE01FC004      ; MAM Timing register

                ldr   r0,=MAMCR
                ldr   r1,=MAMTIM
                ldr   r2,=0
                str   r2,[r0]
                ldr   r2,=7
                str   r2,[r1]
                ldr   r2,=2
                str   r2,[r0]

;
; Initialize the stack pointers.
; The pattern below can be used for any of the exception stacks:
; FIQ, IRQ, SVC, ABT, UND, SYS.
; The USR mode uses the same stack as SYS.
; The stack segments must be defined in the linker command file,
; and be declared above.
;
; --------------------
; Mode, correspords to bits 0-5 in CPSR

MODE_MSK DEFINE 0x1F            ; Bit mask for mode bits in CPSR

USR_MODE DEFINE 0x10            ; User mode
FIQ_MODE DEFINE 0x11            ; Fast Interrupt Request mode
IRQ_MODE DEFINE 0x12            ; Interrupt Request mode
SVC_MODE DEFINE 0x13            ; Supervisor mode
ABT_MODE DEFINE 0x17            ; Abort mode
UND_MODE DEFINE 0x1B            ; Undefined Instruction mode
SYS_MODE DEFINE 0x1F            ; System mode

; Initialize the stack pointers.
; The pattern below can be used for any of the exception stacks:
; FIQ, IRQ, SVC, ABT, UND, SYS.
; The USR mode uses the same stack as SYS.
; The stack segments must be defined in the linker command file,
; and be declared above.

                mrs     r0,cpsr                             ; Original PSR value
                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#SVC_MODE                     ; Set Supervisor mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(SVC_STACK)                  ; End of SVC_STACK

                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#IRQ_MODE                     ; Set IRQ mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(IRQ_STACK)                  ; End of IRQ_STACK

                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#ABT_MODE                     ; Set Abort mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(ABT_STACK)                  ; End of ABT_STACK

                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#UND_MODE                     ; Set Undefined mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(UND_STACK)                  ; End of FIR_STACK

                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#FIQ_MODE                     ; Set FIR mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(FIQ_STACK)                  ; End of FIR_STACK

                bic     r0,r0,#MODE_MSK                     ; Clear the mode bits
                orr     r0,r0,#SYS_MODE                     ; Set System mode bits
                msr     cpsr_c,r0                           ; Change the mode
                ldr     sp,=SFE(CSTACK)                     ; End of CSTACK


#ifdef __ARMVFP__
; Enable the VFP coprocessor.
                mov     r0, #0x40000000                 ; Set EN bit in VFP
                fmxr    fpexc, r0                       ; FPEXC, clear others.

; Disable underflow exceptions by setting flush to zero mode.
; For full IEEE 754 underflow compliance this code should be removed
; and the appropriate exception handler installed.
                mov     r0, #0x01000000           ; Set FZ bit in VFP
                fmxr    fpscr, r0                       ; FPSCR, clear others.
#endif

; Add more initialization here


; Continue to ?main for more IAR specific system startup

                ldr     r0,=?main
                bx      r0

        END

