#ifndef RIGEL_DENISE_INTERNAL_H
#define RIGEL_DENISE_INTERNAL_H

/* Forward declarations shared across Denise sub-modules.
 * Public types live in denise_state.h; put only internal linkage here. */

typedef struct RigelContext RigelContext;
typedef struct RigelDenise  RigelDenise;

/* Convenience accessor */
RigelDenise *denise_from_ctx(RigelContext *ctx);

#endif
