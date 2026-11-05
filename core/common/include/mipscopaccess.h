/**
 * @file
 * @ingroup core_common
 * MIPS COP coprocessor access
 */

#ifndef __MIPSCOPACCESS_H__
#define __MIPSCOPACCESS_H__

#include <tamtypes.h>

/** Programmable register to select TLB entry for reading or writing (purpose: MMU) */
#define COP0_REG_Index 0
/** Pseudo-random counter for TLB replacement (purpose: MMU) */
#define COP0_REG_Random 1
/** Low half of TLB entry for even PFN (Physical page number) (purpose: MMU) */
#define COP0_REG_EntryLo0 2
/** Low half of TLB entry for odd PFN (Physical page number) (purpose: MMU) */
#define COP0_REG_EntryLo1 3
/** Pointer to kernel virtual PTE table (purpose: Exception) */
#define COP0_REG_Context 4
/** Mask that sets the TLB page size (purpose: MMU) */
#define COP0_REG_PageMask 5
/** Number of wired TLB entries (purpose: MMU) */
#define COP0_REG_Wired 6
/** Bad virtual address (purpose: Exception) */
#define COP0_REG_BadVAddr 8
/** Timer compare (purpose: Exception) */
#define COP0_REG_Count 9
/** High half of TLB entry(Virtual page number and ASID) (purpose: MMU) */
#define COP0_REG_EntryHi 10
/** Timer compare (purpose: Exception) */
#define COP0_REG_Compare 11
/** Processor Status Register (purpose: Exception) */
#define COP0_REG_Status 12
/** Cause of the last exception taken (purpose: Exception) */
#define COP0_REG_Cause 13
/** Exception Program Counter (purpose: Exception) */
#define COP0_REG_EPC 14
/** Processor Revision Identifier (purpose: MMU) */
#define COP0_REG_PRId 15
/** Configuration Register (purpose: MMU) */
#define COP0_REG_Config 16
/** Bad Physical Address (purpose: Exception) */
#define COP0_REG_BadPAddr 23
/** This is used for Debug function (purpose: Debug) */
#define COP0_REG_Debug 24
/** Performance Counter and Control Register (purpose: Exception) */
#define COP0_REG_Perf 25
/** Cache Tag register(low bits) (purpose: MMU) */
#define COP0_REG_TagLo 28
/** Cache Tag register(high bits) (purpose: MMU) */
#define COP0_REG_TagHi 29
/** Error Exception Program Counter (purpose: Exception) */
#define COP0_REG_ErrorPC 30

// cop/idx need to be real preprocessor tokens (not just compile-time-
// constant expressions) so # can stringize them directly into the
// mnemonic/register text - real GCC could fold get_mips_cop_reg's old
// "i"-constrained function-parameter version lazily, after always_inline
// substituted each call site's constant arguments, regardless of the
// caller's own -O level. Clang instead validates "i" constraints during
// Sema, before any constant-folding pass runs, so a caller built at -O0
// (e.g. sdk-core's eedebug package) hit "invalid operand for inline asm
// constraint 'i'" even though cop/idx are always real compile-time
// constants at every call site. Macros sidestep the whole question -
// this is why COP0_REG_* above became #defines instead of an enum, so
// they're valid ## arguments here.
// COP0_REG_* are themselves macros, so #cop/#idx need the classic
// two-layer stringize indirection (MIPSCOPACCESS_STR/_XSTR) - a bare
// "#idx" would stringize the literal argument text ("COP0_REG_PRId"),
// not its expansion ("15"); routing through one more macro layer forces
// idx to expand first.
#define MIPSCOPACCESS_STR(x) #x
#define MIPSCOPACCESS_XSTR(x) MIPSCOPACCESS_STR(x)

#define get_mips_cop_reg(cop, idx) \
	({ \
		u32 __val; \
		__asm__ __volatile__("mfc" MIPSCOPACCESS_XSTR(cop) "\t%0, $" MIPSCOPACCESS_XSTR(idx) "\n" : "=r"(__val)); \
		__val; \
	})

#define set_mips_cop_reg(cop, idx, val) \
	__asm__ __volatile__("mtc" MIPSCOPACCESS_XSTR(cop) "\t%0, $" MIPSCOPACCESS_XSTR(idx) "\n" :: "r"((u32)(val)))

#endif /* __MIPSCOPACCESS_H__ */
