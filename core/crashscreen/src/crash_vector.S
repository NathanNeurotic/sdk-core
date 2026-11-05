/* Level 1 exception entry for the crash screen. Registers are saved into
   __crash_frame and control moves to the crash stack; it never returns. */

    .set push
    .set noreorder
    .set noat
    .bss
    .align  4
    .global __crash_wide
__crash_wide:
    .space  512
    .text
    .global __crash_vector
    .ent __crash_vector
__crash_vector:
    nop
    nop
    la      $27, __crash_wide
    sq      $1, 16($27)
    sq      $2, 32($27)
    sq      $3, 48($27)
    sq      $4, 64($27)
    sq      $5, 80($27)
    sq      $6, 96($27)
    sq      $7, 112($27)
    sq      $8, 128($27)
    sq      $9, 144($27)
    sq      $10, 160($27)
    sq      $11, 176($27)
    sq      $12, 192($27)
    sq      $13, 208($27)
    sq      $14, 224($27)
    sq      $15, 240($27)
    sq      $16, 256($27)
    sq      $17, 272($27)
    sq      $18, 288($27)
    sq      $19, 304($27)
    sq      $20, 320($27)
    sq      $21, 336($27)
    sq      $22, 352($27)
    sq      $23, 368($27)
    sq      $24, 384($27)
    sq      $25, 400($27)
    sq      $28, 448($27)
    sq      $29, 464($27)
    sq      $30, 480($27)
    sq      $31, 496($27)
    la      $26, __crash_frame
    sw      $1, 4($26)
    sw      $2, 8($26)
    sw      $3, 12($26)
    sw      $4, 16($26)
    sw      $5, 20($26)
    sw      $6, 24($26)
    sw      $7, 28($26)
    sw      $8, 32($26)
    sw      $9, 36($26)
    sw      $10, 40($26)
    sw      $11, 44($26)
    sw      $12, 48($26)
    sw      $13, 52($26)
    sw      $14, 56($26)
    sw      $15, 60($26)
    sw      $16, 64($26)
    sw      $17, 68($26)
    sw      $18, 72($26)
    sw      $19, 76($26)
    sw      $20, 80($26)
    sw      $21, 84($26)
    sw      $22, 88($26)
    sw      $23, 92($26)
    sw      $24, 96($26)
    sw      $25, 100($26)
    sw      $27, 108($26)
    sw      $28, 112($26)
    sw      $29, 116($26)
    sw      $30, 120($26)
    sw      $31, 124($26)
    sw      $0, 104($26)
    sw      $0, 0($26)
    mfc0    $8, $12
    mfc0    $9, $13
    mfc0    $10, $14
    mfc0    $11, $8
    sw      $8, 128($26)
    sw      $9, 132($26)
    sw      $10, 136($26)
    sw      $11, 140($26)
    la      $29, __crash_stack + 16384
    la      $4, __crash_frame
    j       __crash_cpu_exception
    nop
    .end __crash_vector
    .set pop

/* Re-raises the saved exception: restores EPC, Status and all registers, then returns to the faulting instruction. */
    .set push
    .set noreorder
    .set noat
    .text
    .global __crash_chain
    .ent __crash_chain
__crash_chain:
    lw      $8, 136($4)
    lw      $9, 128($4)
    mtc0    $8, $14
    mtc0    $9, $12
    sync.p
    la      $26, __crash_wide
    lq      $1, 16($26)
    lq      $2, 32($26)
    lq      $3, 48($26)
    lq      $4, 64($26)
    lq      $5, 80($26)
    lq      $6, 96($26)
    lq      $7, 112($26)
    lq      $8, 128($26)
    lq      $9, 144($26)
    lq      $10, 160($26)
    lq      $11, 176($26)
    lq      $12, 192($26)
    lq      $13, 208($26)
    lq      $14, 224($26)
    lq      $15, 240($26)
    lq      $16, 256($26)
    lq      $17, 272($26)
    lq      $18, 288($26)
    lq      $19, 304($26)
    lq      $20, 320($26)
    lq      $21, 336($26)
    lq      $22, 352($26)
    lq      $23, 368($26)
    lq      $24, 384($26)
    lq      $25, 400($26)
    lq      $28, 448($26)
    lq      $29, 464($26)
    lq      $30, 480($26)
    lq      $31, 496($26)
    eret
    nop
    .end __crash_chain
    .set pop

/* Leaves exception mode by eret into fn with interrupts enabled. */
    .set push
    .set noreorder
    .set noat
    .text
    .global __crash_leave
    .ent __crash_leave
__crash_leave:
    mtc0    $4, $14
    mfc0    $8, $12
    lui     $9, 1
    ori     $9, $9, 3
    or      $8, $8, $9
    mtc0    $8, $12
    sync.p
    eret
    nop
    .end __crash_leave
    .set pop
