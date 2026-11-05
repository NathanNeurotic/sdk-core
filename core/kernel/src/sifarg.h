#ifndef SIFARG_H
#define SIFARG_H

// The IOP DMAs an RPC reply straight into the caller's buffer, which must own whole 64-byte cache lines.
// Declares `name` as a pointer to such a buffer of the given type.
#define SIF_ARG(type, name)                                                                  \
    union {                                                                                  \
        type v;                                                                              \
        char pad[sizeof(type) > 64 ? sizeof(type) : 64];                                     \
    } __attribute__((aligned(64))) name##_buf;                                               \
    type *const name = &name##_buf.v

#endif
