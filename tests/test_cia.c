#include "cia/cia_timer.h"

int main(void)
{
    cia_timer_state_t timer = { 10, 0 };
    cia_timer_reload(&timer);
    return timer.counter == 10 ? 0 : 1;
}
