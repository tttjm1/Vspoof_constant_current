//Copyright 2021-2022(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM

/*The MCM measures pack voltage in three different spots:
/ -MCM'E' connector, which is the actual analog pack voltage (e.g. 170 volts).  LiBCM man-in-the-middles this voltage.
/ -VPIN connector, which is a 0:5 volt analog voltage 52 times less than actual pack voltage. LiBCM man-in-the-middles this voltage.
/ -BATTSCI, which is a serial bus the BCM uses to send data to the MCM.  LiBCM replaces the OEM signal with its own implementation.
/
/ The MCM will throw a P-code if all three voltages aren't spoofed to within ~10** volts.
/  **Some comparisons are actually 20 volts, but for simplicity this code treats them all as having to be within 10 volts.
*/

#include "libcm.h"

uint8_t spoofedPackVoltage = 0;

uint8_t modeMCMePWM = MCMe_USING_VPACK;

int16_t pwmCounts_MCMe = 0;
int16_t pwmCounts_VPIN_out = 0;

uint8_t offsetVoltage_MCMe = MCME_VOLTAGE_OFFSET_ADJUST; //constant offset voltage to account for MCM HV insulation test //JTS2doNow: Correlate this to actualPackVoltage

//---------------------------------------------------------------------------------------

void vPackSpoof_setModeMCMePWM(uint8_t newMode) { modeMCMePWM = newMode; }

//---------------------------------------------------------------------------------------

uint8_t vPackSpoof_getMCMeOffsetVoltage(void) { return offsetVoltage_MCMe; }
void    vPackSpoof_setMCMeOffsetVoltage(uint8_t newOffset) { offsetVoltage_MCMe = newOffset; }

//---------------------------------------------------------------------------------------

int16_t vPackSpoof_getPWMcounts_MCMe(void) { return pwmCounts_MCMe; }
void    vPackSpoof_setPWMcounts_MCMe(uint8_t newCounts) { pwmCounts_MCMe = newCounts; }

//---------------------------------------------------------------------------------------

int16_t vPackSpoof_getPWMcounts_VPINout(void) { return pwmCounts_VPIN_out; }

//---------------------------------------------------------------------------------------

//JTS2doNow: Figure out if there's a better way to spoof MCMe to prevent P1576(12)
void spoofVoltageMCMe(void)
{
  //Derivation, empirically determined (see: ~/Electronics/PCB (KiCAD)/RevB/V&V/voltage spoofing results.ods)
  //pwmCounts_MCMe = (               actualPackVoltage   * 512) / spoofedPackVoltage       - 551
  //pwmCounts_MCMe = (               actualPackVoltage   * 256) / spoofedPackVoltage  * 2  - 551 //prevent 16b overflow
  //pwmCounts_MCMe = (( ( ((uint16_t)actualPackVoltage ) * 256) / spoofedPackVoltage) * 2) - 551 

	if(modeMCMePWM == MCMe_USING_VPACK)
	{
		pwmCounts_MCMe = (( ( ((uint16_t)LTC68042result_packVoltage_get()) << 8 ) / spoofedPackVoltage) << 1 ) - 551;
	}
	//else //user entered static PWM value (using '$MCMp' command)

	//bounds checking
	if     (pwmCounts_MCMe > 255) {pwmCounts_MCMe = 255;}
	else if(pwmCounts_MCMe <   0) {pwmCounts_MCMe =   0;}

	analogWrite(PIN_MCME_PWM, (uint8_t)pwmCounts_MCMe);

	//JTSdebug
	static uint8_t pwmCounts_MCMe_previous = 0;
	if(pwmCounts_MCMe != pwmCounts_MCMe_previous)
	{
		Serial.print(F(", pwmCounts_MCMe:"));
		Serial.print(pwmCounts_MCMe);
		pwmCounts_MCMe_previous = pwmCounts_MCMe;
	}
}

//---------------------------------------------------------------------------------------

void spoofVoltageMCMe_setSpecificPWM(uint8_t valuePWM)
{ //used for troubleshooting
	analogWrite(PIN_MCME_PWM, valuePWM);
}

//---------------------------------------------------------------------------------------

void spoofVoltage_VPINout(void)
{
	//      V_DIV_CORRECTION = RESISTANCE_MCM / RESISTANCE_R34
	//      V_DIV_CORRECTION = 100k           / 10k
	#define V_DIV_CORRECTION 1.1

	//remap measured Vpin_in value ratiometrically to desired spoofed voltage
	//It's important to look at VPIN_in, since V_PDU is different from the Vpack during keyON capacitor charging event
	uint16_t intermediateMath = (uint16_t)(adc_packVoltage_VpinIn() * spoofedPackVoltage) * V_DIV_CORRECTION;
	pwmCounts_VPIN_out = (int16_t)( (uint16_t)intermediateMath / LTC68042result_packVoltage_get() );

	//bounds checking
	if     (pwmCounts_VPIN_out > 255) {pwmCounts_VPIN_out = 255;}
	else if(pwmCounts_VPIN_out <  26) {pwmCounts_VPIN_out =  26;} //MCM ADC rolls under if VPIN is less than 0.5 volts (0.5/5*256 = 25.6 ~= 26)

	analogWrite(PIN_VPIN_OUT_PWM, (uint8_t)pwmCounts_VPIN_out);

	//JTSdebug
	static uint8_t pwmCounts_VPIN_out_previous = 0;
	if(pwmCounts_VPIN_out != pwmCounts_VPIN_out_previous)
	{
		Serial.print(F(", pwmCounts_VPIN_out:"));
		Serial.print(pwmCounts_VPIN_out);
		pwmCounts_VPIN_out_previous = pwmCounts_VPIN_out;
	}
}

//---------------------------------------------------------------------------------------

uint8_t calculate_Vspoof_maxPossible(void)
{
	//Hardware limitations prevent us from spoofing the entire voltage range
	//The max allowed voltage is a function of the actual pack voltage
	//Derivation: ~/Electronics/PCB (KiCAD)/RevD/V&V/VPIN-MCMe Calibration.ods

	uint8_t actualPackVoltage = LTC68042result_packVoltage_get();
	uint8_t maxAllowedVspoof = 0;

	if     (actualPackVoltage < 109) { maxAllowedVspoof = actualPackVoltage -  6; }
	else if(actualPackVoltage < 119) { maxAllowedVspoof = actualPackVoltage -  7; }
	else if(actualPackVoltage < 128) { maxAllowedVspoof = actualPackVoltage -  8; }
	else if(actualPackVoltage < 138) { maxAllowedVspoof = actualPackVoltage -  9; }
	else if(actualPackVoltage < 148) { maxAllowedVspoof = actualPackVoltage - 10; }
	else if(actualPackVoltage < 158) { maxAllowedVspoof = actualPackVoltage - 11; }
	else if(actualPackVoltage < 167) { maxAllowedVspoof = actualPackVoltage - 12; }
	else if(actualPackVoltage < 177) { maxAllowedVspoof = actualPackVoltage - 13; }
	else if(actualPackVoltage < 187) { maxAllowedVspoof = actualPackVoltage - 14; }
	else if(actualPackVoltage < 197) { maxAllowedVspoof = actualPackVoltage - 15; }
	else if(actualPackVoltage < 206) { maxAllowedVspoof = actualPackVoltage - 16; }
	else if(actualPackVoltage < 216) { maxAllowedVspoof = actualPackVoltage - 17; }
	else if(actualPackVoltage < 226) { maxAllowedVspoof = actualPackVoltage - 18; }
	else if(actualPackVoltage < 236) { maxAllowedVspoof = actualPackVoltage - 19; }
	else if(actualPackVoltage < 245) { maxAllowedVspoof = actualPackVoltage - 20; }
	else                             { maxAllowedVspoof = actualPackVoltage - 21; }

	return maxAllowedVspoof;
}

//---------------------------------------------------------------------------------------

void spoofVoltage_calculateValue(void)
{
	uint8_t maxPossibleVspoof = calculate_Vspoof_maxPossible();

	//JTSdebug
	static uint8_t maxPossibleVspoof_previous = 0;
	if(maxPossibleVspoof != maxPossibleVspoof_previous)
	{
		Serial.print(F("\nmaxPossibleVspoof: "));
		Serial.print(maxPossibleVspoof);
		maxPossibleVspoof_previous = maxPossibleVspoof;
	}

	#if defined VOLTAGE_SPOOFING_DISABLE
		//For those that don't want voltage spoofing, spoof maximum possible pack voltage
		
		#ifdef STACK_IS_48S
			//48S pack voltage range is close enough to OEM that we can pass just pass the actual maxPossibleVspoof value to the MCM
			spoofedPackVoltage = maxPossibleVspoof;

		#elif defined STACK_IS_60S
			//60S pack voltage is almost always too high to pass directly to MCM
			spoofedPackVoltage = 170; //this value always works with 60S pack
			//Derivations:
			//spoofedPackVoltage must be greater than (actualPackVoltage * 0.67)
			//  If all cells are at Vmax, then Vpack = 4.20 * 60 = 252 volts //252 volts * 0.67 = 169 volts.  Since 169<170, we can spoof this voltage
			//spoofedPackVoltage must be less than (maxPossibleVspoof)
			//  If all cells are at Vmin, then Vpack = 3.18 * 60 = 191 volts //191 volts - 15 = 176 volts.  Since 176>170, we can spoof this voltage
			//
			//JTS2doLater: Figure out a non-constant Vspoof solution (to increase output power when pack voltage is low)
		#endif

	//JTS2doLater: Get these other cases working with 60S
	#elif defined VOLTAGE_SPOOFING_ASSIST_ONLY_BINARY
		if( adc_getLatestBatteryCurrent_amps() > 40 ) { spoofedPackVoltage = 125; } //more than 40 amps assist
		else { spoofedPackVoltage = LTC68042result_packVoltage_get() - offsetVoltage_MCMe; } //less than 40 amps assist or any regen

	//////////////////////////////////////////////////////////////////////////

	#elif defined VOLTAGE_SPOOFING_ASSIST_AND_REGEN
		//Derivation:
		//Maximum assist occurs when MCM thinks pack is at 120 volts.
		//Therefore, we want to adjust the pack voltage over that range:
		//vAdjustRange_mV = (vPackActual_V - offsetVoltage_MCMe - 120) * 1000

		//Since there's ~2x more assist current than regen current, set "0 A" pack voltage to 2/3 the above limits:
		//vPackTwoThirdPoint_mV = vAdjustRange_mV * 2 / 3 + 120,000

		//Next we linearize the (constant) maximum possible assist+regen current:
		//TOTAL_CURRENT_RANGE_A = 140 A + 75 A  //215 A

		//We then calculate the voltage adjustment per amp, across the (variable) spoofed voltage range:
		//voltageAdjustment_mV_per_A = vAdjustRange_mV / TOTAL_CURRENT_RANGE_A

		//Putting these equations together, we determine the correct pack voltage to spoof
		//for any given actual pack voltage at any given current:
		//spoofedVoltage_mV = vPackTwoThirdPoint_mV - actualCurrent_A * voltageAdjustment_mV_per_A

		//Now we need to streamline this equation:
		//spoofedVoltage_mV = vAdjustRange_mV             * 2 / 3 + 120,000   - actualCurrent_A * vAdjustRange_mV / TOTAL_CURRENT_RANGE_A
		//spoofedVoltage_mV = ((vPackActual_mV - 132,000) * 2 / 3 + 120,000 ) - actualCurrent_A * ((vPackActual_mV - 132,000) / 215 )
		//spoofedVoltage_mV = vPackActual_mV            * ( 2 / 3 - actualCurrent_A / 215 ) + 614 * actualCurrent_A + 32,000
		//spoofedVoltage_V = (vPackActual_mV * (667 - actualCurrent_A / 256 * 1000 )/1000 + 614 * actualCurrent_A + 32,000) / 1000
		//spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A / 256 * 1000 )/1 + 614 * actualCurrent_A + 32,000) / 1000

		//approximate:
		//spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A / 256 * 1024 )/1 + 614 * actualCurrent_A + 32,000) / 1024
		//spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A >> 8 << 10) + 614 * actualCurrent_A + 32,000) >> 10
		//spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A << 2 ) + 614 * actualCurrent_A + 32,000) >> 10

		//prevent uint16_t overflow:
		//spoofedVoltage_V = (vPackActual_V * (667 - actualCurrent_A << 2 ) / 4 + ( 614 * actualCurrent_A + 32,000) / 4) >> 8
		//spoofedVoltage_V = ((vPackActual_V * (667 - actualCurrent_A << 2 ) >> 2 ) + 154 * actualCurrent_A + 8,000) >> 8
		//spoofedVoltage_V = ((vPackActual_V * (167 - actualCurrent_A) + 154 * actualCurrent_A + 8,000) >> 8

		//But our rounding lowered the gain more than we wanted, so fudge the number a bit:
		//spoofedVoltage_V = ((vPackActual_V * (167 - actualCurrent_A) + 135 * actualCurrent_A + 8,000) >> 8

		spoofedPackVoltage = (uint8_t)((uint16_t)(LTC68042result_packVoltage_get() * ( 167 - adc_getLatestBatteryCurrent_amps() )
			                    + 135 * adc_getLatestBatteryCurrent_amps() + 8000) >> 8);

	//////////////////////////////////////////////////////////////////////////

	#elif defined	VOLTAGE_SPOOFING_ASSIST_ONLY_VARIABLE
		//Maximum assist occurs when MCM thinks pack is at 120 volts.
		//Therefore, we want to adjust the pack voltage over that range
		//vAdjustRange_mV = (vPackActual_V - offsetVoltage_MCMe - 120) * 1000

		//Since we don't want to spoof voltage during regen, the "0 A" pack voltage is the maximum possible pack voltage.
		//vPackHighestPossible_mV = vAdjustRange_mV + 120,000

		//Next we linearize the (constant) maximum possible assist current:
		//TOTAL_CURRENT_RANGE_A = 140 A
		//However, we want to adjust the pack voltage over a much smaller range, so we choose:
		//TOTAL_CURRENT_RANGE_A = 128 A
		//This will spoof 120 volts at 64 A assist
		//We'll need to bound values to: 120 < vSpoof < (vActual - offsetVoltage_MCMe)

		//We then calculate the voltage adjustment per amp, across the (variable) spoofed voltage range:
		//voltageAdjustment_mV_per_A = vAdjustRange_mV / TOTAL_CURRENT_RANGE_A

		//Putting these equations together, we determine the correct pack voltage to spoof
		//for any given actual pack voltage at any given current:
		//spoofedVoltage_mV = vPackHighestPosible_mV             - actualCurrent_A * voltageAdjustment_mv_per_A

		//Now we need to streamline this equation:
		//spoofedVoltage_mV = vAdjustRange_mV          + 120,000  -  actualCurrent_A * (vAdjustRange_mV               ) / TOTAL_CURRENT_RANGE_A
		//spoofedVoltage_mV = vPackActual_mV - 132,000 + 120,000  -  actualCurrent_A * (vPackActual_mV      - 132,000 ) / 128
		//spoofedVoltage_V  = vPackActual_V  - 132     + 120      -  actualCurrent_A * (vPackActual_V       - 132     ) / 128
		//spoofedVoltage_V  = vPackActual_V  - offsetVoltage_MCMe                 -  actualCurrent_A * (vPackActual_V/128   - 132/128 )

		//approximate:
		//spoofedVoltage_V =  vPackActual_V  - offsetVoltage_MCMe                 -  actualCurrent_A * (vPackActual_V  / 128 -  1   )
		//spoofedVoltage_V =  vPackActual_V  - offsetVoltage_MCMe                 -  actualCurrent_A * (vPackActual_V  >> 7  -  1   )
		//spoofedVoltage_V =  vPackActual_V  - offsetVoltage_MCMe                 -  actualCurrent_A * (vPackActual_V  >> 7) + actualCurrent_A
		//spoofedVoltage_V =  vPackActual_V  - offsetVoltage_MCMe                 -((actualCurrent_A *  vPackActual_V) >> 7) + actualCurrent_A

		//rearrange terms:
		//spoofedVoltage_V = vPackActual_V - offsetVoltage_MCMe + actualCurrent_A -((actualCurrent_A * vPackActual_V) >> 7)

		spoofedPackVoltage = (uint8_t)((int16_t)LTC68042result_packVoltage_get() - offsetVoltage_MCMe + (int16_t)adc_getLatestBatteryCurrent_amps()
	  	                    - ( ( (int16_t)adc_getLatestBatteryCurrent_amps() * (int16_t)LTC68042result_packVoltage_get() ) >> 7 ) );

	//////////////////////////////////////////////////////////////////////////

	#else
		#error (VOLTAGE_SPOOFING value not selected in config.c)
 	#endif

	//////////////////////////////////////////////////////////////////////////

	//bound values
	if( spoofedPackVoltage > maxPossibleVspoof ) { spoofedPackVoltage = maxPossibleVspoof; }
	else if( (spoofedPackVoltage < 120) && (LTC68042result_packVoltage_get() > 130) ) { spoofedPackVoltage = 120; }
}

//---------------------------------------------------------------------------------------

void vPackSpoof_setVoltage(void)
{
	spoofVoltage_calculateValue();

	spoofVoltage_VPINout();
	spoofVoltageMCMe();
	BATTSCI_setPackVoltage(spoofedPackVoltage);
}

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyON(void) { ; }

//---------------------------------------------------------------------------------------

void vPackSpoof_handleKeyOFF(void)
{
	//pinMode(PIN_VPIN_OUT_PWM,INPUT); //set VPIN back to high impedance (to save power) //JTSdebug
	//pinMode(PIN_MCME_PWM,    INPUT); //Set MCM'E' high impedance (to save power) //JTSdebug
}

//---------------------------------------------------------------------------------------

uint8_t vPackSpoof_getSpoofedPackVoltage(void) { return spoofedPackVoltage; }
