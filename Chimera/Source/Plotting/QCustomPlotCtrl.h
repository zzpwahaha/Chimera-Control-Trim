#pragma once
#include "Plotting/dataPoint.h"
#include <QLineSeries>

#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qchart.h>
#include <qchartview.h>
#include <qlineseries.h>
#include <qscatterseries.h>
#include <qobject.h>
#include <../3rd_Party/qcustomplot/qcustomplot.h>

#include <mutex>
#include <vector>
#include <string>
#include <memory>
#include <plotting/PlotCtrl.h>

typedef std::vector<dataPoint> plotDataVec;

/*
* This is a custom object that I use for plotting. All of the drawing is done manually by standard win32 / MFC
* functionality. Plotting used to be done by gnuplot, an external program which my program would send data to in
* real-time, but this custom plotter, while it took some work (it was fun though) allows me to embed plots in the
* main windows and have a little more direct control over the data being plotted.
*/

class QCustomPlotCtrl : public QObject {
	Q_OBJECT;
public:
	QCustomPlotCtrl();
	QCustomPlotCtrl(const QCustomPlotCtrl&) = delete;
	QCustomPlotCtrl(unsigned numTraces, plotStyle inStyle, std::vector<int> thresholds,
		bool narrowOpt = false, bool plotHistOption = false);
	~QCustomPlotCtrl();
	void initializeCalData(calSettings cal);
	void removeData();
	QCPGraph* getCalData();
	// numTraces is used for DAC, OFFSETLOCK or TTL plot for std::vector<std::byte> isShow
	void init(IChimeraQtWindow* parent, QString titleIn, unsigned numTraces = 0);
	dataPoint getMainAnalysisResult();
	void resetChart();
	void setStyle(plotStyle newStyle);
	void setTitle(std::string newTitle);
	void setThresholds(std::vector<int> newThresholds);
	void handleContextMenu(const QPoint& pos);
	QCustomPlot* plot;
	void setControlLocation(QRect loc);
private:

	QCPTextElement* title;
	QCPColorMap* colorMap = nullptr;
	QCPAxisRect* leftAxisRect = nullptr;
	QCPAxisRect* bottomAxisRect = nullptr;
	//const bool narrow;
	std::vector<int> thresholds;
	plotStyle style;
	// first level deliminates different lines which get different colors. second level deliminates different 
	// points within the line.
	bool showLegend = false;
	// used for DAC, OFFSETLOCK or TTL plot to determine whether the trace is shown
	std::vector<std::byte> isShow;
	bool autoScale = true;
	// a colormap that I use for plot stuffs.
	const std::vector<std::array<int, 3>> GIST_RAINBOW_RGB{ { 255 , 0 , 40 },
							{ 255 , 0 , 35 },
							{ 255 , 0 , 30 },
							{ 255 , 0 , 24 },
							{ 255 , 0 , 19 },
							{ 255 , 0 , 14 },
							{ 255 , 0 , 8 },
							{ 255 , 0 , 3 },
							{ 255 , 1 , 0 },
							{ 255 , 7 , 0 },
							{ 255 , 12 , 0 },
							{ 255 , 18 , 0 },
							{ 255 , 23 , 0 },
							{ 255 , 28 , 0 },
							{ 255 , 34 , 0 },
							{ 255 , 39 , 0 },
							{ 255 , 45 , 0 },
							{ 255 , 50 , 0 },
							{ 255 , 55 , 0 },
							{ 255 , 61 , 0 },
							{ 255 , 66 , 0 },
							{ 255 , 72 , 0 },
							{ 255 , 77 , 0 },
							{ 255 , 82 , 0 },
							{ 255 , 88 , 0 },
							{ 255 , 93 , 0 },
							{ 255 , 99 , 0 },
							{ 255 , 104 , 0 },
							{ 255 , 110 , 0 },
							{ 255 , 115 , 0 },
							{ 255 , 120 , 0 },
							{ 255 , 126 , 0 },
							{ 255 , 131 , 0 },
							{ 255 , 137 , 0 },
							{ 255 , 142 , 0 },
							{ 255 , 147 , 0 },
							{ 255 , 153 , 0 },
							{ 255 , 158 , 0 },
							{ 255 , 164 , 0 },
							{ 255 , 169 , 0 },
							{ 255 , 174 , 0 },
							{ 255 , 180 , 0 },
							{ 255 , 185 , 0 },
							{ 255 , 191 , 0 },
							{ 255 , 196 , 0 },
							{ 255 , 201 , 0 },
							{ 255 , 207 , 0 },
							{ 255 , 212 , 0 },
							{ 255 , 218 , 0 },
							{ 255 , 223 , 0 },
							{ 255 , 228 , 0 },
							{ 255 , 234 , 0 },
							{ 255 , 239 , 0 },
							{ 255 , 245 , 0 },
							{ 255 , 250 , 0 },
							{ 254 , 255 , 0 },
							{ 248 , 255 , 0 },
							{ 243 , 255 , 0 },
							{ 237 , 255 , 0 },
							{ 232 , 255 , 0 },
							{ 227 , 255 , 0 },
							{ 221 , 255 , 0 },
							{ 216 , 255 , 0 },
							{ 210 , 255 , 0 },
							{ 205 , 255 , 0 },
							{ 199 , 255 , 0 },
							{ 194 , 255 , 0 },
							{ 189 , 255 , 0 },
							{ 183 , 255 , 0 },
							{ 178 , 255 , 0 },
							{ 172 , 255 , 0 },
							{ 167 , 255 , 0 },
							{ 162 , 255 , 0 },
							{ 156 , 255 , 0 },
							{ 151 , 255 , 0 },
							{ 145 , 255 , 0 },
							{ 140 , 255 , 0 },
							{ 135 , 255 , 0 },
							{ 129 , 255 , 0 },
							{ 124 , 255 , 0 },
							{ 118 , 255 , 0 },
							{ 113 , 255 , 0 },
							{ 108 , 255 , 0 },
							{ 102 , 255 , 0 },
							{ 97 , 255 , 0 },
							{ 91 , 255 , 0 },
							{ 86 , 255 , 0 },
							{ 81 , 255 , 0 },
							{ 75 , 255 , 0 },
							{ 70 , 255 , 0 },
							{ 64 , 255 , 0 },
							{ 59 , 255 , 0 },
							{ 54 , 255 , 0 },
							{ 48 , 255 , 0 },
							{ 43 , 255 , 0 },
							{ 37 , 255 , 0 },
							{ 32 , 255 , 0 },
							{ 27 , 255 , 0 },
							{ 21 , 255 , 0 },
							{ 16 , 255 , 0 },
							{ 10 , 255 , 0 },
							{ 5 , 255 , 0 },
							{ 0 , 255 , 0 },
							{ 0 , 255 , 5 },
							{ 0 , 255 , 10 },
							{ 0 , 255 , 16 },
							{ 0 , 255 , 21 },
							{ 0 , 255 , 26 },
							{ 0 , 255 , 32 },
							{ 0 , 255 , 37 },
							{ 0 , 255 , 43 },
							{ 0 , 255 , 48 },
							{ 0 , 255 , 53 },
							{ 0 , 255 , 59 },
							{ 0 , 255 , 64 },
							{ 0 , 255 , 69 },
							{ 0 , 255 , 75 },
							{ 0 , 255 , 80 },
							{ 0 , 255 , 86 },
							{ 0 , 255 , 91 },
							{ 0 , 255 , 96 },
							{ 0 , 255 , 102 },
							{ 0 , 255 , 107 },
							{ 0 , 255 , 112 },
							{ 0 , 255 , 118 },
							{ 0 , 255 , 123 },
							{ 0 , 255 , 129 },
							{ 0 , 255 , 134 },
							{ 0 , 255 , 139 },
							{ 0 , 255 , 145 },
							{ 0 , 255 , 150 },
							{ 0 , 255 , 155 },
							{ 0 , 255 , 161 },
							{ 0 , 255 , 166 },
							{ 0 , 255 , 172 },
							{ 0 , 255 , 177 },
							{ 0 , 255 , 182 },
							{ 0 , 255 , 188 },
							{ 0 , 255 , 193 },
							{ 0 , 255 , 198 },
							{ 0 , 255 , 204 },
							{ 0 , 255 , 209 },
							{ 0 , 255 , 215 },
							{ 0 , 255 , 220 },
							{ 0 , 255 , 225 },
							{ 0 , 255 , 231 },
							{ 0 , 255 , 236 },
							{ 0 , 255 , 241 },
							{ 0 , 255 , 247 },
							{ 0 , 255 , 252 },
							{ 0 , 251 , 255 },
							{ 0 , 246 , 255 },
							{ 0 , 241 , 255 },
							{ 0 , 235 , 255 },
							{ 0 , 230 , 255 },
							{ 0 , 224 , 255 },
							{ 0 , 219 , 255 },
							{ 0 , 213 , 255 },
							{ 0 , 208 , 255 },
							{ 0 , 202 , 255 },
							{ 0 , 197 , 255 },
							{ 0 , 192 , 255 },
							{ 0 , 186 , 255 },
							{ 0 , 181 , 255 },
							{ 0 , 175 , 255 },
							{ 0 , 170 , 255 },
							{ 0 , 164 , 255 },
							{ 0 , 159 , 255 },
							{ 0 , 154 , 255 },
							{ 0 , 148 , 255 },
							{ 0 , 143 , 255 },
							{ 0 , 137 , 255 },
							{ 0 , 132 , 255 },
							{ 0 , 126 , 255 },
							{ 0 , 121 , 255 },
							{ 0 , 116 , 255 },
							{ 0 , 110 , 255 },
							{ 0 , 105 , 255 },
							{ 0 , 99 , 255 },
							{ 0 , 94 , 255 },
							{ 0 , 88 , 255 },
							{ 0 , 83 , 255 },
							{ 0 , 77 , 255 },
							{ 0 , 72 , 255 },
							{ 0 , 67 , 255 },
							{ 0 , 61 , 255 },
							{ 0 , 56 , 255 },
							{ 0 , 50 , 255 },
							{ 0 , 45 , 255 },
							{ 0 , 39 , 255 },
							{ 0 , 34 , 255 },
							{ 0 , 29 , 255 },
							{ 0 , 23 , 255 },
							{ 0 , 18 , 255 },
							{ 0 , 12 , 255 },
							{ 0 , 7 , 255 },
							{ 0 , 1 , 255 },
							{ 3 , 0 , 255 },
							{ 8 , 0 , 255 },
							{ 14 , 0 , 255 },
							{ 19 , 0 , 255 },
							{ 25 , 0 , 255 },
							{ 30 , 0 , 255 },
							{ 36 , 0 , 255 },
							{ 41 , 0 , 255 },
							{ 47 , 0 , 255 },
							{ 52 , 0 , 255 },
							{ 57 , 0 , 255 },
							{ 63 , 0 , 255 },
							{ 68 , 0 , 255 },
							{ 74 , 0 , 255 },
							{ 79 , 0 , 255 },
							{ 85 , 0 , 255 },
							{ 90 , 0 , 255 },
							{ 95 , 0 , 255 },
							{ 101 , 0 , 255 },
							{ 106 , 0 , 255 },
							{ 112 , 0 , 255 },
							{ 117 , 0 , 255 },
							{ 123 , 0 , 255 },
							{ 128 , 0 , 255 },
							{ 133 , 0 , 255 },
							{ 139 , 0 , 255 },
							{ 144 , 0 , 255 },
							{ 150 , 0 , 255 },
							{ 155 , 0 , 255 },
							{ 161 , 0 , 255 },
							{ 166 , 0 , 255 },
							{ 172 , 0 , 255 },
							{ 177 , 0 , 255 },
							{ 182 , 0 , 255 },
							{ 188 , 0 , 255 },
							{ 193 , 0 , 255 },
							{ 199 , 0 , 255 },
							{ 204 , 0 , 255 },
							{ 210 , 0 , 255 },
							{ 215 , 0 , 255 },
							{ 220 , 0 , 255 },
							{ 226 , 0 , 255 },
							{ 231 , 0 , 255 },
							{ 237 , 0 , 255 },
							{ 242 , 0 , 255 },
							{ 248 , 0 , 255 },
							{ 253 , 0 , 255 },
							{ 255 , 0 , 251 },
							{ 255 , 0 , 245 },
							{ 255 , 0 , 240 },
							{ 255 , 0 , 234 },
							{ 255 , 0 , 229 },
							{ 255 , 0 , 223 },
							{ 255 , 0 , 218 },
							{ 255 , 0 , 212 },
							{ 255 , 0 , 207 },
							{ 255 , 0 , 202 },
							{ 255 , 0 , 196 },
							{ 255 , 0 , 191 } };
	
	const std::vector<std::array<int, 3>> INFERNO{ {1,0,4},
			{1,0,5},
			{1,0,6},
			{2,1,8},
			{2,1,10},
			{3,1,12},
			{4,2,14},
			{4,2,16},
			{5,3,18},
			{6,3,21},
			{7,3,23},
			{8,4,25},
			{10,4,27},
			{11,5,29},
			{12,5,32},
			{14,6,34},
			{15,6,36},
			{17,7,38},
			{18,7,41},
			{19,8,43},
			{21,8,45},
			{22,8,48},
			{24,9,50},
			{25,9,53},
			{27,9,55},
			{28,9,58},
			{30,9,60},
			{32,9,62},
			{33,9,65},
			{35,9,67},
			{37,9,70},
			{39,9,72},
			{40,9,74},
			{42,8,77},
			{44,8,79},
			{46,7,81},
			{47,7,83},
			{49,6,85},
			{51,6,87},
			{53,5,89},
			{55,5,91},
			{56,4,93},
			{58,4,94},
			{60,3,96},
			{61,3,97},
			{63,3,98},
			{65,2,99},
			{66,2,100},
			{68,2,102},
			{70,2,102},
			{71,2,103},
			{73,2,104},
			{74,2,105},
			{76,3,106},
			{77,3,106},
			{79,3,107},
			{80,3,107},
			{82,4,108},
			{83,4,108},
			{85,5,109},
			{86,6,109},
			{88,6,109},
			{89,7,110},
			{91,7,110},
			{92,8,110},
			{94,9,110},
			{95,10,110},
			{97,11,111},
			{98,11,111},
			{99,12,111},
			{101,13,111},
			{102,14,111},
			{104,15,111},
			{105,15,111},
			{107,16,111},
			{108,17,111},
			{110,18,111},
			{111,18,111},
			{112,19,111},
			{114,20,111},
			{115,20,111},
			{117,21,110},
			{118,22,110},
			{120,23,110},
			{121,23,110},
			{123,24,110},
			{124,25,110},
			{126,25,109},
			{127,26,109},
			{129,27,109},
			{130,27,108},
			{131,28,108},
			{133,29,108},
			{134,29,108},
			{136,30,107},
			{137,30,107},
			{139,31,106},
			{140,32,106},
			{142,32,106},
			{143,33,105},
			{145,34,105},
			{146,34,104},
			{148,35,104},
			{149,36,103},
			{151,36,103},
			{152,37,102},
			{154,38,102},
			{155,38,101},
			{157,39,100},
			{158,40,100},
			{160,40,99},
			{161,41,98},
			{162,42,98},
			{164,42,97},
			{165,43,96},
			{167,44,96},
			{168,44,95},
			{170,45,94},
			{171,46,93},
			{173,47,93},
			{174,47,92},
			{176,48,91},
			{177,49,90},
			{178,50,89},
			{180,51,88},
			{181,51,88},
			{183,52,87},
			{184,53,86},
			{185,54,85},
			{187,55,84},
			{188,56,83},
			{189,57,82},
			{191,57,81},
			{192,58,80},
			{194,59,79},
			{195,60,78},
			{196,61,77},
			{197,62,76},
			{199,63,75},
			{200,64,74},
			{201,65,73},
			{203,66,72},
			{204,67,71},
			{205,69,70},
			{206,70,69},
			{207,71,67},
			{209,72,66},
			{210,73,65},
			{211,74,64},
			{212,76,63},
			{213,77,62},
			{214,78,61},
			{215,79,59},
			{217,81,58},
			{218,82,57},
			{219,83,56},
			{220,85,55},
			{221,86,54},
			{222,87,52},
			{223,89,51},
			{224,90,50},
			{224,92,49},
			{225,93,48},
			{226,95,46},
			{227,96,45},
			{228,98,44},
			{229,99,43},
			{230,101,41},
			{230,102,40},
			{231,104,39},
			{232,105,37},
			{233,107,36},
			{233,108,35},
			{234,110,34},
			{235,112,32},
			{235,113,31},
			{236,115,29},
			{237,117,28},
			{237,118,27},
			{238,120,25},
			{238,122,24},
			{239,124,22},
			{239,125,21},
			{240,127,19},
			{240,129,18},
			{241,130,16},
			{241,132,15},
			{241,134,13},
			{242,136,12},
			{242,138,10},
			{242,139,8},
			{243,141,7},
			{243,143,6},
			{243,145,4},
			{244,147,3},
			{244,149,3},
			{244,150,2},
			{244,152,1},
			{244,154,1},
			{244,156,1},
			{244,158,1},
			{245,160,1},
			{245,162,2},
			{245,164,2},
			{245,166,3},
			{245,168,5},
			{245,169,6},
			{244,171,8},
			{244,173,10},
			{244,175,12},
			{244,177,14},
			{244,179,17},
			{244,181,19},
			{244,183,21},
			{243,185,24},
			{243,187,26},
			{243,189,28},
			{243,191,31},
			{242,193,33},
			{242,195,36},
			{242,197,38},
			{241,199,41},
			{241,201,44},
			{240,203,46},
			{240,205,49},
			{240,207,52},
			{239,209,55},
			{239,211,58},
			{238,213,61},
			{238,215,64},
			{237,217,67},
			{237,219,70},
			{236,220,73},
			{235,222,77},
			{235,224,80},
			{234,226,84},
			{234,228,87},
			{234,230,91},
			{233,232,95},
			{233,234,99},
			{233,235,103},
			{232,237,107},
			{232,239,111},
			{232,240,115},
			{233,242,119},
			{233,243,124},
			{234,245,128},
			{234,246,132},
			{235,248,136},
			{236,249,140},
			{237,250,144},
			{238,251,148},
			{240,252,152},
			{241,254,156},
			{243,255,160},
			{245,255,163} };
	const std::vector<double>INFERNO_POSITION{ {0.0000,
				0.0039,
				0.0078,
				0.0118,
				0.0157,
				0.0196,
				0.0235,
				0.0275,
				0.0314,
				0.0353,
				0.0392,
				0.0431,
				0.0471,
				0.0510,
				0.0549,
				0.0588,
				0.0627,
				0.0667,
				0.0706,
				0.0745,
				0.0784,
				0.0824,
				0.0863,
				0.0902,
				0.0941,
				0.0980,
				0.1020,
				0.1059,
				0.1098,
				0.1137,
				0.1176,
				0.1216,
				0.1255,
				0.1294,
				0.1333,
				0.1373,
				0.1412,
				0.1451,
				0.1490,
				0.1529,
				0.1569,
				0.1608,
				0.1647,
				0.1686,
				0.1725,
				0.1765,
				0.1804,
				0.1843,
				0.1882,
				0.1922,
				0.1961,
				0.2000,
				0.2039,
				0.2078,
				0.2118,
				0.2157,
				0.2196,
				0.2235,
				0.2275,
				0.2314,
				0.2353,
				0.2392,
				0.2431,
				0.2471,
				0.2510,
				0.2549,
				0.2588,
				0.2627,
				0.2667,
				0.2706,
				0.2745,
				0.2784,
				0.2824,
				0.2863,
				0.2902,
				0.2941,
				0.2980,
				0.3020,
				0.3059,
				0.3098,
				0.3137,
				0.3176,
				0.3216,
				0.3255,
				0.3294,
				0.3333,
				0.3373,
				0.3412,
				0.3451,
				0.3490,
				0.3529,
				0.3569,
				0.3608,
				0.3647,
				0.3686,
				0.3725,
				0.3765,
				0.3804,
				0.3843,
				0.3882,
				0.3922,
				0.3961,
				0.4000,
				0.4039,
				0.4078,
				0.4118,
				0.4157,
				0.4196,
				0.4235,
				0.4275,
				0.4314,
				0.4353,
				0.4392,
				0.4431,
				0.4471,
				0.4510,
				0.4549,
				0.4588,
				0.4627,
				0.4667,
				0.4706,
				0.4745,
				0.4784,
				0.4824,
				0.4863,
				0.4902,
				0.4941,
				0.4980,
				0.5020,
				0.5059,
				0.5098,
				0.5137,
				0.5176,
				0.5216,
				0.5255,
				0.5294,
				0.5333,
				0.5373,
				0.5412,
				0.5451,
				0.5490,
				0.5529,
				0.5569,
				0.5608,
				0.5647,
				0.5686,
				0.5725,
				0.5765,
				0.5804,
				0.5843,
				0.5882,
				0.5922,
				0.5961,
				0.6000,
				0.6039,
				0.6078,
				0.6118,
				0.6157,
				0.6196,
				0.6235,
				0.6275,
				0.6314,
				0.6353,
				0.6392,
				0.6431,
				0.6471,
				0.6510,
				0.6549,
				0.6588,
				0.6627,
				0.6667,
				0.6706,
				0.6745,
				0.6784,
				0.6824,
				0.6863,
				0.6902,
				0.6941,
				0.6980,
				0.7020,
				0.7059,
				0.7098,
				0.7137,
				0.7176,
				0.7216,
				0.7255,
				0.7294,
				0.7333,
				0.7373,
				0.7412,
				0.7451,
				0.7490,
				0.7529,
				0.7569,
				0.7608,
				0.7647,
				0.7686,
				0.7725,
				0.7765,
				0.7804,
				0.7843,
				0.7882,
				0.7922,
				0.7961,
				0.8000,
				0.8039,
				0.8078,
				0.8118,
				0.8157,
				0.8196,
				0.8235,
				0.8275,
				0.8314,
				0.8353,
				0.8392,
				0.8431,
				0.8471,
				0.8510,
				0.8549,
				0.8588,
				0.8627,
				0.8667,
				0.8706,
				0.8745,
				0.8784,
				0.8824,
				0.8863,
				0.8902,
				0.8941,
				0.8980,
				0.9020,
				0.9059,
				0.9098,
				0.9137,
				0.9176,
				0.9216,
				0.9255,
				0.9294,
				0.9333,
				0.9373,
				0.9412,
				0.9451,
				0.9490,
				0.9529,
				0.9569,
				0.9608,
				0.9647,
				0.9686,
				0.9725,
				0.9765,
				0.9804,
				0.9843,
				0.9882,
				0.9922,
				0.9961,
				1.0000} };



public Q_SLOTS:
	void setData(std::vector<plotDataVec> newData, 
		std::vector<std::string> legends = std::vector<std::string>());

};

//Q_DECLARE_METATYPE(std::vector<std::vector<plotDataVec>>)
Q_DECLARE_METATYPE(std::vector<std::string>)