// created by Mark O. Brown
#include "stdafx.h"
#include "PictureControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <algorithm>
#include <numeric>
#include <boost/lexical_cast.hpp>
#include <qlayout.h>


PictureControl::PictureControl(bool histogramOption, Qt::TransformationMode mode)
	: histOption(histogramOption), /*QWidget(), */transformationMode(mode)
	, slider(Qt::Vertical, RangeSlider::DoubleHandles)
{
	active = true;
	if ( histOption ){
		horData.resize ( 1 );
		vertData.resize ( 1 );
		updatePlotData ( );
	}
	repaint ();
}

void PictureControl::updatePlotData ( ){
	if ( !histOption ){
		return;
	}
	horData[ 0 ].resize ( mostRecentImage_m.getCols ( ) );
	vertData[ 0 ].resize ( mostRecentImage_m.getRows ( ) );
	unsigned count = 0;

	std::vector<long> dataRow;
	for ( auto& data : horData[ 0 ] ){
		data.x = count;
		// integrate the column
		double p = 0.0;
		for ( auto row : range ( mostRecentImage_m.getRows ( ) ) ){
			p += mostRecentImage_m ( row, count );
		}
		count++;
		dataRow.push_back ( p );
	}
	count = 0;
	auto avg = std::accumulate ( dataRow.begin ( ), dataRow.end ( ), 0.0 ) / dataRow.size ( );
	for ( auto& data : horData[ 0 ] ){
		data.y = dataRow[ count++ ] - avg;
	}
	count = 0;
	std::vector<long> dataCol;
	for ( auto& data : vertData[ 0 ] ){
		data.x = count;
		// integrate the row
		double p = 0.0;
		for ( auto col : range ( mostRecentImage_m.getCols ( ) ) ){
			p += mostRecentImage_m ( count, col );
		}
		count++;
		dataCol.push_back ( p );
	}
	count = 0;
	auto avgCol = std::accumulate ( dataCol.begin ( ), dataCol.end ( ), 0.0 ) / dataCol.size ( );
	for ( auto& data : vertData[ 0 ] ){
		data.y = dataCol[ count++ ] - avgCol;
	}
}

/*
* initialize all controls associated with single picture.
*/
void PictureControl::initialize(std::string name, int width, int height, IChimeraQtWindow* parent, int picScaleFactorIn){
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	picScaleFactor = picScaleFactorIn;
	if ( width < 100 ){
		thrower ( "Pictures must be greater than 100 in width because this is the size of the max/min"
									 "controls." );
	}
	if ( height < 100 ){
		thrower ( "Pictures must be greater than 100 in height because this is the minimum height "
									 "of the max/min controls." );
	}
	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	coordinatesText = new QLabel("Coordinates: ", this);
	coordinatesDisp = new QLabel("", this);
	valueText = new QLabel("; Value: ", this);
	valueDisp = new QLabel("", this);
	layout1->addWidget(new QLabel(qstr(name),this));
	layout1->addWidget(coordinatesText);
	layout1->addWidget(coordinatesDisp);
	layout1->addWidget(valueText);
	layout1->addWidget(valueDisp);
	layout1->addStretch();

	maxWidth = width;
	maxHeight = height;
	QHBoxLayout* layout2 = new QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	pic.setStyle(plotStyle::DensityPlotWithHisto);
	pic.init(parent,"");
	pic.plot->setMinimumSize(400, 350);
	pictureObject = new ImageLabel (parent);	
	//connect (pictureObject, &ImageLabel::mouseReleased, [this](QMouseEvent* event) {handleMouse (event); });
	std::vector<unsigned char> data (20000);
	for (auto& pt : data){
		pt = rand () % 255;
	}
	slider.setRange(0, 4096);
	slider.setMaximumWidth(80);
	slider.setMaxLength(800);
	parent->connect (&slider, &RangeSliderIntg::smlValueChanged, [this]() {
		pic.setColorScaleRange(slider.getSilderSmlVal(), slider.getSilderLrgVal());
		pic.resetChart(); });
	parent->connect (&slider, &RangeSliderIntg::lrgValueChanged, [this]() {
		pic.setColorScaleRange(slider.getSilderSmlVal(), slider.getSilderLrgVal());
		pic.resetChart(); });
	layout2->addWidget(pic.plot, 1);
	layout2->addWidget(&slider, 0);

	connect(pic.plot, &QCustomPlot::mouseMove, [this](QMouseEvent* event) {
		handleMouse(event); });

	layout->addLayout(layout1);
	layout->addLayout(layout2);
	layout->addStretch();
}



bool PictureControl::isActive(){
	return active;
}


void PictureControl::setSliderPositions(unsigned min, unsigned max){
	slider.upperSpinBox()->setValue(max);
	slider.lowerSpinBox()->setValue(min);
	//sliderMin.setValue ( min );
	//sliderMax.setValue ( max );
}

void PictureControl::setSliderSize(int size)
{
	slider.setMaxLength(size);
}

void PictureControl::setSliderRange(unsigned min, unsigned max)
{
	slider.setRange(min, max);
}

/*
 * Used during initialization & when used when transitioning between 1 and >1 pictures per repetition. 
 * Sets the unscaled background area and the scaled area.
 */
void PictureControl::setPictureArea( QPoint loc, int width, int height ){
	//// this is important for the control to know where it should draw controls.
	//auto& sBA = scaledBackgroundArea;
	//auto& px = loc.rx (), & py = loc.ry ();
	//unscaledBackgroundArea = { px, py, px + width, py + height };
	//// reserve some area for the texts.
	//unscaledBackgroundArea.setRight (unscaledBackgroundArea.right () - 100);
	//sBA = unscaledBackgroundArea;
	///*
	//sBA.left *= width;
	//sBA.right *= width;
	//sBA.top *= height;
	//sBA.bottom *= height;*/
	//if ( horGraph ){
	//	//horGraph->setControlLocation ( { scaledBackgroundArea.left, scaledBackgroundArea.bottom }, 
	//	//							   scaledBackgroundArea.right - scaledBackgroundArea.left, 65 );
	//}
	//if ( vertGraph ){
	//	//vertGraph->setControlLocation ( { scaledBackgroundArea.left - 65, scaledBackgroundArea.bottom },
	//	//							      65, scaledBackgroundArea.bottom - scaledBackgroundArea.top );
	//}
	//double widthPicScale;
	//double heightPicScale;
	//auto& uIP = unofficialImageParameters;
	//double w_to_h_ratio = double (uIP.width ()) / uIP.height ();
	//double sba_w = sBA.right () - sBA.left ();
	//double sba_h = sBA.bottom () - sBA.top ();
	//if (w_to_h_ratio > sba_w/sba_h){
	//	widthPicScale = 1;
	//	heightPicScale = (1.0/ w_to_h_ratio) * (sba_w / sba_h);
	//}
	//else{
	//	heightPicScale = 1;
	//	widthPicScale = w_to_h_ratio / (sba_w / sba_h);
	//}

	//unsigned long picWidth = unsigned long( (sBA.right () - sBA.left())*widthPicScale );
	//unsigned long picHeight = (sBA.bottom() - sBA.top())*heightPicScale;
	//QPoint mid = { (sBA.left () + sBA.right ()) / 2, (sBA.top () + sBA.bottom ()) / 2 };
	//pictureArea.setLeft(mid.x() - picWidth / 2);
	//pictureArea.setRight(mid.x() + picWidth / 2);
	//pictureArea.setTop(mid.y() - picHeight / 2);
	//pictureArea.setBottom(mid.y() + picHeight / 2);
	//
	//if (pictureObject){
	//	pictureObject->setGeometry (px, py, width, height);
	//	pictureObject->raise ();
	//}
}


/* used when transitioning between single and multiple pictures. It sets it based on the background size, so make 
 * sure to change the background size before using this.
 * ********/
void PictureControl::setSliderControlLocs (QPoint pos, int height){
	//sliderMin.reposition ( pos, height);
	//pos.rx() += 25;
	//sliderMax.reposition ( pos, height );
}

void PictureControl::calculateSoftwareAccumulation(const Matrix<long>& picData)
{
	if (saOption.accumNum == 1) {
		accumPicResult = picData; // this is calling assignment operator T& operator=(const T& t), which return a ref
		return;
	}
	// doing software accumulation
	if (accumPicDatas.size() == 0) {
		accumPicDatas.push_back(picData);
		accumPicResult = Matrix<long>(picData); // this is calling copy-ctor, accumPicResult=picData is calling assignment operator T& operator=(const T& t), which return a ref
		accumNum++;
	}
	else {
		if (saOption.accumAll) {
			for (auto idx : range(accumPicResult.data.size())) {
				accumPicResult.data[idx] += picData.data[idx];
			}
			accumNum++;
		}
		else {
			for (auto idx : range(accumPicResult.size())) {
				accumPicResult.data[idx] = accumPicResult.data[idx] + picData.data[idx]
					- (accumPicDatas.size() == saOption.accumNum ? accumPicDatas.front().data[idx] : 0);
			}
			if (accumPicDatas.size() == saOption.accumNum) {
				accumPicDatas.pop_front();
				accumNum--;
			}
			accumPicDatas.push_back(picData);
			accumNum++;
		}
	}

}

/* used when transitioning between single and multiple pictures. It sets it based on the background size, so make
* sure to change the background size before using this.
* ********/

/*
 * change the colormap used for a given picture.
 */
void PictureControl::updatePalette( QVector<QRgb> palette ){
	imagePalette = palette;
}


/*
 * called when the user changes either the min or max edit.
 */
void PictureControl::handleEditChange( int id ){
	//if ( id == sliderMax.getEditId() ){
	//	sliderMax.handleEdit ( );
	//}
	//if ( id == sliderMin.getEditId() ){
	//	sliderMin.handleEdit ( );
	//}
}


std::pair<unsigned, unsigned> PictureControl::getSliderLocations(){
	return { slider.getSilderSmlVal(), slider.getSilderLrgVal() };
}





/*
 * Recalculate the grid of pixels, which needs to be done e.g. when changing number of pictures or re-sizing the 
 * picture. Does not draw the grid.
 */
void PictureControl::recalculateGrid(imageParameters newParameters){
	// not strictly necessary.
	grid.clear();
	// find the maximum dimension.
	unofficialImageParameters = newParameters;
	double widthPicScale;
	double heightPicScale;
	/*if (unofficialImageParameters.width ()> unofficialImageParameters.height())
	{
		widthPicScale = 1;
		heightPicScale = double(unofficialImageParameters.height()) / unofficialImageParameters.width();
	}
	else
	{
		heightPicScale = 1;
		widthPicScale = double(unofficialImageParameters.width()) / unofficialImageParameters.height();
	}*/
	auto& uIP = unofficialImageParameters;
	double w_to_h_ratio = double (uIP.widthBinned ()) / uIP.heightBinned ();
	auto& sBA = scaledBackgroundArea;
	double sba_w = sBA.right () - sBA.left ();
	double sba_h = sBA.bottom () - sBA.top ();
	if (w_to_h_ratio > sba_w / sba_h){
		widthPicScale = 1;
		heightPicScale = (1.0 / w_to_h_ratio) * (sba_w / sba_h);
	}
	else{
		heightPicScale = 1;
		widthPicScale = w_to_h_ratio / (sba_w / sba_h);
	}

	long width = long((scaledBackgroundArea.right () - scaledBackgroundArea.left ())*widthPicScale);
	long height = long((scaledBackgroundArea.bottom () - scaledBackgroundArea.top ())*heightPicScale);
	QPoint mid = { (scaledBackgroundArea.left () + scaledBackgroundArea.right ()) / 2,
				  (scaledBackgroundArea.top () + scaledBackgroundArea.bottom ()) / 2 };
	pictureArea.setLeft (mid.x () - width / 2);
	pictureArea.setRight (mid.x () + width / 2);
	pictureArea.setTop (mid.y () - height / 2);
	pictureArea.setBottom (mid.y () + height / 2);

	grid.resize(newParameters.widthBinned());
	for (unsigned colInc = 0; colInc < grid.size(); colInc++){
		grid[colInc].resize(newParameters.heightBinned());
		for (unsigned rowInc = 0; rowInc < grid[colInc].size(); rowInc++){
			// for all 4 pictures...
			grid[colInc][rowInc].setLeft(int(pictureArea.left()
											 + (double)(colInc+1) * (pictureArea.right () - pictureArea.left ())
											 / (double)grid.size( ) + 2));
			grid[colInc][rowInc].setRight(int(pictureArea.left()
				+ (double)(colInc + 2) * (pictureArea.right () - pictureArea.left ()) / (double)grid.size() + 2));
			grid[colInc][rowInc].setTop(int(pictureArea.top ()
				+ (double)(rowInc)* (pictureArea.bottom () - pictureArea.top ()) / (double)grid[colInc].size()));
			grid[colInc][rowInc].setBottom(int(pictureArea.top()
				+ (double)(rowInc + 1)* (pictureArea.bottom () - pictureArea.top ()) / (double)grid[colInc].size()));
		}
	}
}

/* 
 * sets the state of the picture and changes visibility of controls depending on that state.
 */
void PictureControl::setActive( bool activeState )
{
	if (!coordinatesText || !coordinatesDisp)	{
		return;
	}
	active = activeState;
	if (!active){
		this->hide();
		slider.hide();
		coordinatesText->hide();
		coordinatesDisp->hide();
		valueText->hide();
		valueDisp->hide();
	}
	else{
		this->show();
		slider.show();
		coordinatesText->show();
		coordinatesDisp->show();
		valueText->show();
		valueDisp->show();
	}
}

/*
 * redraws the background and image. 
 */
void PictureControl::redrawImage(){
	if ( active && mostRecentImage_m.size ( ) != 0 ){
		drawBitmap (mostRecentImage_m, mostRecentAutoscaleInfo, mostRecentSpecialMinSetting,
			mostRecentSpecialMaxSetting, mostRecentGrids, mostRecentPicNum, true);
	}
}

void PictureControl::resetStorage(){
	mostRecentImage_m = Matrix<long>(0,0);
}

void PictureControl::setSoftwareAccumulationOption ( softwareAccumulationOption opt ){
	saOption = opt;
	accumPicDatas.clear ( );
	accumPicResult.data.clear();
	accumNum = 0;
}

/* 
  Version of this from the Basler camera control Code. I will consolidate these shortly.
*/
void PictureControl::drawBitmap ( const Matrix<long>& picData, std::tuple<bool, int, int> autoScaleInfo, 
								  bool specialMin, bool specialMax, std::vector<atomGrid> grids, unsigned pictureNumber,
								  bool includingAnalysisMarkers ){
	mostRecentImage_m = picData;
	mostRecentPicNum = pictureNumber;
	mostRecentGrids = grids;

	auto minColor = slider.getSilderSmlVal( );
	auto maxColor = slider.getSilderLrgVal( );
	mostRecentAutoscaleInfo = autoScaleInfo;
	int pixelsAreaWidth = pictureArea.right () - pictureArea.left () + 1;
	int pixelsAreaHeight = pictureArea.bottom () - pictureArea.top () + 1;
	int dataWidth = grid.size ( );
	// first element containst whether autoscaling or not.
	//long colorRange;
	//if ( std::get<0> ( autoScaleInfo ) ){
	//	// third element contains max, second contains min.
	//	colorRange = std::get<2> ( autoScaleInfo ) - std::get<1> ( autoScaleInfo );
	//	minColor = std::get<1> ( autoScaleInfo );
	//}
	//else{
	//	colorRange = maxColor - minColor;
	//	minColor = minColor;
	//	//colorRange = sliderMax.getValue ( ) - sliderMin.getValue ( );
	//	//minColor = sliderMin.getValue ( );
	//}
	// assumes non-zero size...
	if ( grid.size ( ) == 0 ){
		thrower  ( "Tried to draw bitmap without setting grid size!" );
	}
	int dataHeight = grid[ 0 ].size ( );
	int totalGridSize = dataWidth * dataHeight;
	if ( picData.size ( ) != totalGridSize ){
		thrower  ( "Picture data didn't match grid size!" );
	}
	
	calculateSoftwareAccumulation(picData);

	int width = accumPicResult.getCols();
	int height = accumPicResult.getRows();
	std::vector<plotDataVec> ddvec(height);
	for (size_t idx = 0; idx < height; idx++)
	{
		ddvec[idx].reserve(width);
		for (size_t idd = 0; idd < width; idd++)
		{
			ddvec[idx].push_back(dataPoint{ 0,double(accumPicResult(idx,idd)),0 }); // x,y,err
		}
	}
	pic.setData(ddvec);

	//float yscale = ( 256.0f ) / (float) colorRange;
	//std::vector<uchar> dataArray2 ( dataWidth * dataHeight, 255 );
	//int iTemp;
	//double dTemp = 1;
	//const int picPaletteSize = 256;
	//for (int heightInc = 0; heightInc < dataHeight; heightInc++){
	//	for (int widthInc = 0; widthInc < dataWidth; widthInc++){
	//		dTemp = ceil (yscale * double(picData (heightInc, widthInc) - minColor));
	//		if (dTemp <= 0)	{
	//			// raise value to zero which is the floor of values this parameter can take.
	//			iTemp = 1;
	//		}
	//		else if (dTemp >= picPaletteSize - 1)	{
	//			// round to maximum value.
	//			iTemp = picPaletteSize - 2;
	//		}
	//		else{
	//			// no rounding or flooring to min or max needed.
	//			iTemp = (int)dTemp;
	//		}
	//		// store the value.
	//		dataArray2[widthInc + heightInc * dataWidth] = (unsigned char)iTemp;
	//	}
	//}
	//int sf = picScaleFactor;
	//QImage img (sf * dataWidth, sf * dataHeight, QImage::Format_Indexed8);
	//img.setColorTable (imagePalette);
	//img.fill (0);
	//for (auto rowInc : range(dataHeight)){
	//	std::vector<uchar> singleRow (sf * dataWidth);
	//	for (auto val : range (dataWidth)){
	//		for (auto rep : range (sf)) {
	//			singleRow[sf * val + rep] = dataArray2[rowInc * dataWidth + val];
	//		}
	//	}
	//	for (auto repRow : range (sf)){
	//		memcpy (img.scanLine (rowInc * sf + repRow), singleRow.data(), img.bytesPerLine ());
	//	}
	//}
	// need to convert to an rgb format in order to draw on top. drawing on top using qpainter isn't supported with the 
	// indexed format. 
	//img = img.convertToFormat (QImage::Format_RGB888);
	//QPainter painter;
	//painter.begin (&img);
	//drawDongles (painter, grids, pictureNumber, includingAnalysisMarkers);
	//painter.end ();	
	//// seems like this doesn't *quite* work for some reason, hence the extra number here to adjust
	//if (img.width () / img.height () > (pictureObject->width () / pictureObject->height ())-0.1)	{
	//	pictureObject->setPixmap (QPixmap::fromImage (img).scaledToWidth (pictureObject->width (), transformationMode));
	//}
	//else {
	//	pictureObject->setPixmap (QPixmap::fromImage (img).scaledToHeight (pictureObject->height (), transformationMode));
	//}
	// //update this with the new picture.
	//setHoverValue ( );
}

void PictureControl::setHoverValue( ){
	int loc = (grid.size( ) - 1 - selectedLocation.column) * grid.size( ) + selectedLocation.row;
	if ( loc >= mostRecentImage_m.size( ) )	{
		return;
	}
	valueDisp->setText( cstr( mostRecentImage_m.data[loc] ) );
}

void PictureControl::handleMouse (QMouseEvent* event){
	auto vec = pic.handleMousePosOnCMap(event);
	coordinatesDisp->setText("( " + qstr(int(vec[0])) + " , " + qstr(int(vec[1])) + " )");
	valueDisp->setText(qstr(int(vec[2])));
}

/* 
 * draw the grid which outlines where each pixel is.  Especially needs to be done when selecting pixels and no picture
 * is displayed. 
 */
void PictureControl::drawGrid(QPainter& painter){
	if (!active){
		return;
	}
	if (grid.size() != 0){
		// hard set to 5000. Could easily change this to be able to see finer grids. Tested before and 5000 seems 
		// reasonable.
		if (grid.size() * grid.front().size() > 5000){
			return;
		}
	}
	// draw rectangles indicating where the pixels are.
	for (unsigned columnInc = 0; columnInc < grid.size(); columnInc++){
		for (unsigned rowInc = 0; rowInc < grid[columnInc].size(); rowInc++){
			unsigned pixelRow = picScaleFactor * grid[columnInc][rowInc].top();
			unsigned pixelColumn = picScaleFactor * grid[columnInc][rowInc].left();
			QRect rect = QRect (QPoint (pixelColumn, pixelRow),
						 QPoint (pixelColumn + picScaleFactor - 2, pixelRow + picScaleFactor - 2));
			painter.drawRect (rect);
		}
	}
}

/*
 * draws the circle which denotes the selected pixel that the user wants to know the counts for. 
 */
void PictureControl::drawCircle(coordinate selectedLocation, QPainter& painter){
	if (grid.size() == 0){
		// this hasn't been set yet, presumably this got called by the camera window as the camera window
		// was drawing itself before the control was initialized.
		return;
	}
	if (!active){
		// don't draw anything if the window isn't active.
		return;
	}
	QRect smallRect( selectedLocation.column * picScaleFactor, selectedLocation.row * picScaleFactor, 
					 picScaleFactor-1, picScaleFactor-1 );
	painter.drawEllipse (smallRect);
}

void PictureControl::drawPicNum( unsigned picNum, QPainter& painter ){
	QFont font = painter.font ();
	// I think this is the font height in pixels on the pixmap basically. 
	font.setPointSize (20);
	painter.setFont (font);
	painter.setPen (Qt::white);
	painter.drawText (QPoint ( int(picScaleFactor)/5, picScaleFactor), cstr (picNum));
}

void PictureControl::drawAnalysisMarkers(atomGrid gridInfo){
	analysisMarkers.clear();
	if ( !active ){
		return;
	}
	QPen pen(Qt::yellow);
	unsigned gridCount = 0;

	if (coordinate(gridInfo.gridOrigin) == coordinate(0, 0) && !gridInfo.useFile) {
		// atom grid is empty, not to be used.
		unsigned count = 1;
	}
	else {
		if (!gridInfo.useFile) {
			// use the atom grid.
			unsigned count = 1;
			for (auto columnInc : range(gridInfo.width)) {
				for (auto rowInc : range(gridInfo.height)) {
					analysisMarkers.push_back(new QCPItemRect(pic.plot));
					auto* rect = analysisMarkers.back();
					rect->setPen(pen);
					rect->setClipAxisRect(pic.getCenterAxisRect());
					unsigned pixelRow = (gridInfo.gridOrigin.row + rowInc * gridInfo.pixelSpacingY);
					unsigned pixelColumn = (gridInfo.gridOrigin.column + columnInc * gridInfo.pixelSpacingX);
					rect->topLeft->setCoords(pixelColumn - 0.5 - gridInfo.includedPixelX, pixelRow + 0.5 + gridInfo.includedPixelY);
					rect->bottomRight->setCoords(pixelColumn + 0.5 + gridInfo.includedPixelX, pixelRow - 0.5 - gridInfo.includedPixelY);
				}
			}
		}
		else {
			// use the atom grid from file.
			unsigned count = 1;
			if (gridInfo.atomLocs.empty()) {
				thrower("Atoms locations for file: " + gridInfo.fileName + " is empty, seems like it is not loaded. This should not happen. A low level bug.");
			}
			for (auto coords : gridInfo.atomLocs) {
				for (auto rc : coords) {
					analysisMarkers.push_back(new QCPItemRect(pic.plot));
					auto* rect = analysisMarkers.back();
					rect->setBrush(QBrush("Yellow"));
					rect->setPen(pen);
					rect->setClipAxisRect(pic.getCenterAxisRect());
					rect->topLeft->setCoords(rc.column - 0.5, rc.row + 0.5);
					rect->bottomRight->setCoords(rc.column + 0.5, rc.row - 0.5);
				}
			}
		}

		pic.resetChart();
	}
	gridCount++;
	
}

void PictureControl::removeAnalysisMarkers()
{
	if (!active) {
		return;
	}
	for (auto* marker : analysisMarkers) {
		pic.plot->removeItem(marker);
		marker = nullptr;
	}
	analysisMarkers.clear();
	pic.resetChart();
}

void PictureControl::drawDongles (QPainter& painter, std::vector<atomGrid> grids, unsigned pictureNumber, 
	bool includingAnalysisMarkers){
	//drawPicNum (pictureNumber, painter);
	if (includingAnalysisMarkers) {
		//drawAnalysisMarkers (  grids );
	}
	//painter.setPen (Qt::red);
	//drawCircle (selectedLocation, painter);
}
 
 
void PictureControl::setTransformationMode (Qt::TransformationMode mode) {
	transformationMode = mode;
}


