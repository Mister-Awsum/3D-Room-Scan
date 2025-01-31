// COMPENG 2DX3 Project Deliverable 2

//  Written by Deyontae Patterson
//  March  29, 2024

#include <stdint.h>
#include <stdbool.h> 											// Include the header file for boolean support
#include "tm4c1294ncpdt.h"
#include "SysTick.h"
#include "PLL.h"
#include "uart.h"
#include "onboardLEDs.h"
#include "VL53L1X_api.h"
#include "StepperMotor.h"

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable

#define MAXRETRIES              5           // number of receive attempts before giving up

#define DELAY										1				// stepper motor delay

void I2C_Init(void)
{
  	SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           													// activate I2C0
  	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          												// activate port B
  	while((SYSCTL_PRGPIO_R&0x0002) == 0){};																		// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           																	// 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             																	// 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             																	// 5) enable digital I/O on PB2,3
	//    GPIO_PORTB_AMSEL_R &= ~0x0C;          																// 7) disable analog functionality on PB2,3

                                                                            // 6) configure PB2,3 as I2C
	//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      													// 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                       	// 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
	//    I2C0_MTPR_R = 0x3B;                                        						// 8) configure for 100 kbps clock
        
}

void VL53L1X_XSHUT(void)
{
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
    
}

void PortM_Init(void)											// For Push Buttons (Active High)
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R11;                 	
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R11) == 0){};      
		
	GPIO_PORTM_DIR_R = 0b00000000;       								      
  GPIO_PORTM_DEN_R = 0b00000001;
		
	return;
}

void PortN_Init(void)											// For Push Buttons (Active High)
{
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R12;                 	
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R12) == 0){};      
		
	GPIO_PORTN_DIR_R = 0x0F;       								      
  GPIO_PORTN_DEN_R = 0x0F;														
	return;
}

void PortH_Init(void)											// For Rotating Stepper
{	
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R7;		              
	while((SYSCTL_PRGPIO_R & SYSCTL_PRGPIO_R7) == 0){};	      
  
	GPIO_PORTH_DIR_R = 0x0F;														
	GPIO_PORTH_DEN_R = 0x0F;                        		
	return;
}

void PortF_Init(void)											// Student-unique LED (PF4)
{
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;                 	
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};					
		
	GPIO_PORTF_DIR_R = 0x10;															
	GPIO_PORTF_DEN_R = 0x10;															
	return;
}

uint16_t dev = 0x29;			//address of the ToF sensor as an I2C slave peripheral
int status=0;

int main(void)
{
	uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  	uint16_t wordData;
  	uint16_t Distance;
  	uint16_t SignalRate;
  	uint16_t AmbientRate;
  	uint16_t SpadNum; 
  	uint8_t RangeStatus;
  	uint8_t dataReady;

	// Stepper Motor Variables
	bool isClockwise = true;
	bool isRunning = false;
	int delay = 500;
	int process_time = 10;
	int PhaseAngle = 0;
	int runs;

	// Initialize
	PortM_Init();
	PortN_Init();
	PortF_Init();
	PortH_Init();
	SysTick_Init();
	PLL_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();

	// hello world!
	UART_printf("Program Begins\r\n");
	int mynumber = 1;
	sprintf(printf_buffer,"2DX ToF Program Studio Code %d\r\n",mynumber);
	UART_printf(printf_buffer);

	status = VL53L1_RdByte(dev, 0x010F, &byteData); //for model ID (0xEA)
	sprintf(printf_buffer,"Model_ID: 0x%x\r\n", byteData);
	UART_printf(printf_buffer);
	
	status = VL53L1_RdByte(dev, 0x0110, &byteData); //for module type (0xCC)
	sprintf(printf_buffer,"Module_type: 0x%x\r\n", byteData);
	UART_printf(printf_buffer);
	
	status = VL53L1_RdWord(dev, 0x010F, &wordData); //for both model ID and type
	sprintf(printf_buffer,"(Model_ID, Module_Type): 0x%x\r\n", wordData);
	UART_printf(printf_buffer);
	
	// Wait for device ToF booted
	while(sensorState==0)
	{
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }

	FlashAllLEDs();
	UART_printf("ToF Chip Booted!\r\n Please Wait...\r\n");
	
	status = VL53L1X_ClearInterrupt(dev);

	// Initialize the sensor with the default setting
	status = VL53L1X_SensorInit(dev);
	Status_Check("SensorInit", status);

	// Main Loop

	while(1)
	{
		// Square Wave For Prooving Bus Speed
		
		while((GPIO_PORTM_DATA_R & 0x01) != 0x00)
		{
			FlashLED1(1);
			GPIO_PORTN_DATA_R = 0b00001000;
			SysTick_Wait(12000000);

			GPIO_PORTN_DATA_R = 0b00000000;
			SysTick_Wait(12000000);
		}
		
		// Button press activates Loop
		
		while((GPIO_PORTM_DATA_R & 0x01) == 0)
		{
			FlashLED3(1);	// Wait for Button Release
		}

		runs = 0; // Prepares for multiple runs
		
		while(runs<3)
		{
			FlashLED3(1);
			SysTick_Wait10ms(process_time);

			PhaseAngle = 0;
			isRunning = true;

			// Enable Ranging

			status = VL53L1X_StartRanging(dev);

			// Loops until full rotation

			while(PhaseAngle != 512)
			{
				// Reset to Home
				
				if((GPIO_PORTM_DATA_R & 0x01)==0)
				{
					SysTick_Wait10ms(process_time);
					runs = 3;
					break;
				}

				// Spin Motor
				
				if(isRunning)
				{
					TurnMotor(isClockwise, delay);
					PhaseAngle++;
					
					// 22.5 Degree Step Angle
					
					if(PhaseAngle % 32 == 0)
					{
						// Blink LED
						FlashLED3(1);

						// Wait for ToF to be ready

						while (dataReady == 0)
						{
							status = VL53L1X_CheckForDataReady(dev, &dataReady);
							FlashLED3(1);
							VL53L1_WaitMs(dev, 5);
						}

						dataReady = 0;

						// Read Data

						status = VL53L1X_GetRangeStatus(dev, &RangeStatus);
						status = VL53L1X_GetDistance(dev, &Distance);
						status = VL53L1X_GetSignalRate(dev, &SignalRate);
						status = VL53L1X_GetAmbientRate(dev, &AmbientRate);
						status = VL53L1X_GetSpadNb(dev, &SpadNum);
				
						FlashLED3(1);

						// Clear Interrupt

						status = VL53L1X_ClearInterrupt(dev);

						// print the resulted readings to UART

						sprintf(printf_buffer,"%u, %u \r\n", RangeStatus, Distance);
						UART_printf(printf_buffer);
						SysTick_Wait10ms(50);
					}
				}
			}

			// Disable Ranging

			VL53L1X_StopRanging(dev);
			
			// Returns home
				
			SysTick_Wait10ms(100);
			ReturnHome(isClockwise, PhaseAngle);
			runs++;
		}
	}
}