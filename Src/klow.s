/******************************************************************************
 *
 *     [[K0BA - Kernel 0 For Embedded Applications] | [VERSION: 0.3.1]]
 *
 ******************************************************************************
 ******************************************************************************
 *  Module            : Low-level Scheduler/Start-Up.
 *  Provides to       : High-level Scheduler / Main application.
 *  Public API        : No
 *
 *  o In this unit:
 *                Low-level kernel routines.
 *
 *****************************************************************************/


/*@file klow.s */
.syntax unified /* thumb2 */
.text
.align 4

/* task status values */
.equ READY,   0x01
.equ RUNNING, 0x02

/* armv7m reg addresses */
.equ SCB_ICSR,   0xE000ED04 /* base icsr */
.equ STICK_CTRL, 0xE000E010 /* systick csr */

/* arvm7m reg values */
.equ ISCR_SETPSV, (1 << 28)
.equ ISCR_CLRPSV, (1 << 27)
.equ STICK_CLRP, (1 << 30)
.equ STICK_ON,  0x0F
.equ STICK_OFF, 0x00

/* kernel specifics  */
.equ APP_START_UP_IMM,   0xAA
.equ USR_CTXT_SWTCH,	 0xC5
.equ FAULT_SVC, 0xFF
.equ SP_OFFSET, 0
.equ STATUS_OFFSET, 4
.equ RUNCNTR_OFFSET, 8

.global __getReadyPrio
.type __getReadyPrio, %function
.thumb_func
__getReadyPrio:
CLZ  R12, R0
MOV  R0, R12
NEG  R0, R0
ADD  R0, #31
BX LR

.global kErrHandler  /* defined in kerr.c */
.type kErrHandler, %function
.thumb_func

.global SVC_Handler
.type SVC_Handler, %function
.thumb_func
SVC_Handler:       /* we start-up from msp               */
    TST LR, #4     /* for MSP, LR here shall end with a x9               */
    BNE CHECK_CTXTSWTCH /* swtch ctxts from psp */
   /*fallthrough: check if #N is what we are prepared to handle.         */
   /*  when trapping 6 reg are pushed                                    */
    MRS R12, MSP
    LDR R1, [R12, #24] /* <-  the instruction that brought us here       */
   /* a THUMB2 instruction is 16-bit, armv7m is little endian.           */
   /* the immediate is 1 byte. then, within code memory,                 */
   /* the immediate can be taken, by looking at the BYTE placed, 16      */
   /* [B]it towards the (-)minor address position                        */
    LDRB R12, [R1,#-2]
    CMP R12, #APP_START_UP_IMM
    BEQ USRSTARTUP
    BNE ERR
    CHECK_CTXTSWTCH:
    CPSID I
    MRS R12, PSP
    LDR R1, [R12, #24]
    LDRB R12, [R1,#-2]
    CMP R12, #USR_CTXT_SWTCH
    BEQ SWITCHTASK
    /*fallthru*/
    ERR:
    MOV R0, #FAULT_SVC
    BL  kErrHandler

/* start user application */
.global USRSTARTUP
.type USRSTARTUP, %function
.thumb_func
 USRSTARTUP:                  /* get camera-ready:                        */
    CPSID I
    MOVS R0, #2               /* 0b10 = PSP, PRIV                         */
    MSR CONTROL, R0
    LDR R1, =runPtr
    LDR R2, [R1, #SP_OFFSET]
    MOVS R12, #RUNNING
    STR R12, [R2, #STATUS_OFFSET]
    MOVS R12, #0x01
    STR R12, [R2, #RUNCNTR_OFFSET]
    LDR R2, [R2, #SP_OFFSET] /* base address == &(runPtr->sp)              */
    LDMIA R2!, {R4-R11}      /* 'POP' R4-R11 as    assembled on kInitTcb_  */
    MSR PSP, R2              /* update PSP after 'popping '                */
    MOV LR, #0xFFFFFFFD      /* set LR to indicate we choose PSP           */
    LDR R0, =STICK_CTRL
    MOVS R1, #STICK_ON       /* tick on                                    */
    STR R1, [R0]
    DSB
    CPSIE I
    ISB
    BX LR                    /*...running, running, running...             */

.global SysTick_Handler
.type SysTick_Handler, %function
.thumb_func
SysTick_Handler:
    CPSID I
    PUSH {R0, LR}
    TICKHANDLER:
    BL kTickHandler        /* always run kTickHandler, result in R0        */
    CMP R0, #1
    BEQ SETPENDSV
    POP {R0, LR}
    CMP LR, 0xFFFFFFF1
    BEQ RESUMEISR
    RESUMETASK:
    MOV LR, #0xFFFFFFFD
    CPSIE I
    ISB
    BX LR
    RESUMEISR:
    CPSIE I
    ISB
    BX LR
    SETPENDSV:             /* defer ctxt swtch                             */
    POP {R0, LR}
    LDR R0, =SCB_ICSR
    LDR R1, =ISCR_SETPSV
    STR R1, [R0]
    DSB
    CPSIE I
    ISB
    BX LR

/* deferred context switching */
.global PendSV_Handler
.type PendSV_Handler, %function
.thumb_func
PendSV_Handler:
    CPSID I
    SWITCHTASK:
    LDR R0, =STICK_CTRL
    MOVS R1, #STICK_OFF
    STR R1, [R0]
    BL SAVEUSRCTXT
    BL kSchSwtch
    B  RESTOREUSRCTXT

/* user ctxt save: */

/* when a  thread is interruptEd the CPU pushes:                             */
/* STACKFRAME: caller-saved registers                                        */
/* [ xPSR    ]                                                               */
/* [ PC(R15) ]                                                               */
/* [ LR(R14) ]                                                               */
/* [ R12     ]                                                               */
/* [ R 3     ]                                                               */
/* [ R 2     ]                                                               */
/* [ R 1     ]                                                               */
/* [ R 0     ] (<-SP points to R0)                                           */
/*  more often than NOT, the former stacked registers are enough to restore  */
/*  context properly and, keeping this way would indeed provide us a faster  */
/*  ctxt swtch. but, the kernel cannot (without adding more overhead) know   */
/*  what really has changed. so, we go conservative, save everything:        */

.global SAVEUSRCTXT
.type SAVEUSRCTXT, %function
.thumb_func
SAVEUSRCTXT:
    MRS R12, PSP              /* arM Read Special register                   */
    STMDB R12!, {R4-R11}      /* we assume no FPU                            */

/* STACKFRAME: callee-saved registers                                        */
/* [ R11     ]                                                               */
/* [ R10     ]                                                               */
/* [ R 9     ]                                                               */
/* [ R 8     ]                                                               */
/* [ R 7     ]                                                               */
/* [ R 6     ]                                                               */
/* [ R 5     ]                                                               */
/* [ R 4     ] <--  r12 points to the bottom of the stack.                   */
/*                  this is were we resume a task from                       */
    MSR PSP, R12    /* keep the task state consistent, though switchin       */
    LDR R0, =runPtr
    LDR R1, [R0]
    STR R12, [R1]
    DSB
    BX LR
/* user ctxt restore: now we do the opposite                              */
/* after getting the saved address pointing to the value R4 was storing   */
/* we assign it to  PSP, POP then back to the core and - after updating   */
/* PSP and LR, we return and the CPU will restore the remaining frame     */

.global RESTOREUSRCTXT
.type RESTOREUSRCTXT, %function
.thumb_func
RESTOREUSRCTXT:
    LDR R0, =runPtr
    LDR R1, [R0]
    LDR R3, [R1, #RUNCNTR_OFFSET]
    ADDS R3, #1
    STR R3, [R1, #RUNCNTR_OFFSET]
    MOVS R12, #2
    STR R12, [R1, #STATUS_OFFSET]
    LDR R2, [R1]
    LDMIA R2!, {R4-R11}
    MSR PSP, R2
    MOV LR, #0xFFFFFFFD
    LDR R0, =STICK_CTRL
    MOVS R1, #STICK_ON
    STR R1, [R0]
    DSB
    CPSIE I
    ISB
    BX LR                   /*...running, running, running...            */
/* note we couldnt use PUSH/POP   pseudo-instructions because here in the */
/* handler     sp is the MSP, and these pseudo-instructions take the      */
/* current stack pointer as the base address. quite odd (for a RISC),     */
/* we have Load Multiple Increment After  for POPPing,and  Store Multiple */
/* Decrement Before for PUSHing. they load/store and update the addresses */
/* but as we are not operating directly on the PSP - as PUSH/POP would,   */
/* we update its value manually                                           */
