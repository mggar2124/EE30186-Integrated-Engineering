#include "EE30186.h"
#include "system.h"
#include "socal/socal.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define key0 0xE
#define key1 0xD
#define key2 0xB
#define key3 0x7

//Pointers to the hardware address of the required I/O and counter
volatile int *LEDs = (volatile int *)(ALT_LWFPGA_LED_BASE);
volatile int *Switches = (volatile int *)(ALT_LWFPGA_SWITCH_BASE);
volatile int *push_button = (volatile int *)(ALT_LWFPGA_KEY_BASE);
volatile int *GPIO_Port = (volatile int *)(ALT_LWFPGA_GPIO_1A_BASE);
volatile int *GPIO_Port_2 = (volatile int *)(ALT_LWFPGA_GPIO_1B_BASE);
volatile int *Counter = (volatile int *)(ALT_LWFPGA_COUNTER_BASE);
volatile int *Num_R = (volatile int *)(ALT_LWFPGA_HEXA_BASE);
volatile int *Num_L = (volatile int *)(ALT_LWFPGA_HEXB_BASE);

//*********************************************************************************

void pin_set(int bit_location, int is_high)
{

    if (is_high < 1)
        *GPIO_Port &= ~(1UL << bit_location);
    else
        *GPIO_Port |= (1UL << bit_location);
}

//*********************************************************************************

void LED_state(int duty_cycle)
{
    int LED_pos = duty_cycle / 10;
    *LEDs = 0xFFF << (10 - LED_pos);
}

//*********************************************************************************

int pin_get(int port_location)
{

    return (*GPIO_Port & (1 << port_location)) >> port_location;
}

//*********************************************************************************

int hex_get(int decimal_num)
{

    int Decimal2HexVal;

    switch (decimal_num)
    {
    case 0:
        Decimal2HexVal = 0x40;
        break;
    case 1:
        Decimal2HexVal = 0x79;
        break;
    case 2:
        Decimal2HexVal = 0x24;
        break;
    case 3:
        Decimal2HexVal = 0x30;
        break;
    case 4:
        Decimal2HexVal = 0x19;
        break;
    case 5:
        Decimal2HexVal = 0x12;
        break;
    case 6:
        Decimal2HexVal = 0x02;
        break;
    case 7:
        Decimal2HexVal = 0x78;
        break;
    case 8:
        Decimal2HexVal = 0x00;
        break;
    case 9:
        Decimal2HexVal = 0x18;
        break;
    }
    return Decimal2HexVal;
}

//*********************************************************************************

int get_size(int decimal_num)
{

    int count = 0;
    while (decimal_num > 0)
    {
        decimal_num /= 10;
        count += 1;
    }
    return count;
}

//*********************************************************************************

void delay(int delay_time)
{
    int previous_time = *Counter;
    int count_time = delay_time * 0xC350;
    while (abs(*Counter - previous_time) <= count_time)
        ;
}

//*********************************************************************************

void Display(int number_to_display)
{
    int val_r, val_l;
    val_r = 0xFFFFFFFF;
    val_l = 0xFFFF;

    if (number_to_display < 0)
    {
        number_to_display = -number_to_display;
        val_r = 0xFFFFFF3F;
    }

    int input = number_to_display;
    int input_cp = input;
    int shift = 8;

    if (input == 0)
    {
        val_l = 0xFFFF;
        val_r = 0xFFFFFF40;
    }

    int len_num = get_size(input);
    int single_digits, hex_raw_num, hex_num_post_shift;

    val_r = val_r << len_num * shift;
    if (len_num > 4)
    {
        val_l = val_l << (len_num - 4) * shift;
    }

    for (int i = 0; i < len_num; i++)
    {
        single_digits = input_cp % 10;
        input_cp /= 10;
        hex_raw_num = hex_get(single_digits);
        hex_num_post_shift = hex_raw_num << shift * i;
        if (hex_num_post_shift < 1)
        {
            hex_num_post_shift = hex_raw_num << shift * (i - 4);
            val_l = val_l | hex_num_post_shift;
        }
        else
        {
            val_r = val_r | hex_num_post_shift;
        }
    }

    *Num_R = val_r;
    *Num_L = val_l;
}

//*********************************************************************************

void Display_char(char str[], int fun)
{
    int val_l = 0xFFFF;
    int val_r = 0xFFFFFFFF;

    int letter;
    int val_on[2] = {0x2B, 0x40};
    int val_off[3] = {0xE, 0xE, 0x40};
    int val_open[4] = {0x48, 0x06, 0x0C, 0x40};

    //Display ON
    if (strlen(str) == 2)
    {
        val_r = val_r << strlen(str) * 8;
        for (int i = 0; i < strlen(str); i++)
        {
            letter = val_on[i] << 8 * i;
            val_r = val_r | letter;
        }
    }
    //Display OFF
    if (strlen(str) == 3)
    {
        val_r = val_r << strlen(str) * 8;
        for (int i = 0; i < strlen(str); i++)
        {
            letter = val_off[i] << 8 * i;
            val_r = val_r | letter;
        }
    }
    //Display OPEN
    if (strlen(str) == 4)
    {
        //Shift default value to allow for incoming letters on the right display
        val_r = val_r << strlen(str) * 8;
        for (int i = 0; i < strlen(str); i++)
        {
            letter = val_open[i] << 8 * i;
            val_r = val_r | letter;
        }
    }
    //Display CLOSED
    if (strlen(str) == 6)
    {
        val_l = 0x4647;
        val_r = 0x40120621;
    }
    if (fun ==1)
    	val_l = 0x6553;

    *Num_R = val_r;
    *Num_L = val_l;
}

//*********************************************************************************

int set_increment_size()
{
    volatile int increment_size_switch = (*Switches >> 0) & 0b1;
    static int increment_size;
    if (increment_size_switch == 1)
    {
        increment_size = 5;
    }

    else
    {
        increment_size = 1;
    }

    return increment_size;
}

//*********************************************************************************

void LED_seq()
{
    int is_high = 0;
    *LEDs = is_high;
    for (int x = 0; x < 2; x++)
    {
        for (int i = 0; i < 10; i++)
        {
            delay(100);
            is_high |= (1 << 1 * i);
            *LEDs = is_high;
        }
        for (int i = 0; i < 10; i++)
        {
            delay(100);
            is_high = is_high << 1;
            *LEDs = is_high;
        }
    }
}

//*********************************************************************************

int encoder_reader(int enc_A, int enc_B, int prev_enc)
{
    int valA = pin_get(enc_A);
    int valB = pin_get(enc_B);

    int increment_size = set_increment_size();

    static int prev_A;

    if (valA != prev_A)
       {
           if (valA != valB)
           {
        	   prev_enc += (1*increment_size);
        	   }
           else
           {
        	   prev_enc -= (1*increment_size);
        	   }
       }
    if (prev_enc > 100)
    {
        prev_enc = 100;
    }

    if (prev_enc < 0)
    {
        prev_enc = 0;
    }
    prev_A = valA;
    return prev_enc;
}

//*********************************************************************************

int mode_selector(int key_states, float duty, int actual_rpm, int desired_rpm, int error, int pid_on)
{
    switch (*push_button)
    {
    case key0:
        key_states = 0;
        break;
    case key1:
        key_states = 1;
        break;
    case key2:
        key_states = 2;
        break;
    case key3:
        key_states = 3;
        break;
    }

    if (key_states == 0)
    {
        *Num_L = 0xA127;
        Display(duty);
    }

    else if (key_states == 1)
    {
        *Num_L = 0x0812;
        Display(actual_rpm);
    }

    else if (key_states == 2)
    {
        if (pid_on == 1)
        {
            *Num_L = 0x07AF;
            Display(desired_rpm);
        }
        else
        {
            *Num_L = 0x0812;
            Display(actual_rpm);
        }
    }
    else if (key_states == 3)
    {
        if (pid_on == 1)
        {
            *Num_L = 0x6AF;
            Display(error);
        }
        else
        {
            *Num_L = 0x0812;
            Display(actual_rpm);
        }
    }
    return key_states;
}

//*********************************************************************************

int main(int argc, char **argv)
{
    EE30186_Start();

    volatile int *GpioDdr = GPIO_Port + 1;
    *GpioDdr = 0x8;

    volatile int *GpioDdr_test = GPIO_Port_2 + 1;
    *GpioDdr_test = 0xC;
    *GPIO_Port_2 = 0;

    //------------------------------------------------------------------------------
    //Variables definition for encoder reader

    const int enc_A = 17;
    const int enc_B = 19;

    int prev_enc = 0;
    int encod_prev_count;


    //------------------------------------------------------------------------------
    //Variables definition for PWM Transmitter

    int freq_counter = 50000000;
    int fsw = freq_counter / 750;
    float duty = 0;

    const int fan_pin = 3;
    int flag_fan = 1;

    int on_duty;

    int pwm_prev_t;
    //------------------------------------------------------------------------------
    //Variables definition for Tachometer

    const int tach_pin = 1;

    double tacho_counters[2] = {0, 0};
    double clk_tacho = 0;

    int state_tacho = 0;

    int pos_prev_count;
    int num_pos_edge = 0;

    int tacho_freq[6];
    int num_tacho_freq = 0;

    int average = 0;

    int start_counter;

    int actual_rpm = 0;

    //------------------------------------------------------------------------------
    //PID Variables

    int error, pid_prev_count, pid_dt, pid_on;
    int init_PID = 0;

    //------------------------------------------------------------------------------
    //Init Variables

    *LEDs = 0;
    int init = 1;
    int key_states, desired_rpm;
    int cycle_up=1;

    //******************************************************************************
    //--------------------------While Loop of the system----------------------------
    //------------------------------------------------------------------------------

    while (1)
    {

    	while(((*Switches >> 9) & 0b1) == 0) {
            Display_char("off", 0);
            pin_set(fan_pin, 0);
            init = 0;
            *LEDs = 0;
    	}

    	if(init==0) {
            Display_char("on", 0);
            delay(200);
            LED_seq();

            encod_prev_count = pwm_prev_t = pid_prev_count = *Counter;
            key_states = prev_enc = actual_rpm = desired_rpm = init_PID = 0;
            start_counter = init = 1;
    	}

//------------------------------------------------------------------------------------------------------------------
//************************************************************************************************************

        state_tacho = pin_get(tach_pin);
        if (state_tacho == 1 && start_counter == 1)
        {
            pos_prev_count = *Counter;
            start_counter = 0;
        }

        else if (state_tacho == 0 && start_counter == 0)
        {
            //set to catch positive edges when there is a gap of 10ms to 30ms
            //first if starts the count/recording else if continues the count/recording
            if (abs(*Counter - pos_prev_count) >= 250000 && abs(*Counter - pos_prev_count) <= 1500000 && num_pos_edge == 0)
            {
                tacho_counters[num_pos_edge] = pos_prev_count;
                num_pos_edge = 1;
            }

            else if (abs(*Counter - pos_prev_count) >= 250000 && abs(*Counter - pos_prev_count) <= 1500000 && num_pos_edge == 1)
            {
                tacho_counters[num_pos_edge] = pos_prev_count;
                clk_tacho = abs(tacho_counters[1] - tacho_counters[0]);
                tacho_freq[num_tacho_freq] = 30000 / ((clk_tacho * 1000) / freq_counter);
                num_tacho_freq++;

                if (num_tacho_freq >= 5)
                {
                    for (int i = 0; i < 5; i++)
                    {
                        average += tacho_freq[i];
                    }
                    actual_rpm = average / num_tacho_freq;
                    num_tacho_freq = average = 0;
                }

                num_pos_edge = 0;
            }

            start_counter = 1;
        }

        //------------------------------------------------------------------------------
        //Encoder Reader

        if (abs(*Counter - encod_prev_count) >= 5000)
        {
            prev_enc = encoder_reader(enc_A, enc_B, prev_enc);
            encod_prev_count = *Counter;
        }

        key_states = mode_selector(key_states, duty, actual_rpm, desired_rpm, error, pid_on);

        desired_rpm = 600 + (prev_enc * 10);

        error = desired_rpm - actual_rpm;
        pid_dt = abs(*Counter - pid_prev_count);
        pid_prev_count = *Counter;

		float kp = 0.000000000000001;
		float ki = 0.000000001;
		float kd = 0.00000001;


			if (((*Switches >> 8) & 0b1) == 1)
			{
				if (init_PID == 1)
				{
					Display_char("closed", 0);
					delay(1000);
					init_PID = 0;
				}
				pid_on = 1;
				duty += ki * error * pid_dt + kd * error / pid_dt + kp * error;
				if (duty <= 1){
					duty = 1;
				}
			}
			else
			{
				if (init_PID == 0)
				{
					Display_char("open", 0);
					delay(1000);
					init_PID = 1;
					if(((*Switches >> 7) & 0b1) == 1){
			            Display_char("o", 1);
					}
					else{
						Display(duty);
					}
					}
				if(((*Switches >> 1) & 0b1) == 1){
					duty = 0;
				}
				else{
					if(((*Switches >> 2) & 0b1) == 1){
						duty = 25;
					}

					else
					{
						if(((*Switches >> 3) & 0b1) == 1)
						{
							duty = 50;
						}
						else
						{
								if(((*Switches >> 4) & 0b1) == 1)
								{
									duty = 75;
								}

								else
								{
									if(((*Switches >> 5) & 0b1) == 1)
									{
										duty = 100;
									}
									else{
										if(((*Switches >> 6) & 0b1) == 1)
										{
											if (duty <= 1){
											cycle_up = 1;
											}

											if (duty >= 100){
											cycle_up = 0;
											}

											if (cycle_up == 1){
											duty += 0.0001;
											}
											if (cycle_up == 0){
											duty -= 0.0001;
											}
										}

										else{
											pid_on = 0;
											duty = prev_enc;
											}
									}
								}
							}
					}

				}
			}

        if (duty >= 100){
            duty = 100;
        }
        else if (duty < 0.7){
            duty = 0.8;
            }


        on_duty = (duty / 100) * fsw;


        if(((abs(*Counter - pwm_prev_t) >= on_duty) && flag_fan==1) || (duty<=0.9))
        {
        pin_set(fan_pin, 0);
        flag_fan = 0;
        }
        else if(((abs(*Counter - pwm_prev_t) >= fsw) && flag_fan==0) || (duty>=99.9))
        {
        pwm_prev_t = *Counter;
        pin_set(fan_pin, 1);
        flag_fan = 1;
        }


        LED_state(duty);
    }
    EE30186_End();
    return 0;
}
