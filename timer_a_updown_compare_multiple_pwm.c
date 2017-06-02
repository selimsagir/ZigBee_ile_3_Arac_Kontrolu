/*
 * -------------------------------------------
 *    MSP432 DriverLib - v3_10_00_09 
 * -------------------------------------------
 *
 * --COPYRIGHT--,BSD,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*******************************************************************************
 * MSP432 PWM TA1.1-2, Up/Down Mode, DCO SMCLK
 *
 * Description: This program generates two PWM outputs on P2.2,3 using
 * Timer1_A configured for up/down mode. The value in CCR0, 128, defines the
 * PWM period/2 and the values in CCR1 and CCR2 the PWM duty cycles. Using
 * ~1.045MHz SMCLK as TACLK, the timer period is ~233us with a 75% duty cycle
 * on P7.7 and 25% on P7.6.
 * SMCLK = MCLK = TACLK = default DCO 3MHz
 *
 *         MSP432P401
 *      -------------------
 *  /|\|                   |
 *   | |                   |
 *   --|RST                |
 *     |                   |
 *     |         P7.7/TA1.1|--> CCR1 - 75% PWM
 *     |         P7.6/TA1.2|--> CCR2 - 25% PWM
 *
 * Author: Timothy Logan
*******************************************************************************/
/* DriverLib Includes */
#include "driverlib.h"

// PC UART PROJE AYARLARINDA TANIMLANMIÞTIR.

#ifdef PC_UART
#define UART_BASE EUSCI_A0_BASE
#else
#define UART_BASE EUSCI_A2_BASE
#endif


/* Application Defines */
#define TIMER_PERIOD 1000
#define DUTY_CYCLE1 10
#define DUTY_CYCLE2 10
//#define duration_straight 100000
//#define duration_outer_turn 100000
//#define duration_turn 20000
//#define pwm_forward   100
//#define pwm_backward -100

#define POS_MASTER 0
#define POS_LEFT 1
#define POS_RIGHT 2

int vehicle_pos;

int duration_straight = 100000;
int duration_outer_turn = 100000;
int duration_turn = 20000;



Timer_A_UpDownModeConfig upDownConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock SOurce
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/1 = 3MHz
        TIMER_PERIOD,                           // 127 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE,    // Disable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value

};


/* Timer_A Compare Configuration Parameter  (PWM1) */
const Timer_A_CompareModeConfig compareConfig_PWM1 =
{
        TIMER_A_CAPTURECOMPARE_REGISTER_1,          // Use CCR1
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
		TIMER_A_OUTPUTMODE_TOGGLE_RESET,              // Toggle output but
        DUTY_CYCLE1                                 // 32 Duty Cycle
};

/* Timer_A Compare Configuration Parameter (PWM2) */

const Timer_A_CompareModeConfig compareConfig_PWM2 =
{
        TIMER_A_CAPTURECOMPARE_REGISTER_2,          // Use CCR2
        TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,   // Disable CCR interrupt
		TIMER_A_OUTPUTMODE_TOGGLE_RESET,              // Toggle output but
        DUTY_CYCLE2                                 // 96 Duty Cycle
};


const eUSCI_UART_Config uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        78,                                     // BRDIV = 78				/* Sayfa 724'deki tablodan bakarak 115200 baudrate ayarlandi*/
        2,                                       // UCxBRF = 2
        0x0,                                       // UCxBRS = 0			/*115200 BAUDRATE*/
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // LSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling

};
void check_pos(void){

	MAP_GPIO_setAsInputPin(GPIO_PORT_P7, GPIO_PIN1);
	MAP_GPIO_setAsInputPin(GPIO_PORT_P7, GPIO_PIN2);
	if((MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN1))==1)
	{
		vehicle_pos = POS_MASTER;
	}
	else if(MAP_GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN2)==1)
	{
		vehicle_pos = POS_LEFT;
	}
	else
	{
		vehicle_pos = POS_RIGHT;
	}


}

void stop_motor(void)
{
		MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN6 | GPIO_PIN7);
		MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN6 | GPIO_PIN7);
		MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN4 | GPIO_PIN7);
		MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN4 | GPIO_PIN7);
};

void start_motor(int motor1, int motor2, int duration)
{
    	/* Setting P7.7 and P7.6 and peripheral outputs for CCR */

		MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7,
	            GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION);

		MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P7,
	        	            GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

        if(motor1 > 0)
        {

			MAP_Timer_A_setCompareValue(TIMER_A1_BASE , TIMER_A_CAPTURECOMPARE_REGISTER_1, motor1);
	    	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2  ,  GPIO_PIN4);

        }
        else // reverse
        {
        	MAP_Timer_A_setCompareValue(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_1, TIMER_PERIOD + motor1);
        	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2,  GPIO_PIN4);
        }

        if(motor2 > 0)
        {
	    	MAP_Timer_A_setCompareValue(TIMER_A1_BASE , TIMER_A_CAPTURECOMPARE_REGISTER_2, motor2);
	    	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2  ,  GPIO_PIN7);
        }
        else // reverse
        {
	    	MAP_Timer_A_setCompareValue(TIMER_A1_BASE , TIMER_A_CAPTURECOMPARE_REGISTER_2, TIMER_PERIOD + motor2);
	    	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2  ,  GPIO_PIN7);
        }

	    // start timer
	    MAP_Timer32_setCount(TIMER32_BASE, duration);
	    MAP_Timer32_enableInterrupt(TIMER32_BASE);
	    MAP_Timer32_startTimer(TIMER32_BASE, true);

}


void init_motor_pwm(void)
{
		/* Configuring Timer_A1 for UpDown Mode and starting */
    	MAP_Timer_A_configureUpDownMode(TIMER_A1_BASE, &upDownConfig);
    	MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UPDOWN_MODE);

    	/* Initialize compare registers to generate PWM1 */
    	MAP_Timer_A_initCompare(TIMER_A1_BASE, &compareConfig_PWM1);

    	/* Initialize compare registers to generate PWM2 */
    	MAP_Timer_A_initCompare(TIMER_A1_BASE, &compareConfig_PWM2);

    	stop_motor();
}

int pwm_forward = 100;
int pwm_backward = -100;
int pwm_in_wheel = 100;
int pwm_out_wheel = 100;

void go_forward(void)
{
		start_motor(pwm_forward ,pwm_forward, duration_straight );
}
void go_backward(void)
{
		start_motor(pwm_backward,pwm_backward, duration_straight );
}
void turn_right(void)
{
	if(vehicle_pos == POS_MASTER) //master
		start_motor(pwm_forward , pwm_backward  , duration_turn);
	else if(vehicle_pos == POS_LEFT) //left
		start_motor( pwm_in_wheel, pwm_out_wheel  , duration_outer_turn);
	else //right
		start_motor( -(1)*pwm_out_wheel, -(1)*pwm_in_wheel  , duration_outer_turn);
}
void turn_left(void)
{

		if(vehicle_pos == POS_MASTER) //master
			start_motor(pwm_backward , pwm_forward  , duration_turn);
		else if(vehicle_pos == POS_LEFT) //left
			start_motor( (-1)*pwm_in_wheel, (-1)*pwm_out_wheel  , duration_outer_turn);
		else //right
			start_motor( pwm_out_wheel, pwm_in_wheel  , duration_outer_turn);

}
/*void right_vehicle(void)
{
		start_motor( pwm_out_wheel, pwm_in_wheel  , duration_outer_turn);
}
void left_vehicle(void)
{
		start_motor( pwm_in_wheel, pwm_out_wheel  , duration_outer_turn);
}*/
bool receiving_command = false;
bool received_direction = false;
int direction = 0;

void control_motor(void)
{
		uint8_t data = MAP_UART_receiveData(UART_BASE);

		if (receiving_command) // first byte
		{
			if (!received_direction) // second byte
			{
				direction = data;
				received_direction = true;
				return;
			}
			else
			{
				if (direction == 1) // 3rd byte
				{
					pwm_forward = data*10;
				}
				else if(direction == 2)
				{
					pwm_backward = -data*10;
				}
				else if(direction == 3)
				{
					pwm_in_wheel = data*10;
				}
				else if(direction == 4)
				{
					pwm_out_wheel = data*10;
				}
				else if(direction == 5)
				{
					duration_outer_turn = data*1000;
				}
				else if(direction == 6)
				{
					duration_turn = data*1000;
				}
				else if(direction == 7)
				{
					duration_straight = data*1000;
				}
				receiving_command = false;
				received_direction = false;
				return;
			}
		}

		if (data == 6)
		{
			receiving_command = true;
			return;
		}

	    if(data == 0x88)  //DUR
		{
	    stop_motor();
		}
	    else if(data == 0x99)   //ILERI
		{
	    go_forward();
		}
	    else if(data == 3)   //GERI
		{
	    go_backward();
		}
	    else if(data == 4)   //SAG
		{
	    turn_right();
		}
	    else if(data == 5)   //SOL
		{
	    turn_left();
		}

	    MAP_UART_transmitData(UART_BASE, data);

}
void uart_function(void)
{
		/* Selecting P1.2 and P1.3 in UART mode */
    	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
          GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    	/* Configuring UART Module */
    	MAP_UART_initModule(UART_BASE, &uartConfig);

    	/* Enable UART module */
    	MAP_UART_enableModule(UART_BASE);

    	/* Enabling interrupts */
    	MAP_UART_enableInterrupt(UART_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
#ifdef PC_UART
    	MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
#else
    	MAP_Interrupt_enableInterrupt(INT_EUSCIA2);
#endif

    	MAP_Interrupt_enableSleepOnIsrExit();
    	MAP_Interrupt_enableMaster();
}
void init_timer_2(void)
{
	    MAP_Timer32_initModule(TIMER32_BASE, TIMER32_PRESCALER_256, TIMER32_32BIT,
	            TIMER32_PERIODIC_MODE);

	    MAP_Interrupt_enableInterrupt(INT_T32_INT1);
}

int checkUart=0;

int main(void)
{
    /* Stop WDT  */
    MAP_WDT_A_holdTimer();

    /* Setting DCO to 12MHz */
    CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);
    uart_function();

    init_timer_2();
    init_motor_pwm();
    check_pos();
    //start_motor();

    while(1)
    {

    	if(checkUart==1)

    	{
    		control_motor();
    		checkUart=0;

    	}

    }

    /* Sleeping when not in use */
   /*while (1)
    {
        MAP_PCM_gotoLPM0();
    }*/
}

#ifdef PC_UART
void EUSCIA0_IRQHandler(void)
#else
void EUSCIA2_IRQHandler(void)
#endif
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(UART_BASE);

    MAP_UART_clearInterruptFlag(UART_BASE, status);

    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
    {
    	checkUart=1;
    }
}

/* Timer32 ISR */
void T32_INT1_IRQHandler(void)
{
	stop_motor();
    MAP_Timer32_clearInterruptFlag(TIMER32_BASE);
    MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P6, GPIO_PIN1);
}

