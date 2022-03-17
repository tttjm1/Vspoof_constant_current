//Copyright 2021(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

//JTS2doLater: Make this the only line of code in this file:
#include "libcm.h"

void setup() //~t=2 milliseconds, BUT NOTE this doesn't include CPU_CLOCK warmup or bootloader delay 
{
	gpio_begin();
	Serial.begin(115200); //USB
	METSCI_begin();
	BATTSCI_begin();
	LiDisplay_begin();
	lcd_begin();

	LTC68042configure_initialize();

	if( gpio_keyStateNow() == KEYON ){ LED(3,ON); } //turn LED3 on if LiBCM (re)boots while keyON (e.g. while driving)

	gpio_safetyCoverCheck(); //this function hangs forever if safety cover isn't installed

  	//JTS2doLater: Configure watchdog

	#ifdef RUN_BRINGUP_TESTER
	  	bringupTester_run(); //this function never returns
	#endif

	Serial.print(F("\n\nWelcome to LiBCM v" FW_VERSION ", " BUILD_DATE "\n"));
}

void loop()
{
	key_stateChangeHandler();
	temperature_handler();
	SoC_handler();

	if( key_getSampledState() == KEYON )
	{
		if( gpio_isGridChargerPluggedInNow() == PLUGGED_IN ) { lcd_Warning_gridCharger(); } //P1648 occurs if grid charger powered while keyON
		else if( EEPROM_firmwareStatus_get() != FIRMWARE_STATUS_EXPIRED ) { BATTSCI_sendFrames(); } //P1648 occurs if firmware is expired

		LTC68042cell_nextVoltages(); //round-robin handler measures QTY3 cell voltages per call

		METSCI_processLatestFrame();

		adc_updateBatteryCurrent();

		vPackSpoof_setVoltage();

		debugUSB_printLatest_data_keyOn();

		lcd_refresh();
	}
	else if( key_getSampledState() == KEYOFF )
	{	
		if( time_toUpdate_keyOffValues() == true )
		{ 
			//JTS2doNow: Isn't this just LTC68042cell_sampleGatherAndProcessAllCellVoltages (but better)?
			while( LTC68042cell_nextVoltages() != PROCESSED_LTC6804_DATA ) { ; } //measure & read all cell voltages
			
			SoC_updateUsingOpenCircuitVoltage(); //JTS2doNow: Kludged!  Combine with above.

			SoC_setBatteryStateNow_percent( SoC_estimateFromRestingCellVoltage_percent() );
			SoC_turnOffLiBCM_ifPackEmpty();

			cellBalance_handler();

			debugUSB_printLatest_data_gridCharger();
		}

		gridCharger_handler();
	}

	//JTS2doLater: Check for Serial Input from user

	//JTS2doNow: Feed watchdog!

	blinkLED2(); //Heartbeat

	time_waitForLoopPeriod(); //wait here until next iteration
}