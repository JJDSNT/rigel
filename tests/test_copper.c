#include "agnus/copper.h"

int main(void)
{
    copper_state_t copper = { 123 };
    copper_reset(&copper);
    return copper.program_counter == 0 ? 0 : 1;
}
