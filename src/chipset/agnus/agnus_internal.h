#ifndef RIGEL_AGNUS_INTERNAL_H
#define RIGEL_AGNUS_INTERNAL_H

/* Forward declarations shared across agnus sub-modules.
 * Public types live in agnus_state.h; put only internal linkage here. */

typedef struct RigelContext RigelContext;
typedef struct RigelAgnus   RigelAgnus;

/* Convenience accessor — avoids including the full context header in leaf modules */
RigelAgnus *agnus_from_ctx(RigelContext *ctx);

#endif
