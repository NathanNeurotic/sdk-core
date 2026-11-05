/**
 * @file
 * @ingroup core_startup
 * Crash screen: reports an uncaught C++ exception or a CPU exception on screen and on the DECI2 console.
 */

#ifndef __CRASHSCREEN_H__
#define __CRASHSCREEN_H__

#ifdef __cplusplus
extern "C" {
#endif

/** Called with the full report text after it is drawn, before the program halts. */
typedef void (*crashscreen_hook_t)(const char *report);

/** Installs the CPU exception handlers and the std::terminate handler. Done by the startup code. */
void crashscreen_install(void);

/** Restores the handlers that were in place before crashscreen_install. */
void crashscreen_remove(void);

/** Sets a function called with the report once it has been shown. Pass NULL to clear it. */
void crashscreen_set_hook(crashscreen_hook_t hook);

/** When enabled, a CPU exception is handed to the handler that was installed before crashscreen_install (for example ps2link's) after the report is shown. */
void crashscreen_set_chain(int enable);

/** Shows the crash screen with the given reason and halts. Does not return. */
void crashscreen_abort(const char *reason) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif
