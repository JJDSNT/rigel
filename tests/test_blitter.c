#include "agnus/blitter.h"

int main(void)
{
    blitter_state_t blitter = { 0 };
    blitter_start(&blitter);
    return blitter.busy == 1 ? 0 : 1;
}
