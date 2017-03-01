/********************************************************
 Name          : main.c
 Author        : Nomen Nescio
 Copyright     : Not really
 Description   : EVK1100 template
 **********************************************************/

// Include Files


#include "board.h"
#include "compiler.h"
#include "dip204.h"
#include "gpio.h"
#include "pm.h"
#include "pwm.h"
#include "delay.h"
#include "spi.h"
#include <avr32/io.h>
#include "adc.h"
#include "intc.h"
#include "tc.h"
#include "power_clocks_lib.h"
#include "print_funcs.h"
#  define EXAMPLE_PWM_PIN             AVR32_PWM_3_PIN
#  define EXAMPLE_PWM_FUNCTION        AVR32_PWM_3_FUNCTION
#  define EXAMPLE_PWM_CHANNEL_ID      3
#  define EXAMPLE_ADC_POTENTIOMETER_CHANNEL   1
#  define EXAMPLE_ADC_POTENTIOMETER_PIN       AVR32_ADC_AD_1_PIN
#  define EXAMPLE_ADC_POTENTIOMETER_FUNCTION  AVR32_ADC_AD_1_FUNCTION
#  define EXAMPLE_TC                  (&AVR32_TC)
#  define EXAMPLE_TC_IRQ_GROUP        AVR32_TC_IRQ_GROUP
#  define EXAMPLE_TC_IRQ              AVR32_TC_IRQ0
#  define TC_CHANNEL 0
#  define CPU_HZ 60000000
#  define PBA_HZ 16500000

char choice=0;
float frequency_value=1.0;
float dutycycle_value=1.0;
unsigned short value=0;

volatile avr32_adc_t *adc = &AVR32_ADC; // ADC IP registers address

       // Assign the on-board sensors to their ADC channel.
unsigned short adc_channel_pot = EXAMPLE_ADC_POTENTIOMETER_CHANNEL;

void init_LCD(void)
{
	static const gpio_map_t DIP204_SPI_GPIO_MAP =
					  {
					    {DIP204_SPI_SCK_PIN,  DIP204_SPI_SCK_FUNCTION },  // SPI Clock.
					    {DIP204_SPI_MISO_PIN, DIP204_SPI_MISO_FUNCTION},  // MISO.
					    {DIP204_SPI_MOSI_PIN, DIP204_SPI_MOSI_FUNCTION},  // MOSI.
					    {DIP204_SPI_NPCS_PIN, DIP204_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
					  };

					  // add the spi options driver structure for the LCD DIP204
					  spi_options_t spiOptions =
					  {
					    .reg          = DIP204_SPI_NPCS,
					    .baudrate     = 1000000,
					    .bits         = 8,
					    .spck_delay   = 0,
					    .trans_delay  = 0,
					    .stay_act     = 1,
					    .spi_mode     = 0,
					    .modfdis      = 1
					  };
					  // Assign I/Os to SPI
					  gpio_enable_module(DIP204_SPI_GPIO_MAP, sizeof(DIP204_SPI_GPIO_MAP) / sizeof(DIP204_SPI_GPIO_MAP[0]));
					  // Initialize as master
					  spi_initMaster(DIP204_SPI, &spiOptions);
					  // Set selection mode: variable_ps, pcs_decode, delay
					  spi_selectionMode(DIP204_SPI, 0, 0, 0);
					  // Enable SPI
					  spi_enable(DIP204_SPI);
					  // setup chip registers
					  spi_setupChipReg(DIP204_SPI, &spiOptions, FOSC0);
					  // initialize delay driver
					  delay_init( FOSC0 );
					  // initialize LCD
					  dip204_init(backlight_PWM, TRUE);
}
//*********************************************************
static void int_handler (void)
{
      choice=choice^0x1;//toggle the value
      gpio_clear_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
      gpio_get_pin_interrupt_flag(GPIO_PUSH_BUTTON_0);
}
//*********************************************************
static void tc_irq(void)
{
  // get value for the potentiometer adc channel
	value=adc_get_value(adc, adc_channel_pot);
	//gpio_clr_gpio_pin(LED0_GPIO);
	print_dbg("Tc interrupt1");
  // Clear the interrupt flag. This is a side effect of reading the TC SR.
	//tc_read_sr(EXAMPLE_TC, TC_CHANNEL);
	volatile int i;
	for(i=0;i<1000;i++);
	tc_read_sr(EXAMPLE_TC, TC_CHANNEL);
	print_dbg("Tc interrupt2");
	//gpio_set_gpio_pin(LED0_GPIO);

}
//*********************************************************
int main(void)
{
	//use power manger driver for setting the frequency
	    pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
		pm_freq_param_t System_Clock = {
			    .cpu_f = CPU_HZ,
			    .pba_f = PBA_HZ,
			    .osc0_f = FOSC0,
			    .osc0_startup = OSC0_STARTUP
			};
		pm_configure_clocks(&System_Clock);
//*********************************************************
	init_LCD();
	init_dbg_rs232(PBA_HZ);

	 print_dbg("\n\n\n");
	gpio_local_init();
	gpio_enable_pin_glitch_filter(GPIO_PUSH_BUTTON_0);
	gpio_enable_gpio_pin(GPIO_PUSH_BUTTON_0);
//*********************************************************
	 volatile avr32_tc_t *tc = EXAMPLE_TC;
	  // Options for waveform genration.

	  static const tc_waveform_opt_t WAVEFORM_OPT =
	  {
	    .channel  = TC_CHANNEL,                        // Channel selection.

	    .bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	    .beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	    .bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	    .bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	    .aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	    .aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
	    .acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
	    .acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

	    .wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	    .enetrg   = FALSE,                             // External event trigger enable.
	    .eevt     = 0,                                 // External event selection.
	    .eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	    .cpcdis   = FALSE,                             // Counter disable when RC compare.
	    .cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	    .burst    = FALSE,                             // Burst signal selection.
	    .clki     = FALSE,                             // Clock inversion.
	    .tcclks   = TC_CLOCK_SOURCE_TC3                // Internal source clock 3, connected to fPBA / 8.
	  };

	  static const tc_interrupt_t TC_INTERRUPT =
	  {
	    .etrgs = 0,
	    .ldrbs = 0,
	    .ldras = 0,
	    .cpcs  = 1,
	    .cpbs  = 0,
	    .cpas  = 0,
	    .lovrs = 0,
	    .covfs = 0
	  };
//*********************************************************
	  // GPIO pin/adc-function map.
	static const gpio_map_t ADC_GPIO_MAP =
	 {
      {EXAMPLE_ADC_POTENTIOMETER_PIN, EXAMPLE_ADC_POTENTIOMETER_FUNCTION}
	 };

      // Assign and enable GPIO pins to the ADC function.
     gpio_enable_module(ADC_GPIO_MAP, sizeof(ADC_GPIO_MAP) / sizeof(ADC_GPIO_MAP[0]));

     // configure ADC
       // Lower the ADC clock to match the ADC characteristics (because we configured
       // the CPU clock to 12MHz, and the ADC clock characteristics are usually lower;
       // cf. the ADC Characteristic section in the datasheet).
       AVR32_ADC.mr |= 0x1 << AVR32_ADC_MR_PRESCAL_OFFSET;
       adc_configure(adc);

       // Enable the ADC channels.
       adc_enable(adc,adc_channel_pot);
//*********************************************************
       pwm_opt_t pwm_opt;                                // PWM option config.
         avr32_pwm_channel_t pwm_channel = { .ccnt = 0 };  // One channel config.
         // The channel number and instance is used in several functions.
         // It's defined as local variable for ease-of-use.
         unsigned int channel_id;

         channel_id = EXAMPLE_PWM_CHANNEL_ID;
         gpio_enable_module_pin(EXAMPLE_PWM_PIN, EXAMPLE_PWM_FUNCTION);

         // PWM controller configuration.
         pwm_opt.diva = AVR32_PWM_DIVA_CLK_OFF;
         pwm_opt.divb = AVR32_PWM_DIVB_CLK_OFF;
         pwm_opt.prea = AVR32_PWM_PREA_MCK;
         pwm_opt.preb = AVR32_PWM_PREB_MCK;

         pwm_init(&pwm_opt);

         pwm_channel.CMR.calg = PWM_MODE_LEFT_ALIGNED;       // Channel mode.
         pwm_channel.CMR.cpol = PWM_POLARITY_LOW;            // Channel polarity.
         pwm_channel.CMR.cpd = PWM_UPDATE_DUTY;              // Not used the first time.
         pwm_channel.CMR.cpre = AVR32_PWM_CPRE_MCK;  // Channel prescaler.
         pwm_channel.cdty = 5;   // Channel duty cycle, should be < CPRD.
         pwm_channel.cprd = 20;  // Channel period.
         pwm_channel.cupd = 0;   // Channel update is not used here.
         // With these settings, the output waveform period will be :
         // (115200/256)/20 == 22.5Hz == (MCK/prescaler)/period, with MCK == 115200Hz,
         // prescaler == 256, period == 20.
//**************************************************************
       gpio_enable_pin_interrupt(GPIO_PUSH_BUTTON_0 , GPIO_RISING_EDGE);
       INTC_init_interrupts ();

       INTC_register_interrupt(&int_handler,  (AVR32_GPIO_IRQ_0+88/8), AVR32_INTC_INT1);

//*********************************************************
        // pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
       // Initialize the timer/counter.
         tc_init_waveform(tc, &WAVEFORM_OPT);         // Initialize the timer/counter waveform.

         //*******************************
         // Set the compare triggers.
         // Remember TC counter is 16-bits, so counting second is not possible with fPBA = 12 MHz.
         // We configure it to count ms.
         // We want: (1/(fPBA/8)) * RC = 0.02 s, hence RC = (fPBA/8)*0.02= 41250to get an interrupt every 20 ms.
         tc_write_rc(tc, TC_CHANNEL, (PBA_HZ / 8)*0.02); // Set RC value.
         // tc_write_rc(tc, TC_CHANNEL, 60000);
          tc_configure_interrupts(tc, TC_CHANNEL, &TC_INTERRUPT);
          INTC_register_interrupt(&tc_irq, EXAMPLE_TC_IRQ, AVR32_INTC_INT0);
         // Start the timer/counter.
                             // And start the timer/counter.
//*********************************************************
        Enable_global_interrupt ();
        // launch conversion on all enabled channels

//*********************************************************
       // do an infinite loop
       volatile int true_var = TRUE;
       //float fre=0.0;
       tc_start(tc, TC_CHANNEL);
        while (true_var)    // use a volatile true variable to avoid warning on unreachable code
        {
          adc_start(adc);
          dip204_set_cursor_position(17,2);
          if(choice==0)
          {
        	  //frequency_value =1023-value;
        	  if(value==1023)
        	  {
        	  frequency_value=100000;
        	  }
        	  else

        	  {
        	  frequency_value=((100000-100)/(1023))*value+100.0;
        	  }
        	  dip204_printf_string("F");
          }
          else if(choice==1)
          {
        	  dutycycle_value =value;
        	  dip204_printf_string("D");
          }
          // display value to user
          dip204_set_cursor_position(3,1);
          dip204_printf_string("Lab3");

          float per=1-((dutycycle_value+1)/1024)*1.0;
          dip204_set_cursor_position(1,3);
         // dip204_printf_string("Frequency:%3ukHZ",(115200)/frequency_value);
          dip204_printf_string("Frequency:%5.0fHZ",frequency_value);
          dip204_set_cursor_position(1,4);
          dip204_printf_string("DutyCycle:%3.1f%%  ",(1-per)*100);
          dip204_hide_cursor();

          // Channel duty cycle, should be < CPRD.
           pwm_channel.cprd = 15000000/frequency_value;
         // pwm_channel.cprd = 2000;
          pwm_channel.cdty = (int)(per*pwm_channel.cprd);
          //pwm_channel.cdty = 5;
          dip204_set_cursor_position(1,2);
          dip204_printf_string("Position:%4u",value);
          dip204_set_cursor_position(9,1);
          dip204_printf_string("cprd:%6d",pwm_channel.cprd);

          pwm_channel_init(channel_id, &pwm_channel); // Set channel configuration to channel 0
          pwm_start_channels(1 << channel_id);
        }
        adc_disable(adc,adc_channel_pot);



	return 0;
}
