/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

/**
 * @file
 * @ingroup core_loadcore
 * Convienence macros for IRX modules.
 */

#ifndef __IRX_H__
#define __IRX_H__

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IRX_VER(major, minor)	((((major) & 0xff) << 8) + ((minor) & 0xff))

struct irx_id {
	const char *n;
	u16 v;
};

#define IRX_ID(name, major, minor) \
struct irx_id _irx_id = { \
	name, IRX_VER(major, minor) \
};

/*
 * Module imports
 */
#define IMPORT_MAGIC	0x41e00000

struct irx_import_table
{
	u32	magic;
	struct irx_import_table *next;
	u16	version;
	u16	mode;
	char	name[8];
	void	*stubs[];
} __attribute ((packed));

struct irx_import_stub
{
    u32 jump;
    u16 fno;
    u16 ori_zero;
} __attribute ((packed));

/*
 * Ugly, yet functional.
 */
#define STR(val) #val
#define XSTR(val) STR(val)

/* The name field is 8 bytes: longer names are cut, shorter ones padded with zeros. */
#define IRX_ASM_NAME(modname) \
	".set\t.Lirx_name_len, 0\n\t" \
	".irpc\tc, " #modname "\n\t" \
	".if .Lirx_name_len < 8\n\t" \
	".ascii\t\"\\c\"\n\t" \
	".endif\n\t" \
	".set\t.Lirx_name_len, .Lirx_name_len + 1\n\t" \
	".endr\n\t" \
	".if .Lirx_name_len < 8\n\t" \
	".fill\t8 - .Lirx_name_len, 1, 0\n\t" \
	".endif\n"

#ifdef __clang__
/* The GNU assembler reads the section name below as ".text" plus a comment. Clang takes it literally and
   puts the table header in a section of its own, away from the stubs that follow it, so write it as assembly. */
#define DECLARE_IMPORT_TABLE(modname, major, minor) \
	__asm__ (".section\t.text\n\t" \
		".balign\t4\n\t" \
		".word\t" XSTR(IMPORT_MAGIC) ", 0\n\t" \
		".short\t" XSTR(IRX_VER(major, minor)) ", 0\n\t" \
		IRX_ASM_NAME(modname));
#else
#define DECLARE_IMPORT_TABLE(modname, major, minor)	\
static struct irx_import_table _imp_##modname 		\
	__attribute__((section(".text\n\t#"), unused))= {	\
	magic: IMPORT_MAGIC, version: IRX_VER(major, minor),	\
	name: #modname, };
#endif

// .word 0x03e00008 == jr $ra (return immediately), this value will be patched later
#define DECLARE_IMPORT(ord, name) \
	__asm__ (".section\t.text\n\t"		\
		".globl\t"#name"\n\t"#name":\n\t"	\
		".word 0x03e00008\n\t"			\
		".word " STR(0x24000000|ord));

#define END_IMPORT_TABLE \
	__asm__ (".section\t.text\n\t.word\t0, 0");

/*
 * Module exports
 *
 * I'm not sure why GCC won't generate the fptrs initializers, so I fell back
 * to the above approach to generate the fptrs themselves.  Cleaner solutions
 * requested.
 */
#define EXPORT_MAGIC	0x41c00000

void __attribute__((unused)) _retonly();

struct irx_export_table {
	u32	magic;
	struct irx_export_table *next;
	u16	version;
	u16	mode;
	u8	name[8];
	void	*fptrs[];
};

#ifdef __clang__
#define DECLARE_EXPORT_TABLE(modname, major, minor) \
extern struct irx_export_table _exp_##modname; \
	__asm__ (".section\t.text\n\t" \
		".balign\t4\n\t" \
		".globl\t_exp_" #modname "\n\t" \
		".type\t_exp_" #modname ", @object\n" \
		"_exp_" #modname ":\n\t" \
		".word\t" XSTR(EXPORT_MAGIC) ", 0\n\t" \
		".short\t" XSTR(IRX_VER(major, minor)) ", 0\n\t" \
		IRX_ASM_NAME(modname));
#else
#define DECLARE_EXPORT_TABLE(modname, major, minor)	\
struct irx_export_table _exp_##modname			\
	__attribute__((section(".text\n\t#"), unused)) = {	\
	magic: EXPORT_MAGIC, version: IRX_VER(major, minor),	\
	name: #modname, };
#endif

#define DECLARE_EXPORT(fptr) \
	__asm__ (".section\t.text\n\t.word\t" STR(fptr));

#define END_EXPORT_TABLE  __asm__ (".section\t.text\n\t.word\t0");

#ifdef __cplusplus
}
#endif

#endif /* __IRX_H__ */
