# Helper functions for the MMI test cases that need real 128-bit register
# control (pcpyld/pcpyud copy/select whole 64-bit halves; qfsrv operates on
# a 128+128-bit funnel window) - inline asm from C only guarantees the low
# 64 bits of a "r"-constrained GPR are meaningful, so these are written as
# standalone functions using lq/sq against caller-supplied 16-byte-aligned
# buffers instead. n32 ABI: a0-a2 = $4-$6.

	.set noreorder
	.set noat
	.text

	.globl test_pcpyld
	.ent test_pcpyld
test_pcpyld:
	lq	$2, 0($5)
	lq	$3, 0($6)
	pcpyld	$2, $2, $3
	sq	$2, 0($4)
	jr	$31
	nop
	.end test_pcpyld

	.globl test_pcpyud
	.ent test_pcpyud
test_pcpyud:
	lq	$2, 0($5)
	lq	$3, 0($6)
	pcpyud	$2, $2, $3
	sq	$2, 0($4)
	jr	$31
	nop
	.end test_pcpyud

# shift=0 identity case: mtsab $0,0 forces SA=0, so qfsrv's funnel window
# is entirely rt regardless of the exact SA-computation formula.
	.globl test_qfsrv0
	.ent test_qfsrv0
test_qfsrv0:
	lq	$2, 0($5)
	lq	$3, 0($6)
	mtsab	$0, 0
	qfsrv	$2, $2, $3
	sq	$2, 0($4)
	jr	$31
	nop
	.end test_qfsrv0

	.globl test_plzcw
	.ent test_plzcw
test_plzcw:
	lw	$2, 0($5)
	plzcw	$2, $2
	sw	$2, 0($4)
	jr	$31
	nop
	.end test_plzcw
