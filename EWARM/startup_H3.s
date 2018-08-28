;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Part one of the system initialization code,
;; contains low-level
;; initialization.
;;
;; Copyright 2007-2017 IAR Systems AB.
;;
;; $Revision: 122993 $
;;

        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION IRQ_STACK:DATA:NOROOT(3)
        SECTION FIQ_STACK:DATA:NOROOT(3)
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
        PUBLIC  __iar_program_start
        EXTERN  Undefined_Handler
        EXTERN  SWI_Handler
        EXTERN  Prefetch_Handler
        EXTERN  Abort_Handler       
        EXTERN  IRQ_Handler
        EXTERN  FIQ_Handler

        DATA

__iar_init$$done:               ; The vector table is not needed
                                ; until after copy initialization is done

__vector:                       ; Make this a DATA label, so that stack usage
                                ; analysis doesn't consider it an uncalled fun

        ARM

        ; All default exception handlers (except reset) are
        ; defined as weak symbol definitions.
        ; If a handler is defined by the application it will take precedence.
        LDR     PC,Reset_Addr           ; Reset
        LDR     PC,Undefined_Addr       ; Undefined instructions
        LDR     PC,SWI_Addr             ; Software interrupt (SWI/SVC)
        LDR     PC,Prefetch_Addr        ; Prefetch abort
        LDR     PC,Abort_Addr           ; Data abort
        DCD     0                       ; RESERVED
        LDR     PC,IRQ_Addr             ; IRQ
        LDR     PC,FIQ_Addr             ; FIQ

        DATA
Reset_Addr:     DCD   __iar_program_start
Undefined_Addr: DCD   Undefined_Handler
SWI_Addr:       DCD   SWI_Handler
Prefetch_Addr:  DCD   Prefetch_Handler
Abort_Addr:     DCD   Abort_Handler
IRQ_Addr:       DCD   irq ;DCD   IRQ_Handler
FIQ_Addr:       DCD   FIQ_Handler


; --------------------------------------------------
; ?cstartup -- low-level system initialization code.
;
; After a reset execution starts here, the mode is ARM, supervisor
; with interrupts disabled.
;



        SECTION .text:CODE:NOROOT(2)
        EXTERN do_irq
        EXTERN  __cmain
        REQUIRE __vector
        EXTWEAK __iar_init_core
        EXTWEAK __iar_init_vfp
        

        ARM

__iar_program_start:
?cstartup:

;
; Add initialization needed before setup of stackpointers here.
;

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

#define MODE_MSK 0x1F            ; Bit mask for mode bits in CPSR

#define USR_MODE 0x10            ; User mode
#define FIQ_MODE 0x11            ; Fast Interrupt Request mode
#define IRQ_MODE 0x12            ; Interrupt Request mode
#define SVC_MODE 0x13            ; Supervisor mode
#define ABT_MODE 0x17            ; Abort mode
#define UND_MODE 0x1B            ; Undefined Instruction mode
#define SYS_MODE 0x1F            ; System mode

#define  ARMV7_USR_MODE           0x10
#define  ARMV7_FIQ_MODE           0x11
#define  ARMV7_IRQ_MODE           0x12
#define  ARMV7_SVC_MODE           0x13
#define  ARMV7_MON_MODE           0x16
#define  ARMV7_ABT_MODE           0x17
#define  ARMV7_UND_MODE           0x1b
#define  ARMV7_SYSTEM_MODE        0x1f
#define  ARMV7_MODE_MASK          0x1f
#define  ARMV7_FIQ_MASK           0x40
#define  ARMV7_IRQ_MASK           0x80
#define CR_V	0x2000	;Vectors relocated to 0xffff0000




        MRS     r0, cpsr                ; Original PSR value

        ;; Set up the interrupt stack pointer.

        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #IRQ_MODE       ; Set IRQ mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     r1, =SFE(IRQ_STACK)     ; End of IRQ_STACK
        BIC     sp,r1,#0x7              ; Make sure SP is 8 aligned

        ;; Set up the fast interrupt stack pointer.

        BIC     r0, r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0, r0, #FIQ_MODE       ; Set FIR mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     r1, =SFE(FIQ_STACK)     ; End of FIQ_STACK
        BIC     sp,r1,#0x7              ; Make sure SP is 8 aligned

        ;; Set up the normal stack pointer.

        BIC     r0 ,r0, #MODE_MSK       ; Clear the mode bits
        ORR     r0 ,r0, #SYS_MODE       ; Set System mode bits
        MSR     cpsr_c, r0              ; Change the mode
        LDR     r1, =SFE(CSTACK)        ; End of CSTACK
        BIC     sp,r1,#0x7              ; Make sure SP is 8 aligned

         //--
        BL  cpu_init_cp15

        ;; Turn on core features assumed to be enabled.
        FUNCALL __iar_program_start, __iar_init_core
        BL      __iar_init_core

        ;; Initialize VFP (if needed).
        FUNCALL __iar_program_start, __iar_init_vfp
        BL      __iar_init_vfp 
        ;;; Continue to __cmain for C-level initialization.

         FUNCALL __iar_program_start, __cmain
         B       __cmain

;;;
;;; Add more initialization here
;;;
;ENTRY(cpu_init_cp15)
	/*
	 * Invalidate L1 I/D
	 */
 cpu_init_cp15:
        ;set SMP bit  //add bu wg 
        mrc 	p15, 0, r0, c1, c0, 1
	orr     r0, r0, #(1<<6)
	mcr	p15, 0, r0, c1, c0, 1
        
        
        /* Set V=0 in CP15 SCTLR register - for VBAR to point to vector */
	mrc	p15, 0, r0, c1, c0, 0	; Read CP15 SCTLR Register
	;bic	r0, #CR_V		; V = 0         
        bic	r0, r0, #0x00002000
	mcr	p15, 0, r0, c1, c0, 0	; Write CP15 SCTLR Register

	/* Set vector address in CP15 VBAR register */
	ldr	r0, =__vector
	mcr	p15, 0, r0, c12, c0, 0	;Set VBAR
 
  
	mov	r0, #0			; set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	; invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	; invalidate icache
	mcr	p15, 0, r0, c7, c5, 6	; invalidate BP array
	mcr     p15, 0, r0, c7, c10, 4	; DSB
	mcr     p15, 0, r0, c7, c5, 4	; ISB

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	; clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	; clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	; set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	; set bit 11 (Z---) BTB
 
	orr	r0, r0, #0x00001000	; set bit 12 (I) I-cache
	;bic	r0, r0, #0x00001000 ;add by wg
 
	mcr	p15, 0, r0, c1, c0, 0   
        
	mov	r5, lr			; Store my Caller
	mrc	p15, 0, r1, c0, c0, 0	; r1 has Read Main ID Register (MIDR)
	mov	r3, r1, lsr #20		; get variant field
	and	r3, r3, #0xf		; r3 has CPU variant
	and	r4, r1, #0xf		; r4 has CPU revision
	mov	r2, r3, lsl #4		; shift variant field for combined value
	orr	r2, r4, r2		; r2 has combined CPU variant + revision

	mov	pc, r5			; back to my caller
;ENDPROC(cpu_init_cp15)



	;.align	5
        RSEG CODE:CODE:NOROOT(5)
irq:
	sub     lr, lr, #4                                      ;;; 保存返回地址
	stmfd   sp!, {r0-r12, lr} ;;; save context       	        ;; 寄存器压栈
	mrs     r3, spsr                                        ;;; 读取SPSR
	stmfd   sp!, {r3}	                                    ;;; 压栈

        msr     cpsr_c, #(ARMV7_FIQ_MASK | ARMV7_IRQ_MASK | ARMV7_SVC_MODE)     ;;; 切换到SVC模式
        stmfd   sp!, {r0-r12, lr}                                ;;; 保存lr_usr和其它用到的寄存器

	bl      do_irq

        ldmfd   sp!, {r0-r12, lr}                           ;;; 恢复SYSTEM模式寄存器
        msr     cpsr_c, #(ARMV7_FIQ_MASK | ARMV7_IRQ_MASK | ARMV7_IRQ_MODE)     ;;; 切换到IRQ模式
        ldmfd   sp!, {r3} 	                                    ;;; 数据出栈
        msr     spsr_cxsf, r3                                   ;;; 还原spsr

	ldmfd   sp!, {r0-r12, pc}^    ;;;从异常模式返回; unknown event ignore






        END
