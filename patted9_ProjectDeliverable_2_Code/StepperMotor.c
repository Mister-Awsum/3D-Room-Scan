#include "StepperMotor.h"

void TurnMotor(bool Direction, uint32_t delay)
{
	if(Direction)
	{
		GPIO_PORTH_DATA_R = 0x03;
		SysTick_Wait1us(delay);											
		GPIO_PORTH_DATA_R = 0x06;
		SysTick_Wait1us(delay);
		GPIO_PORTH_DATA_R = 0x0C;
		SysTick_Wait1us(delay);
		GPIO_PORTH_DATA_R = 0x09;
		SysTick_Wait1us(delay);
	}

	else
	{
		GPIO_PORTH_DATA_R = 0x09;
		SysTick_Wait1us(delay);											
		GPIO_PORTH_DATA_R = 0x0C;
		SysTick_Wait1us(delay);
		GPIO_PORTH_DATA_R = 0x06;
		SysTick_Wait1us(delay);
		GPIO_PORTH_DATA_R = 0x03;
		SysTick_Wait1us(delay);
	}
}

void ReturnHome(bool Direction, int CurrentPos)
{
	
	if(Direction)
	{
		while(CurrentPos)
		{
			TurnMotor(!Direction, 500);
			CurrentPos--;
		}
	}

	else
	{
		while(CurrentPos)
		{
			TurnMotor(Direction, 500);
			CurrentPos--;
		}
	}
}
