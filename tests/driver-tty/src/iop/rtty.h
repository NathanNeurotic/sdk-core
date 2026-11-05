#ifndef RTTY_H
#define RTTY_H

/* Discard everything buffered so far. */
#define RTTY_IOCTL_CLEAR 0x52540001
/* Make rtty: the IOP's stdout while still passing the output on to the previous tty:.
   Returns the descriptor stdout was reopened on (1). */
#define RTTY_IOCTL_ATTACH_CONSOLE 0x52540002

#endif
