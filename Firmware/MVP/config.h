//Copyright 2021-2023(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

//config.h - compile time configuration parameters

#ifndef config_h
	#define config_h
	#include "libcm.h"

	#define FW_VERSION "0.9.1e"
	#define BUILD_DATE "2023AUG27"

	//////////////////////////////////////////////////////////////////

	//////////////////////////////
	//                          //
	//  Hardware Configuration  //
	//                          //
	//////////////////////////////

	//choose your battery type:
		#define BATTERY_TYPE_5AhG3 //previously (incorrectly) referred to as "EHW5"
		//#define BATTERY_TYPE_47AhFoMoCo

	//JTS2doNow: Come up with method to determine if user has selected correct option before allowing contactor to fire.
	//choose how many cells in series
		#define STACK_IS_48S
		//#define STACK_IS_60S

	//choose ONE of the following:
		//#define SET_CURRENT_HACK_00 //OEM configuration (no current hack installed inside MCM)
		//#define SET_CURRENT_HACK_20 //actually +25.0%
		#define SET_CURRENT_HACK_40 //actually +45.8%

	//choose ONE of the following:
		#define VOLTAGE_SPOOFING_DISABLE              //spoof maximum possible pack voltage at all times //closest to OEM behavior
		//#define VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE //increase assist power by variably   spoofing pack voltage during assist
		//#define VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY   //increase assist power by statically spoofing pack voltage during heavy assist
		//#define VOLTAGE_SPOOFING_ASSIST_AND_REGEN     //increase assist and regen power by variably spoofing pack voltage //DEPRECATED (regen too strong)

	//choose which display(s) is/are connected:
		#define LCD_4X20_CONNECTED  //Comment to disable all 4x20 LCD commands
		//#define LIDISPLAY_CONNECTED  //Comment to disable all LiDisplay commands //JTS2doLater: mudder has not yet tested this code. Use at your own risk.

	//choose which grid charger is installed
		#define GRIDCHARGER_IS_NOT_1500W
		//#define GRIDCHARGER_IS_1500W //Uncomment if using the optional "+15% SoC per hour" charger (UHP-1500-230) //sold only with FoMoCo Kits
		
	//JTS2doNow: Implement this feature	
	//if using 1500 watt charger with 120 volt extension cord, choose input current limit 
		//#define CHARGER_INPUT_CURRENT__15A_MAX //select this option if using 12 AWG extension cord up to 100 feet, or 14 AWG up to 50 feet**, else if;
		//#define CHARGER_INPUT_CURRENT__13A_MAX //select this option if using 14 AWG extension cord up to 100 feet, or 16 AWG up to 50 feet**, else if;
		//#define CHARGER_INPUT_CURRENT__10A_MAX //select this option if using 16 AWG extension cord up to 100 feet, or 18 AWG up to 50 feet**.
			//**please verify maximum continuous current rating for your specific extension cord

	//////////////////////////////////////////////////////////////////

	/////////////////////////
	//                     //
	//  Firmware Settings  //
	//                     //
	/////////////////////////

	#define STACK_SoC_MAX 85 //maximum state of charge before regen  is disabled
	#define STACK_SoC_MIN 10 //minimum state of charge before assist is disabled

	#define CELL_VMAX_REGEN                     43500 //43500 = 4.3500 volts
	#define CELL_VMIN_ASSIST                    31900
	#define CELL_VMAX_GRIDCHARGER               39600 //3.9 volts is 75% SoC //other values: See SoC.cpp //MUST be less than 'CELL_VREST_85_PERCENT_SoC'
	#define CELL_VMIN_GRIDCHARGER               30000 //grid charger will not charge severely empty cells
	#define CELL_VMIN_KEYOFF                    CELL_VREST_10_PERCENT_SoC //when car is off, LiBCM turns off below this voltage
	#define CELL_BALANCE_MIN_SoC                65    //when car is off, cell balancing is disabled when battery is less than this percent charged
	#define CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE 32    //'32' = 3.2 mV //CANNOT exceed 255 counts (25.5 mV)
	#define CELL_BALANCE_TO_WITHIN_COUNTS_TIGHT 22    //'22' = 2.2 mV //LTC6804 total measurement error is 2.2 mV //MUST be less than CELL_BALANCE_TO_WITHIN_COUNTS_LOOSE
	#define CELL_BALANCE_MAX_TEMP_C             40
	//#define ONLY_BALANCE_CELLS_WHEN_GRID_CHARGER_PLUGGED_IN //uncomment to disable keyOFF cell balancing (unless the grid charger is plugged in)

	//temp setpoints
	#define COOL_BATTERY_ABOVE_TEMP_C_KEYOFF       36 //cabin air cooling
	#define COOL_BATTERY_ABOVE_TEMP_C_GRIDCHARGING 30
	#define COOL_BATTERY_ABOVE_TEMP_C_KEYON        30
	#define HEAT_BATTERY_BELOW_TEMP_C_KEYON        16 //cabin air heating, or heater PCB (if installed)
	#define HEAT_BATTERY_BELOW_TEMP_C_GRIDCHARGING 16
	#define HEAT_BATTERY_BELOW_TEMP_C_KEYOFF       10
	//other temp settings
	#define KEYOFF_DISABLE_THERMAL_MANAGEMENT_BELOW_SoC 50 //when keyOFF (unless grid charger plugged in) //set to 100 to disable when keyOFF

	#define LTC68042_ENABLE_C19_VOLTAGE_CORRECTION //uncomment if using stock Honda 5AhG3 lithium modules

	#define KEYOFF_DELAY_LIBCM_TURNOFF_MINUTES 10 //Even with low SoC, LiBCM will remain on for this many minutes after keyOFF.
		//to turn LiBCM back on: turn ignition 'ON', or turn IMA switch off and on, or plug in USB cable

	//////////////////////////////////////////////////////////////////

	////////////////////////
	//                    //
	//  Debug Parameters  //
	//                    //
	////////////////////////

	//#define RUN_BRINGUP_TESTER_MOTHERBOARD //requires external test PCB (that you don't have)
	//#define RUN_BRINGUP_TESTER_GRIDCHARGER //requires external test equipment

	#define CHECK_FOR_SAFETY_COVER //comment if testing LiBCM without the cover

	#define DEBUG_USB_UPDATE_PERIOD_GRIDCHARGE_mS 1000 //JTS2doLater: Model after "debugUSB_printLatestData"

	//#define DISABLE_ASSIST //uncomment to (always) disable assist
	//#define DISABLE_REGEN  //uncomment to (always) disable regen
	//#define REDUCE_BACKGROUND_REGEN_UNLESS_BRAKING //EXPERIMENTAL! //JTS2doLater: Make this work (for Balto)

	//choose which functions control the LEDs
		#define LED_NORMAL //enable "LED()" functions (see debug.c)
		//#define LED_DEBUG //enable "debugLED()" functions (FYI: blinkLED functions won't work)
#endif

/*
JTS2doLater:
	#define SERIAL_H_LINE_CONNECTED NO //H-Line wire connected to OEM BCM connector pin B01
	#define KEYOFF_TURNOFF_LIBCM_AFTER_HOURS 48 //LiBCM turns off this many hours after keyOFF.

	Change these #define statements so that all they do is reconfigure EEPROM values.
	Also, change this file so that all #define statements are commented out by default.
	With these two changes, a user only needs to edit config.h if they want to change a previously sent parameter.
	If user doesn't uncomment anything, then the previously uploaded value remains in EEPROM
*/
