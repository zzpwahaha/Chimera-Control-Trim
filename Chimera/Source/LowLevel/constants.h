// created by Mark O. Brown
#pragma once

#include "GeneralUtilityFunctions/my_str.h"
#include <string>
#include <vector>
#include <array>
#include <utility>

// running in safemode means that the program doesn't actually try to connect to physical devices. Generally, it will 
// follow the exact coding logic, but at the exact point where it would normally communicate with a device, it will 
// instead simply skip this step. It might generate example data where useful / necessary (e.g. after querying a
// camera system for a picture). It can be used to build and debug other aspects of the program, such as the gui, 
// coding logic, etc.
 
#define MASTER_COMPUTER

#ifdef MASTER_COMPUTER
	//constexpr bool DOFTDI_SAFEMODE = true;
	constexpr bool DDS_SAFEMODE = true;
	constexpr bool ANDOR_SAFEMODE = false;
	const std::pair<unsigned, unsigned> ANDOR_TRIGGER_LINE = std::make_pair(1 - 1, 5); // used for QtAndorWindow::abortCameraRun to give the last trigger and also for consistensy check
	//constexpr bool ANALOG_IN_SAFEMODE = true;
	#ifdef _DEBUG
		constexpr bool PYTHON_SAFEMODE = true;
	#else
		constexpr bool  PYTHON_SAFEMODE = true;
	#endif
	//constexpr bool DAQMX_SAFEMODE = true;
	//constexpr bool ANALOG_OUT_SAFEMODE = true;

	constexpr auto CODE_ROOT = "C:\\Chimera\\Chimera-Cryo";

	const std::string PLOT_FILES_SAVE_LOCATION = str (CODE_ROOT) + "\\Plotting";
	const std::string DATA_ANALYSIS_CODE_LOCATION = "C:\\Users\\Regal-Lab\\Code\\Data_Analysis_Control\\";
	const std::string DEFAULT_SCRIPT_FOLDER_PATH = str (CODE_ROOT) + "\\Default-Scripts\\";
	const std::string ACTUAL_CODE_FOLDER_PATH = str (CODE_ROOT) + "\\Chimera\\";
	const std::string PROFILES_PATH = str (CODE_ROOT) + "\\Profiles\\";
	const std::string CONFIGURATION_PATH = str(CODE_ROOT) + "\\Configurations\\";

	//const std::string DATA_SAVE_LOCATION = "J:\\Data Repository\\New Data Repository\\";
	const std::string DATA_SAVE_LOCATION = "C:\\Chimera\\Chimera-Cryo\\tmpDataSave\\";

	const std::string MUSIC_LOCATION = str (CODE_ROOT) + "\\Final Fantasy VII - Victory Fanfare [HQ].mp3";
	const std::string FUNCTIONS_FOLDER_LOCATION = str (CODE_ROOT) + "\\Functions\\";
	const std::string MASTER_CONFIGURATION_FILE_ADDRESS (str (CODE_ROOT) + "\\Master-Configuration.txt");
	const std::string CAMERA_CAL_ROUTINE_ADDRESS = PROFILES_PATH + "Hotkey Experiments\\Camera";
	const std::string MOT_ROUTINES_ADDRESS = PROFILES_PATH + "Hotkey Experiments\\MOT";
	// location where wave data can be outputted for analyzing with another computer.


	// Zynq realated
	const double ZYNQ_DEADTIME = 0.1; // give zynq 0.1ms to avoid sending zero at t=0 for ttl
	
	const bool ZYNQ_SAFEMODE = false;
	const auto ZYNQ_ADDRESS = "10.10.0.2";
	const auto ZYNQ_PORT = "8080";
	const int ZYNQ_MAX_BUFF = 64;
	const int DIO_LEN_BYTE_BUF = 28;
	const int DAC_LEN_BYTE_BUF = 44;
	const int DDS_LEN_BYTE_BUF = 46;

	const double DIO_TIME_RESOLUTION = 1e-5; // in ms, 10ns
	const double DAC_TIME_RESOLUTION = 0.02; // in ms, 20us for 50kHz update rate
	//const int DAC_RAMP_MAX_PTS = 0xffff; // 65535 ???
	const double DDS_TIME_RESOLUTION = 1.6; // in ms
	const double DDS_MAX_AMP = 1.25; // in mW
	const std::pair<unsigned, unsigned> DIO_REWIND = std::make_pair(8 - 1, 7); // used for the long time run rewind, see DoCore::checkLongTimeRun. THIS IS NOT USED FOR NOW
	const std::array<unsigned short, 2> DAC_REWIND = { 15, 31 }; // used for the long time run rewind, see AoCore::formatDacForFPGA

	//OffsetLock 
	const std::vector<bool> OFFSETLOCK_SAFEMODE = std::vector<bool>{ false,false,true };
	const std::vector<std::string> OL_COM_PORT = { "COM3", "COM7", "COM12"};
	const double OL_TIME_RESOLUTION = 0.02; //in ms
	const std::vector<std::pair<unsigned, unsigned>> OL_TRIGGER_LINE
		= { std::make_pair(3 - 1,6),std::make_pair(3 - 1,7),
			std::make_pair(4 - 1,6),std::make_pair(4 - 1,7),
			std::make_pair(5 - 1,6) }; /*the first is the label on the box minus 1, has minus'd 1 explicitly */
	const double OL_TRIGGER_TIME = 0.01; //in ms i.e. 50us

	//#define DDS_FPGA_ADDRESS "FT1I6IBSB"; //Device Serial: FT1I6IBS, Use FT1I6IBSB in C++ to select Channel B

	//ArbGens
	const bool UWAVE_SAFEMODE = true;
	const bool UWAVE_SAFEMODE_SIG = false;
	const bool UWAVE_SAFEMODE_AGI = false;
	const int numArbGen = 2;
	//const std::string UWAVE_AGILENT_ADDRESS = "TCPIP0::10.10.0.5::inst0::INSTR";
	const std::string UWAVE_AGILENT_ADDRESS = "USB0::0x0957::0x2807::MY57400998::INSTR";
	const std::pair<unsigned, unsigned> UWAVE_AGILENT_TRIGGER_LINE = std::make_pair(7 - 1, 0); /*the first is the label on the box minus 1, has minus'd 1 explicitly */
	const std::string UWAVE_SIGLENT_ADDRESS = "USB0::0xF4EC::0x1102::SDG2XCAC6R0238::INSTR";
	const std::pair<unsigned, unsigned> UWAVE_SIGLENT_TRIGGER_LINE = std::make_pair(7 - 1, 1); /*the first is the label on the box minus 1, has minus'd 1 explicitly */
	const std::string RAMP_LOCATION = str(CODE_ROOT) + "\\Ramp_Files\\";

	//Analog in 
	const bool AI_SAFEMODE = false;
	const std::string AI_SOCKET_ADDRESS = "10.10.0.10";
	const std::string AI_SOCKET_PORT = "80";

	//Mako camera
	const unsigned MAKO_NUMBER = 4;
	const std::array<bool, MAKO_NUMBER> MAKO_SAFEMODE = { false,false,false,false};
	const std::array<std::string, MAKO_NUMBER> MAKO_DELIMS = { "MAKO1_CAM"/*MOT G125*/, "MAKO2_CAM"/*MOT G319*/, "MAKO3_CAM"/*420 MON*/, "MAKO4_CAM"/*1013 MON*/};
	const std::array<std::string, MAKO_NUMBER> MAKO_IPADDRS = { "10.10.0.6", "10.10.0.7","10.10.0.12","10.10.0.11" };
	const std::vector<std::pair<unsigned, unsigned>> MAKO_TRIGGER_LINE
		= { std::make_pair(3 - 1,4),std::make_pair(3 - 1,5),std::make_pair(5 - 1,3),std::make_pair(5 - 1,7) };
	/*the first is the label on the box minus 1, has minus'd 1 explicitly and this is not used in the code, just a reminder*/

	//GIGAMOOG
	const bool GIGAMOOG_SAFEMODE = false;
	const std::string GIGAMOOG_IPADDRESS = "192.168.7.179";
	const int GIGAMOOG_IPPORT = 804;
	//const std::string GIGAMOOG_PORT = "COM5";
	//const int GIGAMOOG_BAUDRATE = 115200;
	const double GM_TRIGGER_TIME = 0.0005; //in ms i.e. 0.5us
	const std::vector<std::pair<unsigned, unsigned>> GM_TRIGGER_LINE
		= { std::make_pair(8 - 1,0),std::make_pair(8 - 1,1) }; //load and move
	/*the first is the label on the box minus 1, has minus'd 1 explicitly and this is not used in the code, just a reminder*/

	//Microwave Windfreak
	const bool MICROWAVE_SAFEMODE = false;
	const std::string MICROWAVE_PORT = "COM9";
	const std::pair<unsigned, unsigned> MW_TRIGGER_LINE = std::make_pair(4 - 1, 2); /*the first is the label on the box minus 1, has minus'd 1 explicitly */

	//PicoScrew
	const bool PICOSCREW_SAFEMODE = false;
	const std::string PICOSCREW_KEY = "8742 101956";
	const unsigned PICOSCREW_NUM = 4;
	const std::array<bool, PICOSCREW_NUM> PICOSCREW_CONNECTED = { true,true,true,true };

	//20-bit static DAC
	const bool STATICAO_SAFEMODE = false;
	const std::string STATICAO_IPADDRESS = "192.168.7.165";
	const int STATICAO_IPPORT = 804;

	//static DDS
	const bool STATICDDS_SAFEMODE = false;
	const std::string STATICDDS_PORT = "COM12";
	const unsigned int STATICDDS_BAUDRATE = 115200;

	//Temperature Monitor
	const bool TEMPMON_SAFEMODE = false;
	const unsigned TEMPMON_NUMBER = 5;
	const std::array<std::string, TEMPMON_NUMBER> TEMPMON_ID{ 
		"Cold_Shield", "Cold_Finger", "Cold_Box", "Main_Chamber_Pressure", "Cryostat_side_Pressure"};
	const std::array<std::string, TEMPMON_NUMBER> TEMPMON_SYNTAX{ 
		"SELECT \"Cold Shield\" from ColdEdge order by time desc limit 1", 
		"SELECT \"Cold finger\" from ColdEdge order by time desc limit 1",
		"SELECT \"temperature_B\" from \"Lakeshore331 T\" order by time desc limit 1",
		"SELECT \"pressure\" from B240_main_chamber order by time desc limit 1",
		"SELECT \"Pressure\" from \"Terranova\" order by time desc limit 1" };
#endif
/// Random other Constants
constexpr double PI = 3.14159265358979323846264338327950288;




