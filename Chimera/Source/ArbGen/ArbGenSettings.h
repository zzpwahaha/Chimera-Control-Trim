﻿#pragma once
#include "LowLevel/constants.h"
//#include "ArbGenStructures.h"

struct arbGenSettings {
	bool safemode;
	std::string address;
	unsigned long sampleRate;
	// "INT" or "USB"
	std::string memoryLocation;
	std::string deviceName;

	unsigned /*long*/ triggerRow;
	unsigned /*long*/ triggerNumber;
	std::string configurationFileDelimiter;

	std::vector<double> calibrationCoeff;
	std::vector<std::string> setupCommands;
};


// NOT IN USE AT THE MOMENT (Nov 27th, 2020)
const arbGenSettings UWAVE_AGILENT_SETTINGS = {
	// safemode option											
	UWAVE_SAFEMODE_AGI,
	// usb/tcpip address
	/*UWAVE_SIGLENT_ADDRESS*/UWAVE_AGILENT_ADDRESS,
	// sample rate in hertz
	1e6,
	// Memory location, whether the device will save waveforms to 
	// the internal 64MB Memory buffer or to an external USB drive, which
	// can (obviously) have much more space.
	"INT",
	// device name (just a convenience, so that the class instance knows 
	// which device it is
	"Cryo Agilent",
	UWAVE_AGILENT_TRIGGER_LINE.first, UWAVE_AGILENT_TRIGGER_LINE.second,
	// Configuration file delimiter, used for saving settings for this 
	// agilent.
	"AGILENT_AWG",
	// Calibration coefficients (arb length)
	{ },
	{ "output1 off", "output2 off",
	"Trigger1:Source external", "Trigger1:Slope Positive",
	"Source1:burst:state off",  "Source2:burst:state off",
	"output1:load INF", "output2:load INF", 
	"SOURCE1:FUNC:ARB:FILTER Normal", "SOURCE2:FUNC:ARB:FILTER Normal" }
};

const arbGenSettings UWAVE_SIGLENT_SETTINGS = {
	// safemode option											
	UWAVE_SAFEMODE_SIG,
	// usb/tcpip address
	UWAVE_SIGLENT_ADDRESS/*UWAVE_AGILENT_ADDRESS*/,
	// sample rate in hertz
	20e6,
	// Memory location, whether the device will save waveforms to 
	// the internal 64MB Memory buffer or to an external USB drive, which
	// can (obviously) have much more space.
	"INT",
	// device name (just a convenience, so that the class instance knows 
	// which device it is
	"Cryo Siglent",
	UWAVE_SIGLENT_TRIGGER_LINE.first, UWAVE_SIGLENT_TRIGGER_LINE.second,
	// Configuration file delimiter, used for saving settings for this 
	// agilent.
	"SIGLENT_AWG",
	// Calibration coefficients (arb length)
	{ },
	/**********make sure be in DDS mode to change burst and sweep***********/
	{ "C1:OUTPUT OFF", "C2:OUTPUT OFF","C1:SRATE MODE,DDS","C2:SRATE MODE,DDS",
	"MODE PHASE-LOCKED",/*Both DDS reset when changing frequency. Phase deviation between Ch1&2 is maintained. This command somehow does not work*/
	"C1:BTWV STATE,ON", "C1:BTWV TRSR,EXT,GATE_NCYC,NCYC,EDGE,RISE,TIME,1,STPS,0","C1:BTWV STATE,OFF",
	"C2:BTWV STATE,ON", "C2:BTWV TRSR,EXT,GATE_NCYC,NCYC,EDGE,RISE,TIME,1,STPS,0","C2:BTWV STATE,OFF",
	/*"C1:SRATE MODE,TARB","C2:SRATE MODE,TARB",*/"C1:OUTPUT LOAD,HZ,PLRT,NOR", "C2:OUTPUT LOAD,HZ,PLRT,NOR" } // In TrueArb mode, can not use burst
};

//const agilentSettings TOP_BOTTOM_AGILENT_SETTINGS = { TOP_BOTTOM_AGILENT_SAFEMODE, TOP_BOTTOM_AGILENT_USB_ADDRESS,
// 10e6, "INT", "Top_Bottom",
//DoRows::which::B, 4,
//"TOP_BOTTOM_AGILENT_AWG", { 0.00705436300498, 2.64993907362, -46.4639258399, 550.927892792, -3807.13743019, 
//15548.5874213, -36450.3147559, 41176.7569795, 384.815598655, -41759.1457154, 8882.70382591, 26042.5221431, 
//15930.7138745, -23506.4467208, -12245.7721134, -11521.3566847, 14301.7653239, 7231.41825458, 6022.29831157, 
//3131.29460931, 8311.02350961, -13655.0675627, -11139.8388811, -4957.03018498, -3668.64359645, 9417.84306993, 
//12035.3963169, 1654.16454032, -16230.8390772, 8171.7015004, 5007.09037684, 5463.49980024, -6960.85369185, 
//2243.57572357, -5168.88995534, -1624.67211651, 1.10973170748, -5916.23101957, 1187.70683116, -2206.88472898, 
//1126.16012039, 8185.05136329, 6312.13302578, 1239.26523376, -7565.47746904, 5116.22647775, -5425.55460459, 
//-6455.76453072, 2942.34478932, 2692.33111404 },
//{ "Trigger:Source external", "Trigger:Slope Positive", "output1:load INF", "output2:load INF",
//   "SOURCE1:FUNC:ARB:FILTER Normal", "SOURCE2:FUNC:ARB:FILTER Normal" } };
//
//const agilentSettings AXIAL_AGILENT_SETTINGS = { AXIAL_AGILENT_SAFEMODE, AXIAL_AGILENT_USB_ADDRESS,
//10e6, "INT", "Axial",
//DoRows::which::D, 3,
//"AXIAL_AGILENT_AWG",{ },
//{ "Trigger1:Source external", "Trigger1:Slope Positive", "Trigger2:Source external", "Trigger2:Slope Positive", "output1:load INF", "output2:load INF",
//  "SOURCE1:FUNC:ARB:FILTER Normal", "SOURCE2:FUNC:ARB:FILTER Normal" }
//};

//const agilentSettings FLASHING_AGILENT_SETTINGS = { FLASHING_SAFEMODE, FLASHING_AGILENT_USB_ADDRESS,
//1e6, "INT", "Flashing",
//DoRows::which::A, 0,
//"FLASHING_AGILENT_AWG", { },
//{ "output1 off", "output2 off",
//"Source1:burst:state off",
//"Source1:Apply:Square 2 MHz, 3 VPP, 1.5 V",
//"Source1:Function:Square:DCycle 42",
//"Source2:burst:state off",
//"Source2:Apply:Square 2 MHz, 3 VPP, 1.5 V",
//"Source2:Function:Square:DCycle 50",
//"Source1:burst:state off", "Source2:burst:state off",
//"output1 on", "output2 on", "output1:load INF", "output2:load INF",
//"SOURCE1:FUNC:ARB:FILTER Normal", "SOURCE2:FUNC:ARB:FILTER Normal",
//"SOURCE1:PHASE 0", "SOURCE2:PHASE 165",
//"SOURCE1:PHASE:SYNCHRONIZE", "SOURCE2:PHASE:SYNCHRONIZE" }
//}; 

//const agilentSettings INTENSITY_AGILENT_SETTINGS = { INTENSITY_SAFEMODE, INTENSITY_AGILENT_USB_ADDRESS,
//													5e5, "USB", "Intensity",
//													DoRows::which::B, 6,
//													"INTENSITY_AGILENT_AWG", { 0.00037532, -0.2343111 }, 
//	// Old Calibrations: //{-0.00071625, -0.17698902}, //{0.01124213, -0.15181633}, //{1.71223989e-3, -0.18720763}, 
//	// { 0.00102751, -0.02149967 },
//													{ "Trigger:Source external", "Trigger:Slope Positive", 
//													  "output1:load INF", "output2:load INF",
//													  "SOURCE1:FUNC:ARB:FILTER Normal", "SOURCE2:FUNC:ARB:FILTER Normal" }
//};

