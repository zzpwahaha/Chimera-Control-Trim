// created by Mark O. Brown
#include "stdafx.h"
#include "QCustomPlotCtrl.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <AnalogInput/CalibrationManager.h>
#include <numeric>
#include <qgraphicslayout.h>
#include <qdebug.h>
#include <qmenu.h>
#include <qdialogbuttonbox.h>
#include <qsizepolicy.h>

QCustomPlotCtrl::QCustomPlotCtrl() :
	style(plotStyle::GeneralErrorPlot)
{
}

QCustomPlotCtrl::QCustomPlotCtrl(unsigned numTraces, plotStyle inStyle, std::vector<int> thresholds_in,
	bool narrowOpt, bool plotHistOption) :
	style(inStyle)/*, narrow(narrowOpt)*/ {
}

QCustomPlotCtrl::~QCustomPlotCtrl() {
}


void QCustomPlotCtrl::init(IChimeraQtWindow* parent, QString titleIn, unsigned numTraces)
{
	plot = new QCustomPlot(parent);
	plot->setContextMenuPolicy(Qt::CustomContextMenu);
	parent->connect(plot, &QCustomPlot::customContextMenuRequested,
		[this, parent](const QPoint& pos2) {
			try {
				handleContextMenu(pos2);
			}
			catch (ChimeraError& err) {
				parent->reportErr(err.qtrace());
			}
		});
	title = new QCPTextElement(plot, titleIn);
	plot->plotLayout()->insertRow(0);
	plot->plotLayout()->addElement(0, 0, title);
	if (this->style == plotStyle::DensityPlot || this->style == plotStyle::DensityPlotWithHisto) {
		//QCPAxisRect* centerAxisRect = new QCPAxisRect(plot);
		centerAxisRect = plot->axisRect();
		centerAxisRect->setupFullAxesBox(true);
		centerAxisRect->setRangeZoomFactor(0.95);
		//plot->plotLayout()->addElement(0, 0, centerAxisRect);
		/*colorbar*/
		QCPColorScale* colorScale = new QCPColorScale(plot);
		colorScale->setType(QCPAxis::atRight);
		colorScale->setRangeZoom(false);
		colorScale->setRangeDrag(false);
		plot->plotLayout()->addElement(1, 1, colorScale);
		QCPColorGradient tmpCG = QCPColorGradient();
		tmpCG.setColorInterpolation(QCPColorGradient::ciRGB);
		for (size_t i = 0; i < INFERNO_POSITION.size(); i++)
		{
			tmpCG.setColorStopAt(INFERNO_POSITION[i], QColor(INFERNO[i][0], INFERNO[i][1], INFERNO[i][2]));
		}
		colorScale->setGradient(tmpCG);
		/*set alignment of plot + 1 colorbar*/
		QCPMarginGroup* marginGroup = new QCPMarginGroup(plot);
		centerAxisRect->setMarginGroup(QCP::msAll, marginGroup);
		colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
		plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
		/*color map and cmap gradient*/
		colorMap = new QCPColorMap(centerAxisRect->axis(QCPAxis::atBottom), centerAxisRect->axis(QCPAxis::atLeft));
		colorMap->setInterpolate(false);
		colorMap->setColorScale(colorScale);
		/*setup a ticker for colormap that only gives integer ticks:*/
		QCPAxisTickerFixed* intTicker = new QCPAxisTickerFixed();
		intTicker->setTickStep(1.0);
		intTicker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
		colorMap->keyAxis()->setTicker(QSharedPointer<QCPAxisTickerFixed>(intTicker));
		colorMap->valueAxis()->setTicker(QSharedPointer<QCPAxisTickerFixed>(intTicker));
		auto c = \
		connect(plot, &QCustomPlot::mouseDoubleClick, this, [this]() {
			colorMap->rescaleAxes();
			colorMap->rescaleDataRange(true);
			colorMap->colorScale()->rescaleDataRange(true);
			plot->yAxis->setScaleRatio(plot->xAxis, 1.0);
			plot->replot(); });
		if (this->style == plotStyle::DensityPlotWithHisto) {
			autoScale = false;
			plot->setInteractions(0x000);
			disconnect(c);
			plot->plotLayout()->insertColumn(0);
			leftAxisRect = new QCPAxisRect(plot);
			bottomAxisRect = new QCPAxisRect(plot);
			plot->plotLayout()->addElement(1, 0, leftAxisRect);
			plot->plotLayout()->addElement(2, 1, bottomAxisRect);
			leftAxisRect->axis(QCPAxis::atLeft)->setLabelFont(QFont("Times", 10));
			bottomAxisRect->axis(QCPAxis::atBottom)->setLabelFont(QFont("Times", 10));
			//plot->axisRect(1)->axis(QCPAxis::atLeft)->setTickLabels(false);
			//plot->axisRect(1)->axis(QCPAxis::atBottom)->setTickLabels(false);
			leftAxisRect->axis(QCPAxis::atRight)->setTickLabels(false);
			bottomAxisRect->axis(QCPAxis::atTop)->setTickLabels(false);
			leftAxisRect->axis(QCPAxis::atLeft)->setTicker(QSharedPointer<QCPAxisTickerFixed>(intTicker));
			bottomAxisRect->axis(QCPAxis::atBottom)->setTicker(QSharedPointer<QCPAxisTickerFixed>(intTicker));
			//leftAxisRect->axis(QCPAxis::atLeft)->setTickLabelRotation(90);
			leftAxisRect->setupFullAxesBox(false);
			bottomAxisRect->setupFullAxesBox(false);
			/*set alignment of three plots + 1 colorbar*/
			bottomAxisRect->setMarginGroup(QCP::msLeft | QCP::msRight, marginGroup);
			leftAxisRect->setMarginGroup(QCP::msTop | QCP::msBottom, marginGroup);
			bottomAxisRect->setMaximumSize(1500, 10);
			leftAxisRect->setMaximumSize(10, 2000);
			//plot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

			plot->plotLayout()->setColumnSpacing(0);
			plot->plotLayout()->setRowSpacing(0);
			//leftAxisRect->setAutoMargins(QCP::msTop | QCP::msBottom);
			//bottomAxisRect->setAutoMargins(QCP::msNone);
			//plot->axisRect(1)->setAutoMargins(QCP::msNone);
			//plot->axisRect(2)->setAutoMargins(QCP::msNone);
			leftAxisRect->setMinimumMargins(QMargins(0, 0, 0, 0));
			//leftAxisRect->setMargins(QMargins(15, 0, 0, 0));
			//bottomAxisRect->setMargins(QMargins(0, 0, 0, 15));
			plot->axisRect(1)->setMargins(QMargins(5, 5, 5, 5));
			//plot->axisRect(2)->setMargins(QMargins(0, 0, 0, 0));
			bottomAxisRect->setMinimumMargins(QMargins(0, 0, 0, 0));
			plot->axisRect(1)->setMinimumMargins(QMargins(0, 0, 0, 0));
			//title->setMaximumSize(1, 1);
			plot->plotLayout()->take(title);
			plot->plotLayout()->simplify();
		}
		// move newly created axes on "axes" layer and grids on "grid" layer:
		foreach(QCPAxisRect * rect, plot->axisRects())
		{
			foreach(QCPAxis * axis, rect->axes())
			{
				axis->setLayer("axes");
				axis->grid()->setLayer("grid");
			}
		}
	}
	if (this->style == plotStyle::DacPlot || this->style == plotStyle::TtlPlot) {
		plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
		plot->axisRect()->setRangeZoomFactor(0.95);
		if (numTraces == 0) {
			errBox("For DAC, TTL, OFFSETLOCK, you should spec the number of trace you want.");
		}
		isShow = std::vector<std::byte>(numTraces, std::byte(1));
		connect(plot, &QCustomPlot::mouseDoubleClick, this, [this]() {
			plot->rescaleAxes();
			plot->replot(); });
		//connect(plot, &QCustomPlot::plottableClick, this, [this](QCPAbstractPlottable* p, int dataIndex) {
		//	});
	}
	if (this->style == plotStyle::GeneralErrorPlot  
		|| this->style == plotStyle::DensityPlot 
		|| this->style == plotStyle::DensityPlotWithHisto) {
		autoScale = true;
		autoScaleCMap = false; // not used for GeneralErrorPlot
		connect(plot, &QCustomPlot::mouseDoubleClick, this, [this]() {
			plot->rescaleAxes();
			plot->replot(); });
	}

	//plot->setMinimumSize(600, 400);
	resetChart();
	if (this->style == plotStyle::DensityPlot || this->style == plotStyle::DensityPlotWithHisto) {
		colorMap->data()->setSize(10, 10);
	}
}

void QCustomPlotCtrl::handleContextMenu(const QPoint& pos) {
	QMenu menu;
	menu.setStyleSheet(chimeraStyleSheets::stdStyleSheet());
	auto* clear = new QAction("Clear Plot", &menu);
	plot->connect(clear, &QAction::triggered,
		[this]() {
			plot->clearGraphs();
			plot->replot();
		});
	menu.addAction(clear);
	if (this->style != plotStyle::DensityPlot && this->style != plotStyle::DensityPlotWithHisto) {
		auto* leg = new QAction("Toggle Legend", &menu);
		plot->connect(leg, &QAction::triggered,
			[this]() {
				showLegend = !showLegend;
				resetChart();
			});
		menu.addAction(leg);
	}

	if (this->style == plotStyle::DensityPlotWithHisto) {
		auto* side = new QAction("Hide Side Plot", &menu);
		side->setCheckable(true);
		side->setChecked(!showSidePlot);
		plot->connect(side, &QAction::triggered, [this, side](bool checked) {
			if (checked) {
				leftAxisRect->setVisible(false);
				bottomAxisRect->setVisible(false);
				leftAxisRect->layout()->take(leftAxisRect);
				bottomAxisRect->layout()->take(bottomAxisRect);
				plot->plotLayout()->simplify();
				resetChart();
				showSidePlot = false;
			}
			else {
				leftAxisRect->setVisible(true);
				bottomAxisRect->setVisible(true);
				plot->plotLayout()->insertColumn(0);//somehow do not need to insert row
				plot->plotLayout()->addElement(0, 0, leftAxisRect);
				plot->plotLayout()->addElement(1, 1, bottomAxisRect);
				plot->plotLayout()->simplify();
				resetChart();
				showSidePlot = true;
			}
		});
		menu.addAction(side);
	}

	if (this->style == plotStyle::DacPlot || this->style == plotStyle::TtlPlot) {
		auto* tra = new QAction("Toggle Traces", &menu);
		plot->connect(tra, &QAction::triggered, [this]() {
			auto graphs = plot->axisRect()->graphs();
			if (graphs.size() != isShow.size()) {
				errBox("There is currently " + qstr(graphs.size()) + " traces in the plot which does not match the"
					" number of plot control for the traces: " + qstr(isShow.size()) + ". Please make sure you run-ed the experiemnt and the plot is updated");
			}
			QDialog* diag = new QDialog();
			diag->setModal(false);
			diag->setWindowTitle(title->text() + ": Select To Show");
			diag->setAttribute(Qt::WA_DeleteOnClose);
			QVBoxLayout* layoutDiag = new QVBoxLayout();
			diag->setLayout(layoutDiag);
			QHBoxLayout* layout = new QHBoxLayout();
			QVBoxLayout* layout1 = new QVBoxLayout();
			QVBoxLayout* layout2 = new QVBoxLayout();
			layout->addLayout(layout1);
			layout->addLayout(layout2);
			std::vector<QCheckBox*> chkbrd;
			for (unsigned i = 0; i < graphs.size(); i++) {
				chkbrd.push_back(new QCheckBox(graphs.at(i)->name()));
				chkbrd.back()->setChecked(isShow[i] > std::byte(0));
				if (i < graphs.size() / 2) {
					layout1->addWidget(chkbrd.back());
				}
				else {
					layout2->addWidget(chkbrd.back());
				}
			}
			QDialogButtonBox* diagbutton = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			layoutDiag->addLayout(layout, 1);
			layoutDiag->addWidget(diagbutton, 0);
			connect(diagbutton, &QDialogButtonBox::accepted, [this, chkbrd, diag]() {
				auto graphs = plot->axisRect()->graphs();
				for (unsigned idx = 0; idx < chkbrd.size(); idx++) {
					isShow[idx] = std::byte(chkbrd[idx]->isChecked() ? 1 : 0);
					graphs.at(idx)->setVisible(chkbrd[idx]->isChecked());
				}
				plot->replot();
				diag->close(); });
			QPoint p = QCursor::pos();
			diag->show();
			diag->move(p.x() - diag->width() / 2, p.y() - diag->height() / 2); // show will update the diag size
			});
		menu.addAction(tra);
		auto* autos = new QAction("Disable Auto Scale", &menu);
		autos->setCheckable(true);
		autos->setChecked(!autoScale);
		plot->connect(autos, &QAction::triggered, [this, autos]() {
			autoScale = !autos->isChecked(); });
		menu.addAction(autos);

		if (plot->selectedGraphs().size() > 0) {
			menu.addAction("Hide selected graph", this, [this]() {
				auto selectedGras = plot->selectedGraphs();
				auto* selectedGra = plot->selectedGraphs().first();
				if (selectedGras.size() > 0) {
					auto gras = plot->axisRect()->graphs();
					int idx = gras.indexOf(selectedGra);
					selectedGra->setVisible(false);
					if (idx >= 0) {
						isShow[idx] = std::byte(0);
					}
					plot->replot();
				}});
		}

	}
	if (this->style == plotStyle::GeneralErrorPlot 
		|| this->style == plotStyle::DensityPlotWithHisto
		|| this->style == plotStyle::DensityPlot) {
		auto* autos = menu.addAction("Disable Auto Scale");
		autos->setCheckable(true);
		autos->setChecked(!autoScale);
		plot->connect(autos, &QAction::triggered, [this, autos]() {
			autoScale = !autos->isChecked(); });

		auto* zoom = menu.addAction("Enable Scroll Zoom");
		zoom->setCheckable(true);
		zoom->setChecked(QCP::iRangeZoom& plot->interactions() ? true : false);
		plot->connect(zoom, &QAction::triggered, [this, zoom]() {
			if (zoom->isChecked()) {
				plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
				plot->axisRect()->setRangeZoomFactor(0.95);
			}
			else {
				plot->setInteractions(0x000);
			} });
	}
	if (this->style == plotStyle::DensityPlotWithHisto || this->style == plotStyle::DensityPlot) {
		auto* autos = menu.addAction("Disable Auto Color Scale");
		autos->setCheckable(true);
		autos->setChecked(!autoScaleCMap);
		plot->connect(autos, &QAction::triggered, [this, autos]() {
			autoScaleCMap = !autos->isChecked(); });
	}
	
	menu.exec(plot->mapToGlobal(pos));
}

dataPoint QCustomPlotCtrl::getMainAnalysisResult() {
	// get the average data. If not only a single data point, as this is currently ment to be used, then I'm not 
	// positive what value this is grabbing... maybe the last point of the average?
	return dataPoint();
}

QCPGraph* QCustomPlotCtrl::getCalData() {
	if (plot->graphCount() == 0) {
		plot->addGraph();
	}
	return plot->graph(0);
}

void QCustomPlotCtrl::removeData() {
	//plot->clearGraphs();
	if (this->style != plotStyle::DensityPlot && this->style != plotStyle::DensityPlotWithHisto) {
		plot->clearPlottables();
	}
}

void QCustomPlotCtrl::initializeCalData(calSettings cal) {
	if (plot->graphCount() == 0) {
		plot->addGraph();
	}
	plot->graph(0)->setPen(QColor(Qt::blue));
	plot->graph(0)->setLineStyle(QCPGraph::lsNone);
	plot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
	plot->graph(0)->setData({}, {});
}

// legends is currently only used for DAC, OFFSETLOCK or TTL plot
void QCustomPlotCtrl::setData(std::vector<plotDataVec> newData, std::vector<std::string> legends)
{
	removeData();
	if (style == plotStyle::CalibrationPlot) {
		// need to rename...
		if (newData.size() != 3 || !plot) {
			// should always have the calibration data, historical fit, and recent fit.
			return;
		}
		// first data set is scatter
		auto& newLineData = newData[0];
		initializeCalData(calSettings());
		QVector<double> calXdata, calYdata;
		for (auto count : range(newLineData.size())) {
			calXdata.append(newLineData[count].x);
			calYdata.append(newLineData[count].y);
		}
		plot->addGraph();
		plot->graph(0)->setName("Data");
		plot->graph(0)->setLineStyle(QCPGraph::lsNone);
		plot->graph(0)->setPen(QColor(Qt::blue));
		plot->graph(0)->setScatterStyle(QCPScatterStyle::ssCircle);
		plot->graph(0)->setData(calXdata, calYdata);
		// second line
		auto& newLineData2 = newData[1];
		QCPCurve* curve1 = new QCPCurve(plot->xAxis, plot->yAxis);
		QVector<QCPCurveData> fitData;
		for (auto count : range(newLineData2.size())) {
			fitData.push_back(QCPCurveData(count, newLineData2[count].x, newLineData2[count].y));
		}
		curve1->data()->set(fitData);
		curve1->setPen(QPen(QBrush(QColor(Qt::red)), 1.5));
		curve1->setName("Fit");
		// third line
		auto& newLineData3 = newData[2];
		QCPCurve* curve2 = new QCPCurve(plot->xAxis, plot->yAxis);
		QVector<QCPCurveData> histCalData;
		for (auto count : range(newLineData3.size())) {
			histCalData.push_back(QCPCurveData(count, newLineData3[count].x, newLineData3[count].y));
		}
		curve2->data()->set(histCalData);
		curve2->setPen(QPen(QBrush(QColor("orange")), 1.5));
		curve2->setName("Historical Fit");
	}
	else if (style == plotStyle::BinomialDataPlot || style == plotStyle::GeneralErrorPlot) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		for (auto traceNum : range(newData.size())) {
			auto& newLine = newData[traceNum];
			QVector<double> newXdata, newYdata, yerrData;
			for (auto count : range(newLine.size())) {
				newXdata.append(newLine[count].x);
				newYdata.append(newLine[count].y);
				yerrData.append(newLine[count].err);
			}
			plot->addGraph();
			QCPErrorBars* errorBars = new QCPErrorBars(plot->xAxis, plot->yAxis);
			errorBars->removeFromLegend();
			errorBars->setAntialiased(false);
			errorBars->setDataPlottable(plot->graph());
			if (traceNum == newData.size() - 1) {
				plot->graph()->setName("Avg.");
				plot->graph()->setPen(QColor(Qt::blue));
				errorBars->setPen(QColor(Qt::blue));
				plot->graph()->setScatterStyle(QCPScatterStyle::ssDisc);
			}
			else {
				plot->graph()->setName("DSet " + qstr(traceNum + 1));
				auto gcolor = GIST_RAINBOW_RGB[traceNum * GIST_RAINBOW_RGB.size() / newData.size()];
				auto qtColor = QColor(gcolor[0], gcolor[1], gcolor[2], 25);
				plot->graph()->setPen(qtColor);
				errorBars->setPen(qtColor);
				plot->graph()->setScatterStyle(QCPScatterStyle::ssCircle);
			}			
			plot->graph()->setLineStyle(QCPGraph::lsNone);
			plot->graph()->setData(newXdata, newYdata);
			errorBars->setData(yerrData);
		}
	}
	else if (style == plotStyle::HistPlot) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		unsigned lineCount = 0;
		// ignore the last data point, which is supposedly the avg, which is not used in histo
		for (auto traceNum : range(newData.size() - 1)) {
			auto& newLine = newData[traceNum];
			if (newLine.size() > 1000) {
				continue; // something very wrong...
			}
			auto color = GIST_RAINBOW_RGB[(lineCount++) * GIST_RAINBOW_RGB.size() / newData.size()];
			
			QVector<double> newXdata, newYdata;
			for (auto count : range(newLine.size())) {
				newXdata.append(newLine[count].x);
				newYdata.append(newLine[count].y);
			}
			plot->addGraph();
			plot->graph()->setPen(QColor(color[0], color[1], color[2], 150));
			plot->graph()->setName("DSet " + qstr(traceNum + 1));
			plot->graph()->addData(newXdata, newYdata);
		}
		lineCount = 0;
		for (auto thresholdNum : range(thresholds.size())) {
			QCPItemStraightLine* infLine = new QCPItemStraightLine(plot);
			infLine->point1->setCoords(thresholds[thresholdNum], 0);  
			infLine->point2->setCoords(thresholds[thresholdNum], 1);  
			auto color = GIST_RAINBOW_RGB[(lineCount++) * GIST_RAINBOW_RGB.size() / newData.size()];
			infLine->setPen(QColor(color[0], color[1], color[2], 150));
		}
	}
	else if (style == plotStyle::DensityPlot) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		int Height= newData.size();
		int Width = newData[0].size();
		auto pp = plot->axisRect()->plottables().at(0);
		colorMap->data()->setRange(QCPRange(0, Width - 1), QCPRange(0, Height - 1));
		colorMap->data()->setSize(Width, Height);
		for (size_t i = 0; i < Height; i++) {
			for (size_t j = 0; j < Width; j++) {
				colorMap->data()->setCell(j, i, newData[i][j].y);
			}
		}
	}
	else if (style == plotStyle::DensityPlotWithHisto) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		int Height = newData.size();
		int Width = newData[0].size();
		//auto pp = centerAxisRect->plottables().at(0);
		colorMap->data()->setRange(QCPRange(0, Width - 1), QCPRange(0, Height - 1));
		colorMap->data()->setSize(Width, Height);
		for (size_t i = 0; i < Height; i++) {
			for (size_t j = 0; j < Width; j++) {
				colorMap->data()->setCell(j, i, newData[i][j].y);
			}
		}
	}
	else if (style == plotStyle::PicturePlot) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		unsigned lineCount = 0;
		for (auto traceNum : range(newData.size())) {
			QCPCurve* newCurve = new QCPCurve(plot->xAxis, plot->yAxis);
			auto& newLine = newData[traceNum];
			auto color = GIST_RAINBOW_RGB[lineCount * GIST_RAINBOW_RGB.size() / newData.size()];
			QVector<double> newXdata, newYdata;
			for (auto count : range(newLine.size())) {
				newXdata.append(newLine[count].x);
				newYdata.append(newLine[count].y);
			}
			newCurve->setData(newXdata, newYdata);
			newCurve->setPen(QColor(color[0], color[1], color[2]));
			lineCount++;
		}
	}
	else if (style == plotStyle::DacPlot || style == plotStyle::TtlPlot) {
		if (newData.size() == 0 || !plot) {
			return;
		}
		unsigned lineCount = 0;
		bool addLegend = newData.size() != legends.size() ? false : true;
		for (auto traceNum : range(newData.size())) {
			auto& newLine = newData[traceNum];
			auto color = GIST_RAINBOW_RGB[lineCount * GIST_RAINBOW_RGB.size() / newData.size()];
			QVector<double> newXdata, newYdata;
			for (auto count : range(newLine.size())) {
				newXdata.append(newLine[count].x);
				newYdata.append(newLine[count].y);
			}
			lineCount++;
			plot->addGraph();
			plot->graph()->setPen(QColor(color[0], color[1], color[2]));
			plot->graph()->addData(newXdata, newYdata);
			plot->graph()->setName(qstr(legends[traceNum]));
			if (addLegend) {
				plot->graph()->setName(qstr(legends[traceNum]));
			}
			plot->graph()->setVisible(isShow[traceNum] == std::byte(0) ? false : true);

		}
	}
	else { // line plot... not sure specificly
		if (newData.size() == 0 || !plot) {
			return;
		}
		unsigned lineCount = 0;
		bool addLegend = newData.size() != legends.size() ? false : true;
		for (auto traceNum : range(newData.size())) {
			auto& newLine = newData[traceNum];
			auto color = GIST_RAINBOW_RGB[lineCount * GIST_RAINBOW_RGB.size() / newData.size()];
			QVector<double> newXdata, newYdata;
			for (auto count : range(newLine.size())) {
				newXdata.append(newLine[count].x);
				newYdata.append(newLine[count].y);
			}
			lineCount++;
			plot->addGraph();
			plot->graph()->setPen(QColor(color[0],color[1],color[2]));
			plot->graph()->addData(newXdata, newYdata);
			if (addLegend) {
				plot->graph()->setName(qstr(legends[traceNum]));
			}
		}
	}
	resetChart();
	plot->replot();
}

void QCustomPlotCtrl::setTitle(std::string newTitle) {
	title->setText(qstr(newTitle));
}

void QCustomPlotCtrl::setThresholds(std::vector<int> newThresholds) {
	thresholds = newThresholds;
}

void QCustomPlotCtrl::setStyle(plotStyle newStyle) {
	style = newStyle;
}

void QCustomPlotCtrl::resetChart() {
	if (this->style != plotStyle::DensityPlot && this->style != plotStyle::DensityPlotWithHisto) {
		if (!showLegend) {
			plot->legend->setVisible(false);
		}
		else {
			plot->legend->setVisible(true);
		}
	}

	if (style == plotStyle::BinomialDataPlot) {
		plot->yAxis->setRange({ 0,1 });
		plot->xAxis->rescale();
	}
	else if (style == plotStyle::HistPlot) {
		plot->yAxis->rescale();
		plot->yAxis->setRangeLower(0);
		plot->xAxis->rescale();
	}
	else if (style == plotStyle::DensityPlot) {
		int width = colorMap->data()->keyRange().size();
		int height = colorMap->data()->valueRange().size();
		if (autoScaleCMap) {
			colorMap->rescaleDataRange(true);
			colorMap->colorScale()->rescaleDataRange(true);
		}
		if (autoScale) {
			colorMap->rescaleAxes();
		}
		//plot->yAxis->setScaleRatio(plot->xAxis, 1.0);
		width > height ? centerAxisRect->axis(QCPAxis::atLeft)->setScaleRatio(centerAxisRect->axis(QCPAxis::atBottom), 1.0) :
			centerAxisRect->axis(QCPAxis::atBottom)->setScaleRatio(centerAxisRect->axis(QCPAxis::atLeft), 1.0);
	}
	else if (style == plotStyle::DensityPlotWithHisto) {
		int width = colorMap->data()->keyRange().size();
		int height = colorMap->data()->valueRange().size();
		centerAxisRect->axis(QCPAxis::atLeft)->setScaleRatio(
			centerAxisRect->axis(QCPAxis::atBottom), height / width);
		if (autoScaleCMap) {
			colorMap->rescaleDataRange(true);
			colorMap->colorScale()->rescaleDataRange(true);
		}
		if (autoScale) {
			colorMap->rescaleAxes();
		}

		//width > height ? colorMap->valueAxis()->scaleRange(width / height) :
		//	colorMap->keyAxis()->scaleRange(height / width);
		width > height ? centerAxisRect->axis(QCPAxis::atLeft)->setScaleRatio(centerAxisRect->axis(QCPAxis::atBottom), 1.0) :
			centerAxisRect->axis(QCPAxis::atBottom)->setScaleRatio(centerAxisRect->axis(QCPAxis::atLeft), 1.0);
		

		bottomAxisRect->axis(QCPAxis::atBottom)->setRange(centerAxisRect->axis(QCPAxis::atBottom)->range());
		leftAxisRect->axis(QCPAxis::atLeft)->setRange(centerAxisRect->axis(QCPAxis::atLeft)->range());
	}
	else if (style == plotStyle::DacPlot || style == plotStyle::TtlPlot || style == plotStyle::GeneralErrorPlot) {
		if (autoScale) {
			plot->rescaleAxes();
		}
	}
	else {
		plot->rescaleAxes();
	}
	//plot->yAxis->scaleRange(1.1, plot->yAxis->range().center());
	//plot->xAxis->scaleRange(1.1, plot->yAxis->range().center());
	auto defs = chimeraStyleSheets::getDefs();
	auto neutralColor = QColor(defs["@NeutralTextColor"]);
	auto generalPen = QPen(neutralColor);

	plot->xAxis->setBasePen(generalPen);
	plot->xAxis->setLabelColor(neutralColor);
	plot->yAxis->setBasePen(generalPen);
	plot->yAxis->setLabelColor(neutralColor);

	plot->xAxis->setTickPen(generalPen);
	plot->yAxis->setTickPen(generalPen);
	plot->xAxis->setSubTickPen(generalPen);
	plot->yAxis->setSubTickPen(generalPen);
	plot->xAxis->setTickLabelColor(neutralColor);
	plot->yAxis->setTickLabelColor(neutralColor);
	plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignLeft);
	if (this->style == plotStyle::DensityPlot || this->style == plotStyle::DensityPlotWithHisto) {
		auto legendColor = QColor(defs["@StaticBackground"]);
		legendColor.setAlpha(150);
		plot->legend->setBrush(legendColor);
		plot->legend->setTextColor(neutralColor);
	}
	plot->setBackground(QColor(defs["@StaticBackground"]));
	title->setTextColor(defs["@NeutralTextColor"]);
	plot->replot();
}

void QCustomPlotCtrl::setControlLocation(QRect loc) {
	plot->setGeometry(loc);
}

QCPAxisRect* QCustomPlotCtrl::getCenterAxisRect()
{
	return centerAxisRect;
}

std::vector<double> QCustomPlotCtrl::handleMousePosOnCMap(QMouseEvent* event)
{
	std::vector<double> vec{};
	if (style != plotStyle::DensityPlotWithHisto || colorMap == nullptr) {
		return vec;
	}
	//or also use m_colorMap->keyAxis(), they are the same 
	double x = centerAxisRect->axis(QCPAxis::atBottom)->pixelToCoord(event->pos().x());
	double y = centerAxisRect->axis(QCPAxis::atLeft)->pixelToCoord(event->pos().y());
	vec.insert(vec.end(), { std::floor(x + 0.5), std::floor(y + 0.5), colorMap->data()->data(x, y) });
	return vec;
}

void QCustomPlotCtrl::setColorScaleRange(int lower, int upper)
{
	if (colorMap != nullptr) {
		auto cs = colorMap->colorScale();
		cs->setDataRange(QCPRange(lower, upper));
	}
}

