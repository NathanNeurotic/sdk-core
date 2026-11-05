/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# (C)2001, Gustavo Scotti (gustavo@scotti.com)
# (c) 2003 Marcus R. Brown (mrbrown@0xd6.org)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * EE Kernel functions
 */

#include <syscallnr.h>

	.text
	.p2align 3

#define SYSCALL(name)	\
		SYSCALL_SPECIAL(name, name)

#ifdef USE_KMODE
#define SYSCALL_SPECIAL(symbol, name) 		\
	.set	push;		\
	.set	noreorder;	\
	.text;			\
	.align 4;		\
	.globl	symbol;		\
	.type	symbol,@function;	\
	.ent	symbol,0;		\
symbol:	j	__syscall;	\
	li	$3, __NR_##name;\
	nop;			\
	.end	symbol;		\
	.size	symbol,.-symbol;	\
	.set	pop;
#else
#define SYSCALL_SPECIAL(symbol, name) 		\
	.set	push;		\
	.set	noreorder;	\
	.text;			\
	.align 4;		\
	.globl	symbol;		\
	.type	symbol,@function;	\
	.ent	symbol,0;		\
symbol:	li	$3, __NR_##name;\
	syscall;		\
	jr	$31;		\
	nop;			\
	.end	symbol;		\
	.size	symbol,.-symbol;	\
	.set	pop;
#endif

.set	push
.set	noreorder
.text
.align	4
.globl	__syscall
.type	__syscall,@function
.ent	__syscall,0
__syscall:
        mfc0        $2, $12
        andi        $2, $2, 0x18
        beqz        $2, _kMode
        slt         $2, $3, $0
        syscall
        jr          $31
        nop
_kMode:
        subu        $26, $0, $3
        movn        $3, $26, $2
        sll         $3, $3, 2
        lui         $26, 0x8000
        lhu         $2, 0x02F0($26)
        sll         $2, $2, 16
        lh          $26, 0x02F8($26)
        add         $2, $26
        addu        $3, $2
        lw          $26, 0x00($3)
        jr          $26
        nop
.end	__syscall
.size	__syscall,.-__syscall
.set	pop

SYSCALL(ResetEE)

SYSCALL(SetGsCrt)

SYSCALL(KExit)

SYSCALL(_LoadExecPS2)

SYSCALL(_ExecPS2)

SYSCALL(RFU009)

SYSCALL(AddSbusIntcHandler)

SYSCALL(RemoveSbusIntcHandler)

SYSCALL(Interrupt2Iop)

SYSCALL(SetVTLBRefillHandler)

SYSCALL(SetVCommonHandler)

SYSCALL(SetVInterruptHandler)

SYSCALL(AddIntcHandler)

SYSCALL(AddIntcHandler2)

SYSCALL(RemoveIntcHandler)

SYSCALL(AddDmacHandler)

SYSCALL(AddDmacHandler2)

SYSCALL(RemoveDmacHandler)

SYSCALL(_EnableIntc)

SYSCALL(_DisableIntc)

SYSCALL(_EnableDmac)

SYSCALL(_DisableDmac)

SYSCALL(_SetAlarm)

SYSCALL(SetAlarm)

SYSCALL(_ReleaseAlarm)

SYSCALL(ReleaseAlarm)

SYSCALL(_iEnableIntc)

SYSCALL(_iDisableIntc)

SYSCALL(_iEnableDmac)

SYSCALL(_iDisableDmac)

SYSCALL(_iSetAlarm)

SYSCALL(iSetAlarm)

SYSCALL(_iReleaseAlarm)

SYSCALL(iReleaseAlarm)

SYSCALL(CreateThread)

SYSCALL(DeleteThread)

SYSCALL(StartThread)

SYSCALL(ExitThread)

SYSCALL(ExitDeleteThread)

SYSCALL(TerminateThread)

SYSCALL(iTerminateThread)

SYSCALL(DisableDispatchThread)

SYSCALL(EnableDispatchThread)

SYSCALL(ChangeThreadPriority)

SYSCALL(iChangeThreadPriority)

SYSCALL(RotateThreadReadyQueue)

SYSCALL(_iRotateThreadReadyQueue)

SYSCALL(ReleaseWaitThread)

SYSCALL(iReleaseWaitThread)

SYSCALL(GetThreadId)

SYSCALL(_iGetThreadId)

SYSCALL(ReferThreadStatus)

SYSCALL(iReferThreadStatus)

SYSCALL(SleepThread)

SYSCALL(WakeupThread)

SYSCALL(_iWakeupThread)

SYSCALL(CancelWakeupThread)

SYSCALL(iCancelWakeupThread)

SYSCALL(SuspendThread)

SYSCALL(_iSuspendThread)

SYSCALL(ResumeThread)

SYSCALL(iResumeThread)

SYSCALL(RFU059)

SYSCALL(RFU060)

SYSCALL(SetupThread)

SYSCALL(RFU061)

SYSCALL(SetupHeap)

SYSCALL(EndOfHeap)

SYSCALL(CreateSema)

SYSCALL(DeleteSema)

SYSCALL(SignalSema)

SYSCALL(iSignalSema)

SYSCALL(WaitSema)

SYSCALL(PollSema)

SYSCALL(iPollSema)

SYSCALL(ReferSemaStatus)

SYSCALL(iReferSemaStatus)

SYSCALL(iDeleteSema)

SYSCALL(SetOsdConfigParam)

SYSCALL(GetOsdConfigParam)

SYSCALL(GetGsHParam)

SYSCALL(GetGsVParam)

SYSCALL(SetGsHParam)

SYSCALL(SetGsVParam)

SYSCALL(CreateEventFlag)

SYSCALL(DeleteEventFlag)

SYSCALL(SetEventFlag)

SYSCALL(iSetEventFlag)

SYSCALL(PutTLBEntry)

SYSCALL(iPutTLBEntry)

SYSCALL(_SetTLBEntry)

SYSCALL(iSetTLBEntry)

SYSCALL(GetTLBEntry)

SYSCALL(iGetTLBEntry)

SYSCALL(ProbeTLBEntry)

SYSCALL(iProbeTLBEntry)

SYSCALL(ExpandScratchPad)

SYSCALL(EnableIntcHandler)

SYSCALL(iEnableIntcHandler)

SYSCALL(DisableIntcHandler)

SYSCALL(iDisableIntcHandler)

SYSCALL(EnableDmacHandler)

SYSCALL(iEnableDmacHandler)

SYSCALL(DisableDmacHandler)

SYSCALL(iDisableDmacHandler)

SYSCALL(KSeg0)

SYSCALL(EnableCache)

SYSCALL(DisableCache)

SYSCALL(GetCop0)

SYSCALL(FlushCache)

SYSCALL(CpuConfig)

SYSCALL(iGetCop0)

SYSCALL(iFlushCache)

SYSCALL(RFU105)

SYSCALL(iCpuConfig)

SYSCALL(sceSifStopDma)

SYSCALL(SetCPUTimerHandler)

SYSCALL(SetCPUTimer)

SYSCALL(SetOsdConfigParam2)

SYSCALL(GetOsdConfigParam2)

SYSCALL(GsGetIMR)

SYSCALL(iGsGetIMR)

SYSCALL(GsPutIMR)

SYSCALL(iGsPutIMR)

SYSCALL(SetPgifHandler)

SYSCALL(SetVSyncFlag)

SYSCALL(SetSyscall)

SYSCALL(_print)

SYSCALL(sceSifDmaStat)

SYSCALL(isceSifDmaStat)

SYSCALL(sceSifSetDma)

SYSCALL(isceSifSetDma)

SYSCALL(sceSifSetDChain)

SYSCALL(isceSifSetDChain)

SYSCALL(sceSifSetReg)

SYSCALL(sceSifGetReg)

SYSCALL(_ExecOSD)

SYSCALL(Deci2Call)

SYSCALL(PSMode)

SYSCALL(MachineType)

SYSCALL(GetMemorySize)

SYSCALL(_GetGsDxDyOffset)

SYSCALL(_InitTLB)

SYSCALL(SetMemoryMode)

SYSCALL(GetMemoryMode)

	.text
	.align	2
.globl ChangeGP
.ent ChangeGP
ChangeGP:
	move	$v0, $gp
	jr	$ra
	move	$gp, $a0
.end ChangeGP

.globl SetGP
.ent SetGP
SetGP:
	jr	$ra
	move	$gp, $a0
.end SetGP

.globl GetGP
.ent GetGP
GetGP:
	jr	$ra
	move	$v0, $gp
.end GetGP


	.globl	sceSifWriteBackDCache
	.ent	sceSifWriteBackDCache
	.set	push
	.set	noreorder

sceSifWriteBackDCache:	/* DHWBIN: Data cache Hit WriteBack INvalidate.  */

	lui	$25, 0xffff
	ori	$25, $25, 0xffc0
	blez	$5, last
	addu	$10, $4, $5
	and	$8, $4, $25
	addiu	$10, $10, -1
	and	$9, $10, $25
	subu	$10, $9, $8
	srl	$11, $10, 0x6
	addiu	$11, $11, 1
	andi	$9, $11, 0x7
	beqz	$9, eight
	srl	$10, $11, 0x3
loop1:
	sync
	cache	0x18, 0($8)
	sync
	addiu	$9, $9, -1
	nop
	bgtz	$9, loop1
	addiu	$8, $8, 64

eight:
	beqz	$10, last
loop8:
	addiu	$10, $10, -1
	sync
	cache	0x18, 0($8)
	sync
	cache	0x18, 64($8)
	sync
	cache	0x18, 128($8)
	sync
	cache	0x18, 192($8)
	sync
	cache	0x18, 256($8)
	sync
	cache	0x18, 320($8)
	sync
	cache	0x18, 384($8)
	sync
	cache	0x18, 448($8)
	sync
	bgtz	$10, loop8
	addiu	$8, $8, 512
last:
	jr	$31
	nop

	.set	pop
	.end	sceSifWriteBackDCache

#define DXWBIN	0x14		/* Data cache: indeX WriteBack INvalidate.  */
#define DXIN	0x16		/* Data cache: indeX INvalidate.  */

#define opDCache(name, op)	\
	.set	push;		\
	.set	noreorder;	\
	.set	nomacro;	\
	.text;			\
	.p2align 3;		\
	.globl	name;		\
	.type	name,@function;	\
	.ent	name, 0;	\
name:	lui	$7, 0xffff;	\
	daddu	$6, $0, $0;	\
	ori	$7, 0xf000;	\
	nop;			\
1:	sync;			\
	cache	0x10, 0($6);	\
	sync;			\
	mfc0	$2, $28;	\
	and	$2, $7;		\
	addu	$2, $6;		\
	sltu	$3, $5, $2;	\
	sltu	$2, $4;		\
	bnez	$2, 2f;		\
	nop;			\
	bnez	$3, 2f;		\
	nop;			\
	sync;			\
	cache	op, 0($6);	\
	sync;			\
2:	sync;			\
	cache	0x10, 1($6);	\
	sync;			\
	mfc0	$2, $28;	\
	and	$2, $7;		\
	addu	$2, $6;		\
	sltu	$3, $5, $2;	\
	sltu	$2, $4;		\
	bnez	$2, 3f;		\
	nop;			\
	bnez	$3, 3f;		\
	nop;			\
	sync;			\
	cache	op, 1($6);	\
	sync;			\
3:	sync;			\
	addiu	$6, 64;		\
	slti	$2, $6, 4096;	\
	bnez	$2, 1b;		\
	nop;			\
	jr	$31;		\
	nop;			\
	.end	name;		\
	.size	name,.-name;	\
	.set	pop;

opDCache(_SyncDCache, DXWBIN)

opDCache(_InvalidDCache, DXIN)

/* This is needed in case we are linked against libm (the math library) but
   not libc.  */
	.globl	__errno
	.ent	__errno
	.weak	__errno

__errno:
	la	$2, errno
	jr	$31
	nop
	.end	__errno

/* New applications compiled against ps2lib that use errno will resolve to
   this, while existing newlib applications will resolve to __errno.  */
	.globl	errno
	.weak	errno

	.data
	.align	2
errno:
	.space	4

/* Assembler version of strlen that uses quadword instructions.

   Jeff Johnston, Cygnus Solutions, Feb 10/1999.

   ============================================================
   Copyright (C) 1999 by Cygnus Solutions. All rights reserved.

   Permission to use, copy, modify, and distribute this
   software is freely granted, provided that this notice
   is preserved.
   ============================================================  */

	.text
	.align	2
	.globl	strlen
	.text
	.ent	strlen
	.weak	strlen
strlen:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0

#ifndef __OPTIMIZE_SIZE__

/* check if src is quadword aligned, doubleword aligned, or neither in which case
   perform checking for null terminator one byte at a time. */

	andi	$2,$4,0x7
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L15_strlen
	move	$7,$4
	.set	pop

	andi	$3,$4,0xf
	dli	$2,0x0101010101010101
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L12_strlen
	move	$5,$4
	.set	pop

/* src is quadword aligned.  Load a quadword at a time and check for null terminator.
   If null terminator is found, go and find exact position by looking at each byte
   of the last quadword.  Otherwise, continue to next quadword and keep searching. */

	lq	$3,0($5)
	pcpyld	$8,$2,$2
	dli	$4,0x8080808080808080
	psubb	$2,$3,$8
	pnor	$3,$0,$3
	pcpyld	$9,$4,$4
	pand	$2,$2,$3
	pand	$2,$2,$9
	pcpyud	$3,$2,$8
	or	$6,$3,$2
	.set	push
	.set	noreorder
	.set	nomacro
	bnel	$6,$0,$L15_strlen
	move	$4,$5
	.set	pop

	addu	$5,$5,16
$L14_strlen:
	lq	$2,0($5)
	#nop
	pnor	$3,$0,$2
	psubb	$2,$2,$8
	pand	$2,$2,$3
	pand	$4,$2,$9
	pcpyud	$3,$4,$6
	or	$3,$3,$4
	.set	push
	.set	noreorder
	.set	nomacro
	beql	$3,$0,$L14_strlen
	addu	$5,$5,16
	.set	pop

	.set	push
	.set	noreorder
	.set	nomacro
	b	$L15_strlen
	move	$4,$5
	.set	pop

/* src is doubleword aligned.  Load a doubleword at a time and check for null terminator.
   If null terminator is found, go and find exact position by looking at each byte
   of the last doubleword.  Otherwise, continue to next doubleword and keep searching. */


$L12_strlen:
	ld	$3,0($5)
	dli	$4,0x8080808080808080
	dsubu	$2,$3,$2
	nor	$3,$0,$3
	and	$2,$2,$3
	and	$2,$2,$4
	.set	push
	.set	noreorder
	.set	nomacro
	bnel	$2,$0,$L15_strlen
	move	$4,$5
	.set	pop

	dli	$6,0x0101010101010101
	addu	$5,$5,8
$L16_strlen:
	ld	$2,0($5)
	#nop
	nor	$3,$0,$2
	dsubu	$2,$2,$6
	and	$2,$2,$3
	and	$2,$2,$4
	.set	push
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L16_strlen
	addu	$5,$5,8
	.set	pop

	move	$4,$5

#else /* __OPTIMIZE_SIZE__ */

	move	$7,$4

#endif /* __OPTIMIZE_SIZE__ */

/* search a byte at a time for null terminator and then calculate length by subtracting
   original string address from null terminator address. */

$L9_strlen:
$L15_strlen:
	.set	push
	.set	noreorder
	.set	nomacro

	lb	$2,0($4)
	#nop
	beq	$2,$0,1f
	addu	$4,$4,1

	lb	$2,0($4)
	#nop
	bne	$2,$0,$L9_strlen
	addu	$4,$4,1
1:
	subu	$4,$4,1

	.set	pop

	.set	push
	.set	noreorder
	.set	nomacro
	j	$31
	subu	$2,$4,$7
	.set	pop

	.end	strlen

/* Assembler version of strncpy using quadword instructions

   Jeff Johnston, Cygnus Solutions, Feb 10/1999.

   ============================================================
   Copyright (C) 1999 by Cygnus Solutions. All rights reserved.

   Permission to use, copy, modify, and distribute this
   software is freely granted, provided that this notice
   is preserved.
   ============================================================  */

	.text
	.p2align 3
	.globl	strncpy
	.text
	.ent	strncpy
	.weak	strncpy
strncpy:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0

	move	$8,$4

#ifndef __OPTIMIZE_SIZE__

/* check if src and dest are doubleword aligned, quadword aligned, or neither in which
   case copy byte by byte */

	or	$7,$5,$4
	li	$10,0x10
	andi	$2,$7,0x7
	li	$9,0x8
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L9_strncpy
	andi	$2,$7,0xf
	.set	pop
	movz	$9,$10,$2
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L17_strncpy
	sltu	$2,$6,$9
	.set	pop

/* src and dest are quadword aligned.  Check a quadword at a time looking for a
   null terminator or until the nth byte is reached.  Otherwise, copy the
   quadword to dest and continue looping checking quadwords.   Once a null
   terminator is found or n is < 16 go copy a byte at a time. */

$L31_strncpy:
	bne	$2,$0,$L9_strncpy
	dli	$7,0x0101010101010101
	lq	$3,0($5)
	pcpyld	$9,$7,$7
	pnor	$3,$0,$3
	dli	$7,0x8080808080808080
	psubb	$2,$3,$9
	pcpyld	$10,$7,$7
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$3,$2,$3
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$3,$0,$L5_strncpy
	move	$7,$8
	.set	pop
	lq	$3,0($5)
	.p2align 3
$L39_strncpy:
	addu	$6,$6,-16
	addu	$5,$5,16
	sltu	$2,$6,16
	sq	$3,0($7)
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L5_strncpy
	addu	$7,$7,16
	.set	pop

	lq	$2,0($5)
	#nop
	pnor	$3,$0,$2
	psubb	$2,$2,$9
	pand	$2,$2,$3
	pand	$2,$2,$10
	pcpyud	$3,$2,$4
	or	$2,$2,$3
	.set	push
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L19_strncpy
	lq	$3,0($5)
	b	$L9_strncpy
	move	$4,$7
	.set	pop

/* src and dest are quadword aligned.  Check a quadword at a time looking for a
   null terminator or until the nth byte is reached.  Otherwise, copy the
   quadword to dest and continue looping checking quadwords.   Once a null
   terminator is found or n is < 16 go copy a byte at a time. */

$L17_strncpy:
	bne	$2,$0,$L9_strncpy
	ld	$3,0($5)
	dli	$9,0x0101010101010101
	dli	$10,0x8080808080808080
	dsubu	$2,$3,$9
	nor	$3,$0,$3
	and	$2,$2,$3
	and	$2,$2,$10
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L5_strncpy
	move	$7,$8
	.set	pop
	ld	$3,0($5)
	.p2align 3
$L19_strncpy:
	addu	$6,$6,-8
	addu	$5,$5,8
	sltu	$2,$6,8
	sd	$3,0($7)
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L5_strncpy
	addu	$7,$7,8
	.set	pop

	ld	$2,0($5)
	#nop
	nor	$3,$0,$2
	dsubu	$2,$2,$9
	and	$2,$2,$3
	and	$2,$2,$10
	.set	push
	.set	noreorder
	.set	nomacro
	beql	$2,$0,$L19_strncpy
	ld	$3,0($5)
	.set	pop

$L5_strncpy:
	move	$4,$7

#endif /* !__OPTIMIZE_SIZE__ */

/* check a byte at a time looking for either the null terminator or until n bytes are
   copied.  If the null terminator is found and n is not reached yet, copy null
   bytes until n is reached. */

	.p2align 3
$L9_strncpy:
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$6,$0,$L18_strncpy
	move	$2,$6
	.set	pop

	lbu	$2,0($5)
	addu	$6,$6,-1
	addu	$5,$5,1
	sb	$2,0($4)
	sll	$2,$2,24
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L9_strncpy
	addu	$4,$4,1
	.set	pop

	move	$2,$6
$L20_strncpy:
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L18_strncpy
	addu	$6,$6,-1
	.set	pop

	.p2align 3
$L16_strncpy:
	sb	$0,0($4)
	move	$2,$6
	addu	$4,$4,1
	.set	push
	.set	noreorder
	.set	nomacro
	nop
	nop
	bne	$2,$0,$L16_strncpy
	addu	$6,$6,-1
	.set	pop

$L18_strncpy:
	.set	push
	.set	noreorder
	.set	nomacro
	j	$31
	move	$2,$8
	.set	pop

	.end	strncpy

/* Assembler version of memcpy using quadword instructions.

   Jeff Johnston, Cygnus Solutions, Feb 10/1999.

   ============================================================
   Copyright (C) 1999 by Cygnus Solutions. All rights reserved.

   Permission to use, copy, modify, and distribute this
   software is freely granted, provided that this notice
   is preserved.
   ============================================================  */

	.text
	.align	2
	.globl	memcpy
	.text
	.ent	memcpy
	.weak	memcpy
memcpy:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	move	$8,$4

#ifndef __OPTIMIZE_SIZE__

/* if bytes to move are < 32 or src or dest are not quadword aligned, jump to
   code that moves one byte at a time */

	sltu	$2,$6,32
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L2_memcpy
	move	$3,$8
	.set	pop

	or	$2,$5,$8
	andi	$2,$2,0xF
	.set	push
	.set	noreorder
	.set	nomacro
	bnel	$2,$0,$L20_memcpy
	addu	$6,$6,-1
	.set	pop

	move	$7,$8

/* while remainder to move is >= 32 bytes, use LQ/SQ quadword instructions
   to move data */

$L5_memcpy:
	lq	$3,0($5)
	addu	$6,$6,-32
	addu	$5,$5,16
	sltu	$4,$6,32
	sq	$3,0($7)
	addu	$7,$7,16
	lq	$2,0($5)
	addu	$5,$5,16
	sq	$2,0($7)
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$4,$0,$L5_memcpy
	addu	$7,$7,16
	.set	pop

	sltu	$2,$6,8
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L2_memcpy
	move	$3,$7
	.set	pop

/* while remainder to move is >= 8 bytes, use LD/SD doubleword instructions
   to move data */

$L9_memcpy:
	ld	$3,0($5)
	addu	$6,$6,-8
	addu	$5,$5,8
	sltu	$2,$6,8
	sd	$3,0($7)
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L9_memcpy
	addu	$7,$7,8
	.set	pop

	move	$3,$7

#else  /* __OPTIMIZE_SIZE__ */

	move	$3,$8

#endif /* !__OPTIMIZE_SIZE__ */

/* Move any remaining bytes one at a time */

$L2_memcpy:
	addu	$6,$6,-1
$L20_memcpy:
	li	$2,-1			# 0xffffffffffffffff
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$6,$2,$L12_memcpy
	move	$4,$2
	.set	pop

$L13_memcpy:
	lbu	$2,0($5)
	addu	$6,$6,-1
	addu	$5,$5,1
	sb	$2,0($3)
	.set	push
	.set	noreorder
	.set	nomacro
	nop
	bne	$6,$4,$L13_memcpy
	addu	$3,$3,1
	.set	pop

$L12_memcpy:
	.set	push
	.set	noreorder
	.set	nomacro
	j	$31
	move	$2,$8
	.set	pop

	.end	memcpy

/* Assembler version of memset using quadword instructions.

   Jeff Johnston, Cygnus Solutions, Feb 10/1999.

   ============================================================
   Copyright (C) 1999 by Cygnus Solutions. All rights reserved.

   Permission to use, copy, modify, and distribute this
   software is freely granted, provided that this notice
   is preserved.
   ============================================================  */

	.text
	.align	2
	.globl	memset
	.text
	.ent	memset
	.weak	memset
memset:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, extra= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0

#ifndef __OPTIMIZE_SIZE__

/* if not setting a double word or more, go and set one byte at a time */

	sltu	$2,$6,8
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L2_memset
	move	$3,$4
	.set	pop

/* if not aligned on a quadword boundary, set one byte at a time */

	andi	$2,$4,0xf
	.set	push
	.set	noreorder
	.set	nomacro
	bne	$2,$0,$L2_memset
	move	$7,$4
	.set	pop

/* otherwise, build a double word containing the bytes to set */

	andi	$9,$5,0xff
	sltu	$10,$6,32
	move	$8,$9
	dsll	$3,$8,8
	or	$8,$3,$9
	pcpyh	$3,$8

/* check if setting 32 bytes or more, otherwise set 8 bytes at a time */

	.set	push
	.set	noreorder
	.set	nomacro
	bne	$10,$0,$L31_memset
	sltu	$2,$6,8
	.set	pop

/* take double word built from the byte to set and make a quadword */

	pcpyld	$8,$3,$3

/* loop while bytes to set >=32 and use quadword stores */

$L12_memset:
	sq	$8,0($7)
	addu	$6,$6,-32
	addu	$7,$7,16
	sltu	$2,$6,32
	sq	$8,0($7)
	.set	push
	.set	noreorder
	.set	nomacro
	beq	$2,$0,$L12_memset
	addu	$7,$7,16
	.set	pop

	.set	push
	.set	noreorder
	.set	nomacro
	b	$L31_memset
	sltu	$2,$6,8
	.set	pop

$L16_memset:
	addu	$6,$6,-8
	addu	$7,$7,8
	sltu	$2,$6,8
$L31_memset:
	.set	push
	.set	noreorder
	.set	nomacro
	nop
	nop
	beql	$2,$0,$L16_memset
	sd	$3,0($7)
	.set	pop

	move	$3,$7

#else  /* __OPTIMIZE_SIZE__ */

	move	$3,$4

#endif /* __OPTIMIZE_SIZE__ */

/* loop while bytes left to set and set one byte at a time */

$L2_memset:
	li	$2,4294901760			# 0xffff0000
	addu	$6,$6,-1
	ori	$2,$2,0xffff
	beq	$6,$2,$L19_memset
	li	$2,4294901760			# 0xffff0000
	ori	$2,$2,0xffff

$L20_memset:
	sb	$5,0($3)
	addu	$6,$6,-1
	.set	push
	.set	noreorder
	.set	nomacro
	nop
	nop
	nop
	bne	$6,$2,$L20_memset
	addu	$3,$3,1
	.set	pop

$L19_memset:
	.set	push
	.set	noreorder
	.set	nomacro
	j	$31
	move	$2,$4
	.set	pop

	.end	memset

