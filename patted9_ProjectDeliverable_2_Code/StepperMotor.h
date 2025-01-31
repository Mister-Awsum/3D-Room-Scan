#include <stdint.h>
#include <stdbool.h>
#include "SysTick.h"
#include "tm4c1294ncpdt.h"

void TurnMotor(bool Direction, uint32_t delay);
void ReturnHome(bool Direction, int CurrentPos);