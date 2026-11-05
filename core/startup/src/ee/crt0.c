/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003  Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/

#include <kernel.h>
#include <stdio.h>
#include <stdint.h>
#include <stdnoreturn.h>  // noreturn
#include <string.h>
#include <startup.h>

extern char* _end;
extern char* _heap_size;
extern char* _fbss;
extern char* _stack;
extern char* _stack_size;

__attribute__((weak)) void _ps2sdk_memory_init();
__attribute__((weak)) void _init();
__attribute__((weak)) void _fini();

// Real, confirmed bug (2026-09-07): __attribute__((constructor)) never ran
// anywhere in this SDK, on real hardware or any emulator - "if (_init)
// _init();" below was always a no-op. _init/__libc_init_array are
// normally provided by gcc's own crtbegin.o/crtend.o, which -nostartfiles
// (required here - this file *is* the startup code) never links. This
// compiler emits the legacy .ctors section (confirmed via objdump - not
// .init_array), so crtbegin.o's own __do_global_ctors_aux is what would
// have walked it; without that object, nothing does. __CTOR_LIST__/
// __CTOR_END__ (provided by the linkfile, see its own comment) bound the
// section directly, sidestepping the need for crtbegin.o's own -1
// start-sentinel/0 end-terminator convention entirely. Real GCC .ctors
// calling order is reverse-array (highest address/lowest priority
// first), matching __do_global_ctors_aux's own real behavior.
extern void (*__CTOR_LIST__[])(void);
extern void (*__CTOR_END__[])(void);

// clang emits .init_array instead of .ctors for global/static C++
// constructors - see the linkfile's own comment on __init_array_start/
// __init_array_end. Unlike .ctors (walked in reverse), .init_array runs
// in forward array order per the Itanium C++ ABI.
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

// Real, confirmed bug: any global/static C++ object with a non-trivial
// destructor fails to link with "undefined reference to `__dso_handle`".
// GCC emits a __cxa_atexit(dtor, obj, &__dso_handle) call at that
// object's construction point; __dso_handle is normally defined by
// gcc's own crtbegin.o, which -nostartfiles (required here - this file
// *is* the startup code, see above) never links. A statically-linked
// PS2 ELF is always exactly one "module" (no dlclose, no shared
// objects), so the value only needs to be a stable, unique address -
// same minimal fix used by other no-crtbegin bare-metal targets
// (avr-libc and similar). weak so a real crtbegin.o's own definition
// would still win if some future path ever links one instead.
void *__dso_handle __attribute__((weak)) = &__dso_handle;

static void _call_global_ctors(void)
{
    int count = __CTOR_END__ - __CTOR_LIST__;
    for (int i = count - 1; i >= 0; i--)
        __CTOR_LIST__[i]();

    int init_array_count = __init_array_end - __init_array_start;
    for (int i = 0; i < init_array_count; i++)
        __init_array_start[i]();
}

void _libcglue_args_parse(int argc, char** argv);
void _libcglue_init();
void _libcglue_deinit();
int main(int argc, char** argv);

// __attribute__((used)): _main is only reachable via the raw "j _main"
// below, not a real C call the compiler can see - without this it gets
// optimized away as dead code before the asm ever runs. Real linker
// relocation from that jump keeps it past --gc-sections regardless.
static void _main() __attribute__((used));

static struct sargs args;
static struct sargs_start *args_start;

/*
 * First function to be called by the loader
 * This function sets up the stack and heap.
 * DO NOT USE THE STACK IN THIS FUNCTION!
 */
void __start(struct sargs_start *pargs)
{
    __asm__ __volatile__(
        "# Clear bss area       \n"
        ".set noat              \n"
        "la   $2, _fbss         \n"
        "la   $3, _end          \n"
        "1:                     \n"
        "sltu   $1, $2, $3      \n"
        "beq   $1, $0, 2f       \n"
        "nop                    \n"
        "sq   $0, ($2)          \n"
        "addiu   $2, $2, 16     \n"
        "j   1b                 \n"
        "nop                    \n"
        "2:                     \n"
        "                       \n"
        "# Save first argument  \n"
        "sw     %1, %0          \n"
        "                       \n"
        "# SetupThread          \n"
        "la     $4, _gp         \n"
        "la     $5, _stack      \n"
        "la     $6, _stack_size \n"
        "la     $7, args	    \n"
        "la     $8, ExitThread  \n"
        "move   $gp, $4         \n"
        "addiu  $3, $0, 60      \n"
        "syscall                \n"
        "move   $sp, $2         \n"
        "                       \n"
        "# Jump to _main        \n"
        "j      _main           \n"
        ".set at              \n"
        : /* No outputs. */
        : "m"(args_start), "r"(pargs)
        : "1", "2", "3", "4", "5", "6", "7", "8");
}

/*
 * Intermediate function between _start and main, stack can be used as normal.
 */
static void _main()
{
    int retval;
    struct sargs *pa;

    // initialize heap
    SetupHeap(&_end, (int)&_heap_size);

    // writeback data cache
    FlushCache(0);

    // Capability to override 32MiB on DESR/DVR models
    // NOTE: this call can restart the application
    if (_ps2sdk_memory_init)
        _ps2sdk_memory_init();
    
    // Initialize the kernel (Apply necessary patches).
    _InitSys();

    // Use arguments sent through start if sent (by ps2link for instance)
    pa = &args;
    if (args.argc == 0 && args_start != NULL && args_start->args.argc != 0)
        pa = &args_start->args;

    // call libcglue argument parsing
    _libcglue_args_parse(pa->argc, pa->argv);

    // initialize libcglue
    _libcglue_init();

    // Enable interruts
    EI();

    // call global constructors - see _call_global_ctors's own comment
    // for why this replaces the old (always-dead) "if (_init) _init();"
    _call_global_ctors();

    // call main
    retval = main(pa->argc, pa->argv);

    // call global destructors (weak)
    if (_fini)
        _fini();

    // uninitialize libcglue
    _libcglue_deinit();

    // Exit
    Exit(retval);
}

noreturn void _exit(int status)
{
    // call global destructors (weak)
    if (_fini)
        _fini();

    // uninitialize libcglue
    _libcglue_deinit();

    // Exit
    Exit(status);
}
