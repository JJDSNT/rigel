#include "agnus/beam.h"

int main(void)
{
    beam_state_t beam = { 0, 0 };
    beam_step(&beam, 4);
    return beam.hpos == 4 ? 0 : 1;
}
