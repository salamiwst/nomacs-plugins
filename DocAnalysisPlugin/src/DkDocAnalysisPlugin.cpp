/*******************************************************************************************************
 DkDocAnalysisPlugin.cpp
 Created on:	14.07.2013

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2011-2013 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2013 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2013 Florian Kleber <florian@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#include "DkDocAnalysisPlugin.h"
#include "DkViewPort.h"
#include "DkMetaData.h"
#include "DkCentralWidget.h"
#include "DkImageLoader.h"

#include <QFileDialog>
#include <QImageWriter>
#include <QVector2D>
#include <QMouseEvent>

namespace nmp {

/*-----------------------------------DkDocAnalysisPlugin ---------------------------------------------*/

/**
*	Constructor
**/
DkDocAnalysisPlugin::DkDocAnalysisPlugin() {

	viewport = 0;
	jpgDialog = 0;	
	tifDialog = 0;
}

/**
*	Destructor
**/
DkDocAnalysisPlugin::~DkDocAnalysisPlugin() {

	qDebug() << "[DOCUMENT ANALYSIS PLUGIN] deleted...";

	if (viewport) {
		viewport->deleteLater();
		viewport = 0;
	}
	jpgDialog = 0;
	tifDialog = 0;
}

/**
* Returns unique ID for the generated dll
**/
QString DkDocAnalysisPlugin::id() const {
	return PLUGIN_ID;
};

/**
* Returns descriptive image
**/
QImage DkDocAnalysisPlugin::image() const {

   return QImage(":/nomacsPluginDocAnalysis/img/description.png");
};

/**
* Main function: runs plugin based on its ID
* @param runID runID of the plugin 
* @param image current image in the Nomacs viewport
**/
QSharedPointer<nmc::DkImageContainer> DkDocAnalysisPlugin::runPlugin(const QString &runID, QSharedPointer<nmc::DkImageContainer> image) const  {
	
	//for a viewport plugin runID and image are null
	if (viewport && image) {

		DkDocAnalysisViewPort* docAnalysisViewport = dynamic_cast<DkDocAnalysisViewPort*>(viewport);

		if (!docAnalysisViewport->isCanceled()) 
			image->setImage(docAnalysisViewport->getPaintedImage(), tr("Pages Annotated"));

		viewport->setVisible(false);

		return image;
	}
	
	return image;
};



/**
* returns paintViewPort
**/
nmc::DkPluginViewPort* DkDocAnalysisPlugin::getViewPort() {

	if (!viewport) {
		DkDocAnalysisViewPort* vp = new DkDocAnalysisViewPort();
		vp->setMainWindow(getMainWindow());

		viewport = vp;

		// signal for saving magic cut
		connect(viewport, SIGNAL(saveMagicCutRequest(QImage, int, int, int, int)), this, SLOT(saveMagicCut(QImage, int, int, int, int)));
		connect(this, SIGNAL(magicCutSavedSignal(bool)), viewport, SLOT(magicCutSaved(bool)));
	}
	return viewport;
}

void DkDocAnalysisPlugin::deleteViewPort() {

	if (viewport) {
		viewport->deleteLater();
		viewport = 0;
	}
}

/**
* function for saving a magic cut.
* Saves the image using it's original name and appending the x- and y-Coordinates to the name.
* Additional metadata to where the cut is located in the original image
* is written in the Comment string of the image.
* @param saveImage The image to be saved
* @param xCoord The x-coordinate of the upper left corner of the bounding box of the cut
* @param yCoord The y-coordinate of the upper left corner of the bounding box of the but
* @param height The height of the bounding box (y-extent)
* @param width The width of the bounding box (x-extent)
* \sa magicCutSavedSignal(bool) 
**/
void DkDocAnalysisPlugin::saveMagicCut(QImage saveImage, int xCoord, int yCoord, int height, int width/* QString saveNameAppendix*/) {
	qDebug() << "saving...";

	QSharedPointer<nmc::DkImageLoader> loader;
	nmc::DkNoMacs* nmcWin;
	QMainWindow* win = getMainWindow();
	if (win) {

		// this should usually work - since we are a nomacs plugin
		nmcWin = dynamic_cast<nmc::DkNoMacs*>(win);

		if (nmcWin) {
			nmc::DkCentralWidget* cw = nmcWin->getTabWidget();

			if (cw) {
				loader = cw->getCurrentImageLoader();
			}

		}
	}
	
	QString selectedFilter;
	QString saveName;
	QFileInfo saveFile;

	QString saveNameAppendix = QString("_%1").arg(xCoord);
	saveNameAppendix.append(QString("_%1").arg(yCoord));

	if (loader && loader->hasFile()) {
		saveFile = loader->filePath();
		saveName = saveFile.fileName();

		qDebug() << "saveName: " << saveName; //.toStdString();
		
		if (loader->getSavePath() != saveFile.absolutePath())
			saveFile = QFileInfo(loader->getSavePath(), saveName);

		int filterIdx = -1;

		QStringList sF = nmc::Settings::param().app().saveFilters;
		//qDebug() << sF;

		QRegExp exp = QRegExp("*." + saveFile.suffix() + "*", Qt::CaseInsensitive);
		exp.setPatternSyntax(QRegExp::Wildcard);

		for (int idx = 0; idx < sF.size(); idx++) {

			//qDebug() << exp;
			//qDebug() << saveFile.suffix();
			//qDebug() << sF.at(idx);

			if (exp.exactMatch(sF.at(idx))) {
				selectedFilter = sF.at(idx);
				filterIdx = idx;
				break;
			}
		}

		if (filterIdx == -1)
			saveName.remove("." + saveFile.suffix());
	}

	// note: basename removes the whole file name from the first dot...
	QString savePath = (!selectedFilter.isEmpty()) ? saveFile.absoluteFilePath() : QFileInfo(saveFile.absoluteDir(), saveName).absoluteFilePath();

	savePath.insert(savePath.length()-saveFile.completeSuffix().length()-1, saveNameAppendix);

	QString fileName = QFileDialog::getSaveFileName(viewport, tr("Save Magic Cut"),
		savePath, nmc::Settings::param().app().saveFilters.join(";;"), &selectedFilter);

	//qDebug() << "selected Filter: " << selectedFilter;

	if (fileName.isEmpty())
		return;

	QString ext = QFileInfo(fileName).suffix();

	if (!ext.isEmpty() && !selectedFilter.contains(ext)) {

		QStringList sF = nmc::Settings::param().app().saveFilters;

		for (int idx = 0; idx < sF.size(); idx++) {

			if (sF.at(idx).contains(ext)) {
				selectedFilter = sF.at(idx);
				break;
			}
		}
	}

	// TODO: if more than one file is opened -> open new threads
	QFileInfo sFile = QFileInfo(fileName);
	int compression = -1;	// default value

	//if (saveDialog->selectedNameFilter().contains("jpg")) {
	if (selectedFilter.contains(QRegExp("(jpg|jpeg|j2k|jp2|jpf|jpx)", Qt::CaseInsensitive))) {

		if (!jpgDialog)
			jpgDialog = new nmc::DkCompressDialog(nmcWin);

		if (selectedFilter.contains(QRegExp("(j2k|jp2|jpf|jpx)")))
			jpgDialog->setDialogMode(nmc::DkCompressDialog::j2k_dialog);
		else
			jpgDialog->setDialogMode(nmc::DkCompressDialog::jpg_dialog);

		jpgDialog->imageHasAlpha(saveImage.hasAlphaChannel());
		//jpgDialog->show();
		if (!jpgDialog->exec())
			return;

		compression = jpgDialog->getCompression();


		if (saveImage.hasAlphaChannel()) {

			QRect imgRect = QRect(QPoint(), saveImage.size());
			QImage tmpImg = QImage(saveImage.size(), QImage::Format_RGB32);
			QPainter painter(&tmpImg);
			painter.fillRect(imgRect, jpgDialog->getBackgroundColor());
			painter.drawImage(imgRect, saveImage, imgRect);

			saveImage = tmpImg;
		}

	//	qDebug() << "returned: " << ret;
	}

	if (selectedFilter.contains("webp")) {

		if (!jpgDialog)
			jpgDialog = new nmc::DkCompressDialog(nmcWin);

		jpgDialog->setDialogMode(nmc::DkCompressDialog::webp_dialog);
		jpgDialog->setImage(saveImage);

		if (!jpgDialog->exec())
			return;

		compression = jpgDialog->getCompression();
	}

	//if (saveDialog->selectedNameFilter().contains("tif")) {
	if (selectedFilter.contains("tif")) {
		
		if (!tifDialog)
			tifDialog = 0; // TODO: fix new DkTifDialog(nmcWin);

		if (!tifDialog->exec())
			return;

		compression = tifDialog->getCompression();
	}

	QImageWriter* imgWriter = new QImageWriter(sFile.absoluteFilePath());
	imgWriter->setCompression(compression);
	imgWriter->setQuality(compression);
	
	if (imgWriter->supportsOption(QImageIOHandler::Description)) {
		QString comment = QString(saveName);
		comment.append(QString("; %1").arg(xCoord));
		comment.append(QString("; %1").arg(yCoord));
		comment.append(QString("; %1").arg(height));
		comment.append(QString("; %1").arg(width));

		imgWriter->setText("Comment", comment);
	}
	
	bool saved = imgWriter->write(saveImage);
	//imgWriter->setFileName(QFileInfo().absoluteFilePath());
	delete imgWriter;


	emit magicCutSavedSignal(saved);
	//bool saved = sImg.save(filePath, 0, compression);
	//qDebug() << "jpg compression: " << compression;
}

/*-----------------------------------DkDocAnalysisViewPort ---------------------------------------------*/

DkDocAnalysisViewPort::DkDocAnalysisViewPort(QWidget* parent, Qt::WindowFlags flags) : DkPluginViewPort(parent, flags) {
	setFocusPolicy(Qt::StrongFocus);
	setMouseTracking(true);
	init();
}

DkDocAnalysisViewPort::~DkDocAnalysisViewPort() {
	qDebug() << "[DOCUMENT ANALYSIS VIEWPORT] deleted...";
	
	// acitive deletion since the MainWindow takes ownership...
	// if we have issues with this, we could disconnect all signals between viewport and toolbar too
	// however, then we have lot's of toolbars in memory if the user opens the plugin again and again
	if (magicCut) {
		delete magicCut;
	}
	if (lineDetection) {
		delete lineDetection;
	}
	if (lineDetectionDialog) {
		delete lineDetectionDialog;
	}
	if (docAnalysisToolbar) {
		delete docAnalysisToolbar;
		docAnalysisToolbar = 0;
	}
	setMouseTracking(false);
	setFocusPolicy(Qt::NoFocus);
}

void DkDocAnalysisViewPort::init() {
	
	cancelTriggered = false;
	isOutside = false;
	defaultCursor = Qt::ArrowCursor;
	setCursor(defaultCursor);
	
	editMode = mode_default;
	showBottomLines = false;
	showTopLines = false;
	
	docAnalysisToolbar = new DkDocAnalysisToolBar(tr("Document Analysis Toolbar"), this);

	// connect signals from toolbar to viewport
	connect(docAnalysisToolbar, SIGNAL(measureDistanceRequest(bool)), this, SLOT(pickDistancePoint(bool)));
	connect(docAnalysisToolbar, SIGNAL(pickSeedpointRequest(bool)),  this, SLOT(pickSeedpoint(bool)));
	connect(docAnalysisToolbar, SIGNAL(clearSelectionSignal()), this, SLOT(clearMagicCut()));
	connect(docAnalysisToolbar, SIGNAL(undoSelectionSignal()), this, SLOT(undoSelection()));
	connect(docAnalysisToolbar, SIGNAL(toleranceChanged(int)), this, SLOT(setMagicCutTolerance(int)));
	connect(docAnalysisToolbar, SIGNAL(openCutDialogSignal()), this, SLOT(openMagicCutDialog()));
	connect(docAnalysisToolbar, SIGNAL(detectLinesSignal()), this, SLOT(openLineDetectionDialog()));
	connect(docAnalysisToolbar, SIGNAL(showBottomTextLinesSignal(bool)), this, SLOT(showBottomTextLines(bool)));
	connect(docAnalysisToolbar, SIGNAL(showTopTextLinesSignal(bool)), this, SLOT(showTopTextLines(bool)));
	connect(docAnalysisToolbar, SIGNAL(clearSingleSelectionRequest(bool)), this, SLOT(pickResetRegionPoint(bool)));
	connect(docAnalysisToolbar, SIGNAL(cancelPlugin()), this, SLOT(cancelPlugin()));
	connect(this, SIGNAL(cancelPickSeedpointRequest()), docAnalysisToolbar, SLOT(pickSeedpointCanceled()));
	connect(this, SIGNAL(cancelDistanceMeasureRequest()), docAnalysisToolbar, SLOT(measureDistanceCanceled()));
	connect(this, SIGNAL(enableSaveCutSignal(bool)), docAnalysisToolbar, SLOT(enableButtonSaveCut(bool)));
	connect(this, SIGNAL(enableShowTextLinesSignal(bool)), docAnalysisToolbar, SLOT(enableButtonShowTextLines(bool)));
	connect(this, SIGNAL(toggleBottomTextLinesButtonSignal(bool)), docAnalysisToolbar, SLOT(toggleBottomTextLinesButton(bool)));
	connect(this, SIGNAL(toggleTopTextLinesButtonSignal(bool)), docAnalysisToolbar, SLOT(toggleTopTextLinesButton(bool)));
	connect(this, SIGNAL(startDistanceMeasureRequest()), docAnalysisToolbar, SLOT(measureDistanceStarted()));
	connect(this, SIGNAL(startPickSeedpointRequest()), docAnalysisToolbar, SLOT(pickSeedpointStarted()));
	connect(this, SIGNAL(startClearSingleRegionRequest()), docAnalysisToolbar, SLOT(clearSingleRegionStarted()));
	connect(this, SIGNAL(cancelClearSingleRegionRequest()), docAnalysisToolbar, SLOT(clearSingleRegionCanceled()));

	// the magic cut tool
	magicCut = new DkMagicCut();
	magicCutDialog = 0;
	// regular update of contours
	QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateAnimatedContours()));
    timer->start(850);
	// the distance tool
	distance = new DkDistanceMeasure();
	// the line detection tool
	lineDetection = new DkLineDetection();
	lineDetectionDialog = 0;
}


void DkDocAnalysisViewPort::mouseMoveEvent(QMouseEvent *event) {

	if (editingActive()) {
		if(parent()) {
			nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
			
			if(viewport) {
		
				if(QRectF(QPointF(), viewport->getImage().size()).contains(mapToImage(event->pos()))) {
					
					switch(editMode) {

					case mode_pickSeedpoint: {
						this->setCursor(Qt::PointingHandCursor);
						break;
					}
					case mode_cancelSeedpoint: {
						this->setCursor(Qt::PointingHandCursor);
						break;
					}
					case mode_pickDistance: {
						if(distance->hasStartPoint() && !distance->hastStartAndEndPoint()) {
							this->setCursor(Qt::BlankCursor);
						} else {
							this->setCursor(Qt::CrossCursor);
						}
						//imgPos = mWorldMatrix.inverted().map(event->pos());
						//imgPos = mImgMatrix.inverted().map(imgPos);
						QPointF imgPos;
						imgPos = mapToImage(event->pos());
						distance->setCurPoint(imgPos.toPoint());
						update();
						break;
					}
					default:
						break;
					}
					
					/*if (isOutside) {
						paths.append(QPainterPath());
						paths.last().moveTo(mapToImage(event->posF()));
						pathsPen.append(pen);
					}
					else {
						QPointF point = mapToImage(event->posF());
						paths.last().lineTo(point);
						update();
					}*/
					isOutside = false;
				}
				else isOutside = true;
			}
		}
	} else {
		// propagate mouse event (for panning)
		this->unsetCursor();
		QWidget::mouseMoveEvent(event);
	}
}

void DkDocAnalysisViewPort::mouseDoubleClickEvent(QMouseEvent *event) {

	// if any editing operation is going on then do not allow full screen
	if (editingActive() || editingDrawingActive() )
		return;
	else
		QWidget::mouseDoubleClickEvent(event);
}

void DkDocAnalysisViewPort::mouseReleaseEvent(QMouseEvent *event) {

	QPointF imgPos;
	QPoint xy;
	imgPos = mapToImage(event->pos());
	xy = imgPos.toPoint();

	if(parent()) {
		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());

		if(viewport) {
			// check if point is within image
			if(QRectF(QPointF(), viewport->getImage().size()).contains(imgPos)) {

				switch(editMode) {

				case mode_pickSeedpoint:
					//imgPos = mWorldMatrix.inverted().map(event->pos());
					//imgPos = mImgMatrix.inverted().map(imgPos);
					
					if (event->button() == Qt::LeftButton)
						if(!magicCut->magicwand(xy)) {
								QString tooLargeAreaString = QString("Selected area is too big");
								QMessageBox tooLargeAreaDialog(this);
								tooLargeAreaDialog.setWindowTitle("Area too big");
								tooLargeAreaDialog.setIcon(QMessageBox::Information);
								tooLargeAreaDialog.setText(tooLargeAreaString);
								tooLargeAreaDialog.show();
								tooLargeAreaDialog.exec();
						}
					
					update();
					// check if the save-button has to be enabled or disabled
					if(magicCut->hasContours())
						emit enableSaveCutSignal(true);
					else
						emit enableSaveCutSignal(false);

					this->setCursor(Qt::PointingHandCursor);
					break;
				case mode_cancelSeedpoint:
					
					if (event->button() == Qt::LeftButton) {
						magicCut->resetRegionMask(xy);
					}
					update();
					// check if the save-button has to be enabled or disabled
					if(magicCut->hasContours())
						emit enableSaveCutSignal(true);
					else
						emit enableSaveCutSignal(false);

					this->setCursor(Qt::PointingHandCursor);
					break;
				case mode_pickDistance: 

					if(distance->hastStartAndEndPoint()) {
						distance->resetPoints();
					}

					distance->setPoint(xy);
					update();

					/*if(distance->hastStartAndEndPoint())
					{
						QString distanceString = QString("Distance: %1 cm (%2 inch)").arg(distance->getDistanceInCm()).arg(distance->getDistanceInInch());
						QMessageBox distanceDialog(this);
						distanceDialog.setWindowTitle("Distance");
						distanceDialog.setIcon(QMessageBox::Information);
						distanceDialog.setText(distanceString);
						distanceDialog.show();
						distanceDialog.exec();

						distance->resetPoints();
						// go back to default mode
						stopEditing();
					}*/
		
					this->setCursor(Qt::CrossCursor);
					break;
				}
			}
		}
	}
}

void DkDocAnalysisViewPort::keyPressEvent(QKeyEvent* event) {

	if ((event->key() == Qt::Key_Escape) && (editMode != mode_default)) {
		
		stopEditing();	
	}
	else if (event->key() == Qt::Key_Return) {
		// use Alt + Enter for MagicCut to distinguish from ordinary cut (which is only Enter)
		if(event->modifiers() == Qt::AltModifier && !event->isAutoRepeat()) {

			openMagicCutDialog();
		}
	}
	else if (editMode == mode_pickDistance && event->key() == Qt::Key_Shift) {
		// if shift is held, perform snapping for distance measure tool
		distance->setSnapping(true);
		if(!event->isAutoRepeat()) {
			if(distance->hasStartPoint()) {
				// update immediately
				distance->setCurPoint(distance->getCurPoint());
			}
		}
		update();
	}
	else {
		//DkViewPort::keyPressEvent(event);
		// propagate event
		QWidget::keyPressEvent(event);
	}

}

void DkDocAnalysisViewPort::keyReleaseEvent(QKeyEvent *event) {
	distance->setSnapping(false);
}

/**
* Function to check if nomacs is in any of the editing modes (pick seedpoint/color/distance) 
* @returns true, if any editing function is active (!= mode_default)
* \sa editModes
**/
bool DkDocAnalysisViewPort::editingActive() {
		if(editMode == mode_default)
			return false;
		else 
			return true;
}

/**
* Check if any drawing specific to the editing operations is active
* @returns true if contour drawing or distance drawing is currently active
**/
bool DkDocAnalysisViewPort::editingDrawingActive() {

	if(magicCut)
		return magicCut->hasContours();
	if(distance)
		return distance->hasStartPoint();

	return false;
}

/**
* Stops the current editing and sends corresponding signals to the Tool Bar
* \sa cancelDistanceMeasureRequest() DkMagicCutToolBar::measureDistanceCanceled()
* \sa cancelPickSeedpointRequest() DkMagicCutToolBar::pickSeedpointCanceled()
**/
void DkDocAnalysisViewPort::stopEditing() {

	switch(editMode) {
	case mode_pickDistance:
		emit cancelDistanceMeasureRequest();
		distance->resetPoints();
		break;
	case mode_pickSeedpoint:
		emit cancelPickSeedpointRequest();
		break;
	case mode_cancelSeedpoint:
		emit cancelClearSingleRegionRequest();
		break;
	}
	editMode = mode_default;
	this->unsetCursor();
	update();
}

void DkDocAnalysisViewPort::paintEvent(QPaintEvent *event) {
	
	QPainter painter(this);
	
	if (mWorldMatrix)
		painter.setWorldTransform((*mImgMatrix) * (*mWorldMatrix));	// >DIR: using both matrices allows for correct resizing [16.10.2013 markus]

	if(parent()) {
		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
		
		// show the bottom text lines if toggled
		if (showBottomLines) {
			QImage botTextLines = lineDetection->getBottomLines();
			painter.drawImage(botTextLines.rect(), botTextLines);
		}
		if (showTopLines) {
			QImage topTextLines = lineDetection->getTopLines();
			painter.drawImage(topTextLines.rect(), topTextLines);
		}


		// draw Contours
		if (magicCut->hasContours()) {	
			drawContours(&painter);
		}

		// draw distance line
		if (editMode == mode_pickDistance) {
			drawDistanceLine(&painter);
		}
	}

	painter.end();

	DkPluginViewPort::paintEvent(event);
}

/**
* Slot - A timed function, called regularly to generate an animated effect
* for the drawn contours of the magic cut areas
* \sa DkMagicCut DkMagicCut::updateAnimateContours()
**/
void DkDocAnalysisViewPort::updateAnimatedContours() {

	magicCut->updateAnimateContours();
	update();
}

/**
* Draws the contours of selected regions (made using the magic cut tool)
* @param painter The painter to use
* \sa DkMagicCut
**/ 
void DkDocAnalysisViewPort::drawContours(QPainter *painter) {

	QPen pen = painter->pen();
	QPen contourPen = magicCut->getContourPen();
	if (avgBrightness < brightnessThreshold)
		contourPen.setColor(QColor(255,255,255));

	painter->setPen(contourPen);
	painter->drawPath(magicCut->getContourPath());
	painter->setPen(pen);
}


/**
* Draws lines and points referring to the distance measure tool
* @param painter The painter to use
* \sa DkDistanceMeasure
**/
void DkDocAnalysisViewPort::drawDistanceLine(QPainter *painter) {

	// return if no start point yet
	if(!distance->hasStartPoint()) return;

	if (distance->getCurPoint().isNull()) {
		distance->setCurPoint(distance->getStartPoint());
	}

	QPen pen = painter->pen();
	// set pen color to white
	if (avgBrightness < brightnessThreshold) {
		painter->setPen(QColor(255,255,255));
	}

	QPoint point = distance->getStartPoint();
	point = mImgMatrix->map(point);
	
	// special handling of drawing cross - to avoid zooming of cross lines
	QPointF startPointMapped = mWorldMatrix->map(point);
	QPointF crossTransP1 = startPointMapped;
	crossTransP1.setY(crossTransP1.y() + 7);
	QPointF crossTransP2 = startPointMapped;
	crossTransP2.setY(crossTransP2.y() - 7);
	painter->setWorldMatrixEnabled(false);
	painter->drawLine(crossTransP1, crossTransP2);
	painter->setWorldMatrixEnabled(true);
	crossTransP1 = startPointMapped;
	crossTransP1.setX(crossTransP1.x() + 7);
	crossTransP2 = startPointMapped;
	crossTransP2.setX(crossTransP2.x() - 7);
	painter->setWorldMatrixEnabled(false);
	painter->drawLine(crossTransP1, crossTransP2);
	painter->setWorldMatrixEnabled(true);
	
	// draw the line
	//painter->drawLine(distance->getStartPoint(), distance->getCurPoint());
	// draw the text containing the current distance

	QPoint point_end = distance->getCurPoint();
	point_end = mImgMatrix->map(point_end);

	QString dist_text = QString::number(distance->getDistanceInCm(), 'f', 2) + " cm";
	QPoint pos_text = QPoint(point_end.x(), point_end.y());

	QFont font = painter->font();
	font.setPointSizeF(12);

	painter->setFont(font);

	QFontMetricsF fm(font);
	QRectF rect = fm.boundingRect(dist_text);
	QPointF transP = mWorldMatrix->map(pos_text);

	if (point_end.x() < point.x())
		transP.setX(transP.x());
	else
		transP.setX(transP.x() - rect.width());

	if (point_end.y() > point.y()) 
		transP.setY(transP.y() + rect.height());
	else
		transP.setY(transP.y());
	
	painter->setWorldMatrixEnabled(false);
	painter->drawText(transP.x(), transP.y(), dist_text);
	painter->setWorldMatrixEnabled(true);

	point = distance->getCurPoint();
	point = mImgMatrix->map(point);

	// special handling of drawing cross - to avoid zooming of cross lines
	QPointF endPointMapped = mWorldMatrix->map(point);
	crossTransP1 = endPointMapped;
	crossTransP1.setY(crossTransP1.y() + 7);
	crossTransP2 = endPointMapped;
	crossTransP2.setY(crossTransP2.y() - 7);
	painter->setWorldMatrixEnabled(false);
	painter->drawLine(crossTransP1, crossTransP2);
	painter->setWorldMatrixEnabled(true);
	crossTransP1 = endPointMapped;
	crossTransP1.setX(crossTransP1.x() + 7);
	crossTransP2 = endPointMapped;
	crossTransP2.setX(crossTransP2.x() - 7);
	painter->setWorldMatrixEnabled(false);
	painter->drawLine(crossTransP1, crossTransP2);
	// draw the distance line
	painter->drawLine(startPointMapped, endPointMapped);
	painter->setWorldMatrixEnabled(true);


	/*if(distance->hastStartAndEndPoint()) {
		point = distance->getEndPoint();
		point = mImgMatrix.map(point);
		painter->drawLine(point.x(), point.y()+3, point.x(), point.y()-3);
		painter->drawLine(point.x()-3, point.y(), point.x()+3, point.y());
	}*/

	// reset pen color to white
	painter->setPen(pen);

}

QImage DkDocAnalysisViewPort::getPaintedImage() {

	if(parent()) {
		nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
		if (viewport) {
			QImage img = viewport->getImage();
			return img;
		}
	}
	
	return QImage();
}

void DkDocAnalysisViewPort::setMainWindow(QMainWindow* win) {
	this->win = win;
	brightnessThreshold = 0.3;

	QImage image;
	// >DIR: OK, let's get the current image metadata [21.10.2014 markus]
	// all ifs are to be 100% save : )
	// TODO: add error dialogs if we cannot retrieve metadata...
	if (win) {

		// this should usually work - since we are a nomacs plugin
		nmc::DkNoMacs* nmcWin = dynamic_cast<nmc::DkNoMacs*>(win);

		if (nmcWin) {

			nmc::DkCentralWidget* cw = nmcWin->getTabWidget();

			if (cw) {
				QSharedPointer<nmc::DkImageLoader> loader = cw->getCurrentImageLoader();
				
				if (loader) {
					QSharedPointer<nmc::DkImageContainerT> imgC = loader->getCurrentImage();
					
					if (imgC) {

						metadata = imgC->getMetaData();
						image = imgC->image();
					}
				}
			}

		}
	}

	if (metadata) {
		qDebug() << "metadata received, the image has: " << metadata->getResolution() << " dpi";
	} else {
		qDebug() << "WARNING: I could not retrieve the image metadata...";
		QMessageBox dialog(this);
		dialog.setIcon(QMessageBox::Warning);
		dialog.setText(tr("Could not retrieve image metadata"));
		dialog.show();
		dialog.exec();
	}
	distance->setMetaData(metadata);

	if (!image.isNull()) {
		cv::Mat img = nmc::DkImage::qImage2Mat(image);
		// set image for magic cut
		magicCut->setImage(img, mImgMatrix);
		// disable the save region button
		emit enableSaveCutSignal(false);
		// the line detection part
		lineDetection->setImage(img);
		if(lineDetectionDialog) {
			lineDetectionDialog->setDefaultConfiguration();
			lineDetectionDialog->setMetaData(metadata);
		}
		// disable the display text lines button
		showBottomTextLines(false);
		showTopTextLines(false);
		emit enableShowTextLinesSignal(false);

		// calculate the average brightness of the image by converting to YUV format
		cv::Mat yuvImg;
		cv::cvtColor(img, yuvImg, CV_BGR2YUV);
		std::vector<cv::Mat> ImgChannels;
		cv::split(yuvImg, ImgChannels);
		// calculate average vallue of luma component (Y channel)
		cv::Scalar sum = cv::sum(ImgChannels[0]);
		avgBrightness = (sum[0] / (ImgChannels[0].size().width * ImgChannels[0].size().height)) / 255;
	}
}

bool DkDocAnalysisViewPort::isCanceled() {
	return cancelTriggered;
}

void DkDocAnalysisViewPort::setVisible(bool visible) {
	
	if (docAnalysisToolbar)
		emit showToolbar(docAnalysisToolbar, visible);

	DkPluginViewPort::setVisible(visible);
}

/**
* Calculates the average Brigtness of a cv::Mat
**/
void DkDocAnalysisViewPort::getBrightness(const cv::Mat& frame, double& brightness)
{
	cv::Mat temp, lum; //color[3];
	std::vector<cv::Mat> color;
	temp = frame;

	split(temp, color);

	color[0] = color[0] * 0.299;
	color[1] = color[1] * 0.587;
	color[2] = color[2] * 0.114;


	lum = color[0] + color [1] + color[2];

	cv::Scalar summ = sum(lum);


	brightness = summ[0]/((pow(2,64)-1)*frame.rows * frame.cols) * 2; //-- percentage conversion factor
}


/**
* Starts/ends the distance points picking mode.
* Cancels any other active modes.
* @param pick start or end the mode
* \sa cancelPickSeedpointRequest() DkMagicCutToolBar::pickSeedpointCanceled()
**/
void DkDocAnalysisViewPort::pickDistancePoint(bool pick) {

	switch(editMode) {
	case mode_pickSeedpoint:
		emit cancelPickSeedpointRequest();
		break;
	case mode_cancelSeedpoint:
		emit cancelClearSingleRegionRequest();
		break;
	}

	if(pick) {
		editMode = mode_pickDistance;
		distance->resetPoints();
		this->setCursor(Qt::CrossCursor);
	} else
		editMode = mode_default;	
}

/**
* Starts the distance points picking mode if not yet active
* Cancels any other active modes.
* \sa cancelPickSeedpointRequest() DkMagicCutToolBar::pickSeedpointCanceled(), startDistanceMeasureRequest()
**/
void DkDocAnalysisViewPort::pickDistancePoint() {

	switch(editMode) {
	case mode_pickDistance:
		return;
	case mode_pickSeedpoint:
		emit cancelPickSeedpointRequest();
		break;
	case mode_cancelSeedpoint:
		emit cancelClearSingleRegionRequest();
		break;
	}

	editMode = mode_pickDistance;
	emit startDistanceMeasureRequest();
	distance->resetPoints();
	this->setCursor(Qt::CrossCursor);
}


/**
* Starts/ends the seed points picking mode for region selection.
* Cancels any other active modes.
* @param pick start or end the mode
* \sa cancelDistanceMeasureRequest() DkMagicCutToolBar::measureDistanceCanceled()
**/
void DkDocAnalysisViewPort::pickSeedpoint(bool pick) {

	switch(editMode) {
	case mode_pickDistance:
		emit cancelDistanceMeasureRequest();
		distance->resetPoints();
		break;
	case mode_cancelSeedpoint:
		emit cancelClearSingleRegionRequest();
		break;
	}

	if(pick) {
		editMode = mode_pickSeedpoint;
		this->setCursor(Qt::PointingHandCursor);
	} else
		editMode = mode_default;	
}

/**
* Starts/ends the seed points picking mode for region selection.
* Cancels any other active modes.
* @param pick start or end the mode
* \sa cancelDistanceMeasureRequest() cancelPickSeedpointRequest DkMagicCutToolBar::measureDistanceCanceled()
**/
void DkDocAnalysisViewPort::pickResetRegionPoint(bool pick) {

	switch(editMode) {
	case mode_pickDistance:
		emit cancelDistanceMeasureRequest();
		distance->resetPoints();
		break;
	case mode_pickSeedpoint:
		emit cancelPickSeedpointRequest();
		break;
	}

	if(pick) {
		editMode = mode_cancelSeedpoint;
		this->setCursor(Qt::PointingHandCursor);
	} else
		editMode = mode_default;	
}

/**
* Opens the dialog to save the magic cut, if anything is selected.
* \sa DkMagicCutDialog DkMagicCut
**/
void DkDocAnalysisViewPort::openMagicCutDialog() {

	nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
	if (viewport->getImage().isNull()) return;

	// make sure that at least one region is outlined
	if (!magicCut->hasContours()) {
		/*QString msg = tr("Please select at least one region before saving.\n");
		QMessageBox errorDialog(this);
		errorDialog.setWindowTitle("Information");
		errorDialog.setIcon(QMessageBox::Information);
		errorDialog.setText(msg);
		errorDialog.show();
	    errorDialog.exec();*/
		return;
	}

	if (!magicCutDialog) {
		magicCutDialog = new DkMagicCutDialog(magicCut, this, 0);
		connect(magicCutDialog, SIGNAL(savePressed(QImage, int, int, int, int)), this, SLOT(saveMagicCutPressed(QImage, int, int, int, int)));
	}

	bool done = magicCutDialog->exec();

}

/**
* Sets the changed magic cut tolerance in the tool to the new value.
* @param tol The new color tolerance value
* \sa DkMagicCut::setTolerance(int)
**/
void DkDocAnalysisViewPort::setMagicCutTolerance(int tol) {

	magicCut->setTolerance(tol);
}

void DkDocAnalysisViewPort::undoSelection() {
	bool hasmore = magicCut->undoSelection();
	if (!hasmore) {
		emit enableSaveCutSignal(false);
	}
}

/**
* Clears all selected regions and resets the region mask in the magic cut tool.
* \sa DkMagicCut::resetRegionMask()
**/
void DkDocAnalysisViewPort::clearMagicCut() {

	magicCut->resetRegionMask();
	emit enableSaveCutSignal(false);
}

/**
* Emits a signal that the magic cut should be saved.
* \a saveMagicCutRequest(QImage, int, int, int, int) DkNoMacs::saveMagicCut(QImage, int, int, int, int)
**/
void DkDocAnalysisViewPort::saveMagicCutPressed(QImage saveImg, int xCoord, int yCoord, int height, int width) {

	//std::cout << "SAVE MAGIC CUT: " << nameAppendix.toStdString() << std::endl;
	emit saveMagicCutRequest(saveImg, xCoord, yCoord, height, width);
}

/**
* Called after the magic cut has been saved, resets masks, displays error message when needed.
* @param saved true if successfully saved, false otherwise
* \sa DkMagicCut::resetRegionMask()
**/
void DkDocAnalysisViewPort::magicCutSaved(bool saved) {

	if(saved) {
		magicCut->resetRegionMask();
		emit enableSaveCutSignal(false);
	}
	else {

		QString msg = tr("Sorry, the magic cut could not be saved\n");
		QMessageBox errorDialog(this);
		errorDialog.setWindowTitle("Error saving magic cut");
		errorDialog.setIcon(QMessageBox::Critical);
		errorDialog.setText(msg);
		errorDialog.show();
	    errorDialog.exec();
	}
}

/**
* Opens the line detection configuration dialog and automatically displays the bottom text lines
* when lines were detected
* \sa DkLineDetectionDialog DkLineDetection
**/
void DkDocAnalysisViewPort::openLineDetectionDialog() {

	nmc::DkBaseViewPort* viewport = dynamic_cast<nmc::DkBaseViewPort*>(parent());
	if (viewport->getImage().isNull()) return;

	if (!lineDetectionDialog) {
		lineDetectionDialog = new DkLineDetectionDialog(lineDetection, metadata, this, 0);
		//connect(magicCutDialog, SIGNAL(savePressed(QImage, QString)), this, SLOT(saveMagicCutPressed(QImage, QString)));
	}

	bool done = lineDetectionDialog->exec();

	if(lineDetection->hasTextLines()) {
		emit enableShowTextLinesSignal(true);
		showBottomTextLines(true);
		
	}
}

/**
* Sets the flag to indicate that the bottom text lines are shown - used for rendering
* @param show Bottom text lines shall be shown/hidden
**/
void DkDocAnalysisViewPort::showBottomTextLines(bool show) {
	showBottomLines = show;
	emit toggleBottomTextLinesButtonSignal(show);
	update();
}

/**
* Sets the flag to indicate that the top text lines are shown - used for rendering
* @param show Top text lines shall be shown/hidden
**/
void DkDocAnalysisViewPort::showTopTextLines(bool show) {
	showTopLines = show;
	emit toggleTopTextLinesButtonSignal(show);
	update();
}

/**
* Sets the flag to indicate that the bottom text lines are shown or not, depending on the
* previous state
**/
void DkDocAnalysisViewPort::showBottomTextLines() {
	showBottomLines = !showBottomLines;
	emit toggleBottomTextLinesButtonSignal(showBottomLines);
	update();
}

/**
* Sets the flag to indicate that the top text lines are shown or not, depending on the
* previous state
**/
void DkDocAnalysisViewPort::showTopTextLines() {
	showTopLines = !showTopLines;
	emit toggleTopTextLinesButtonSignal(showTopLines);
	update();
}

/**
* Emits the Signal to the base PluginViewPort class to close the Plugin
* \sa DkPluginViewPort closePlugin()
**/
void DkDocAnalysisViewPort::cancelPlugin() {
	emit closePlugin();
}



/*-----------------------------------DkDocAnalysisToolBar ---------------------------------------------*/
DkDocAnalysisToolBar::DkDocAnalysisToolBar(const QString & title, QWidget * parent /* = 0 */) : QToolBar(title, parent) {

	createIcons();
	createLayout();
	QMetaObject::connectSlotsByName(this);

	setIconSize(QSize(nmc::Settings::param().display().iconSize, nmc::Settings::param().display().iconSize));

	if (nmc::Settings::param().display().toolbarGradient) {

		QColor hCol = nmc::Settings::param().display().highlightColor;
		hCol.setAlpha(80);

		setStyleSheet(
			//QString("QToolBar {border-bottom: 1px solid #b6bccc;") +
			QString("QToolBar {border: none; background: QLinearGradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #edeff9, stop: 1 #bebfc7); spacing: 3px; padding: 3px;}")
			+ QString("QToolBar::separator {background: #656565; width: 1px; height: 1px; margin: 3px;}")
			//+ QString("QToolButton:disabled{background-color: rgba(0,0,0,10);}")
			+ QString("QToolButton:hover{border: none; background-color: rgba(255,255,255,80);} QToolButton:pressed{margin: 0px; border: none; background-color: " + nmc::DkUtils::colorToString(hCol) + ";}")
			);
	}
	else
		setStyleSheet("QToolBar{spacing: 3px; padding: 3px;}");

	qDebug() << "[DOCANALYSIS TOOLBAR] created...";
}

DkDocAnalysisToolBar::~DkDocAnalysisToolBar() {

	qDebug() << "[DOCANALYSIS TOOLBAR] deleted...";
}

/**
* Enables/Disables All the actions which require a loaded image
**/
void DkDocAnalysisToolBar::enableNoImageActions(bool enable) {

	actions[linedetection_action]->setEnabled(enable);
	actions[distance_action]->setEnabled(enable);
	actions[magic_action]->setEnabled(enable);
}

void DkDocAnalysisToolBar::createIcons() {

	// create icons
	icons.resize(icons_end);

	icons[linedetection_icon] = QIcon(":/nomacsPluginDocAnalysis/img/detect_lines.png");
	icons[showbottomlines_icon] = QIcon(":/nomacsPluginDocAnalysis/img/lower_lines.png");
	icons[showtoplines_icon] = QIcon(":/nomacsPluginDocAnalysis/img/upper_lines.png");
	icons[distance_icon] = QIcon(":/nomacsPluginDocAnalysis/img/distance.png");
	icons[magic_icon] = QIcon(":/nomacsPluginDocAnalysis/img/magic_wand.png");
	icons[savecut_icon] = QIcon(":/nomacsPluginDocAnalysis/img/save_cut.png");
	icons[clearselection_icon] = QIcon(":/nomacsPluginDocAnalysis/img/reset_cut.png");
	icons[clearsingleselection_icon] = QIcon(":/nomacsPluginDocAnalysis/img/reset_cut_single.png");
	icons[cancel_icon] = QIcon(":/nomacsPluginDocAnalysis/img/cancel.png");
	icons[undoselection_icon] = QIcon(":/nomacsPluginDocAnalysis/img/selection_undo.png");


	if (!nmc::Settings::param().display().defaultIconColor) {
		// now colorize all icons
		for (int idx = 0; idx < icons.size(); idx++) {

			icons[idx].addPixmap(nmc::DkImage::colorizePixmap(icons[idx].pixmap(100, QIcon::Normal, QIcon::On), nmc::Settings::param().display().iconColor), QIcon::Normal, QIcon::On);
			icons[idx].addPixmap(nmc::DkImage::colorizePixmap(icons[idx].pixmap(100, QIcon::Normal, QIcon::Off), nmc::Settings::param().display().iconColor), QIcon::Normal, QIcon::Off);
		}
	}
}

void DkDocAnalysisToolBar::createLayout() {

	/*QList<QKeySequence> enterSc;
	enterSc.append(QKeySequence(Qt::Key_Enter));
	enterSc.append(QKeySequence(Qt::Key_Return));*/
	actions.resize(actions_end);

	QAction* linedetectionAction = new QAction(icons[linedetection_icon], tr("Detect text lines"), this);
	linedetectionAction->setShortcut(Qt::SHIFT + Qt::Key_D);
	linedetectionAction->setStatusTip(tr("Opens dialog to configure and start the detection of text lines for the current image"));
	linedetectionAction->setCheckable(false);
	linedetectionAction->setChecked(false);
	linedetectionAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	linedetectionAction->setObjectName("linedetectionAction");
	actions[linedetection_action] = linedetectionAction;

	QAction* showbottomlinesAction = new QAction(icons[showbottomlines_icon], tr("Show detected bottom text lines"), this);
	showbottomlinesAction->setShortcut(Qt::SHIFT + Qt::Key_L);
	showbottomlinesAction->setStatusTip(tr("Displays the previously calculated lower text lines for the image"));
	showbottomlinesAction->setCheckable(true);
	showbottomlinesAction->setChecked(false);
	showbottomlinesAction->setEnabled(false);
	showbottomlinesAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	showbottomlinesAction->setObjectName("showbottomlinesAction");
	actions[showbottomlines_action] = showbottomlinesAction;

	QAction* showtoplinesAction = new QAction(icons[showtoplines_icon], tr("Show detected top text lines"), this);
	showtoplinesAction->setShortcut(Qt::SHIFT + Qt::Key_U);
	showtoplinesAction->setStatusTip(tr("Displays the previously calculated upper text lines for the image"));
	showtoplinesAction->setCheckable(true);
	showtoplinesAction->setChecked(false);
	showtoplinesAction->setEnabled(false);
	showtoplinesAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	showtoplinesAction->setObjectName("showtoplinesAction");
	actions[showtoplines_action] = showtoplinesAction;

	QAction* distanceAction = new QAction(icons[distance_icon], tr("Measure distance"), this);
	distanceAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_D);
	distanceAction->setStatusTip(tr("Measure the distance between 2 points (in cm)"));
	distanceAction->setCheckable(true);
	distanceAction->setChecked(false);
	distanceAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	distanceAction->setObjectName("distanceAction");
	actions[distance_action] = distanceAction;

	QAction* magicAction = new QAction(icons[magic_icon], tr("Select region"), this);
	magicAction->setShortcut(Qt::SHIFT + Qt::Key_S);
	magicAction->setStatusTip(tr("Select regions with similar color"));
	magicAction->setCheckable(true);
	magicAction->setChecked(false);
	magicAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	magicAction->setObjectName("magicAction");
	actions[magic_action] = magicAction;

	QAction* savecutAction = new QAction(icons[savecut_icon], tr("Save the selected region"), this);
	savecutAction->setStatusTip(tr("Open the Save Dialog for saving the currently selected region"));
	savecutAction->setCheckable(false);
	savecutAction->setChecked(false);
	savecutAction->setEnabled(false);
	savecutAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	savecutAction->setObjectName("savecutAction");
	actions[savecut_action] = savecutAction;

	QAction* undoselectionAction = new QAction(icons[undoselection_icon], tr("Clear previous selected region"), this);
	undoselectionAction->setShortcut(Qt::CTRL + Qt::Key_Z);
	undoselectionAction->setStatusTip(tr("Clears the previously selected region"));
	undoselectionAction->setCheckable(false);
	undoselectionAction->setChecked(false);
	undoselectionAction->setEnabled(false);
	undoselectionAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	undoselectionAction->setObjectName("undoselectionAction");
	actions[undoselection_action] = undoselectionAction;

	QAction* clearsingleselectionAction = new QAction(icons[clearsingleselection_icon], tr("Clear selection of a single region"), this);
	clearsingleselectionAction->setShortcut(Qt::SHIFT + Qt::Key_C);
	clearsingleselectionAction->setStatusTip(tr("Select selected region to cleared"));
	clearsingleselectionAction->setCheckable(true);
	clearsingleselectionAction->setChecked(false);
	clearsingleselectionAction->setEnabled(false);
	clearsingleselectionAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	clearsingleselectionAction->setObjectName("clearsingleselectionAction");
	actions[clearsingleselection_action] = clearsingleselectionAction;

	QAction* clearselectionAction = new QAction(icons[clearselection_icon], tr("Clear selection"), this);
	clearselectionAction->setShortcut(Qt::ALT + Qt::Key_C);
	clearselectionAction->setStatusTip(tr("Clear the current selection"));
	clearselectionAction->setCheckable(false);
	clearselectionAction->setChecked(false);
	clearselectionAction->setEnabled(false);
	clearselectionAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	clearselectionAction->setObjectName("clearselectionAction");
	actions[clearselection_action] = clearselectionAction;

	QLabel *lbl_tolerance = new QLabel(tr("Tolerance:"));
	lbl_tolerance->setStatusTip(tr("Set the maximum color difference tolerance for flood-filling"));
	lbl_tolerance->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable

	QSpinBox *toleranceBox = new QSpinBox();
	toleranceBox->setStatusTip(tr("Set the maximum color difference tolerance for flood-filling"));
	toleranceBox->setRange(0, 255);
	toleranceBox->setValue(40);
	toleranceBox->setWhatsThis(tr("alwaysenabled"));
	toleranceBox->setObjectName("toleranceBox");

	QAction* cancelpluginAction = new QAction(icons[cancel_icon], tr("Close the Plugin"), this);
	//clearsingleselectionAction->setShortcut(Qt::SHIFT + Qt::Key_C);
	cancelpluginAction->setStatusTip(tr("Close the Document Analysis Plugin"));
	cancelpluginAction->setCheckable(false);
	cancelpluginAction->setChecked(false);
	cancelpluginAction->setEnabled(true);
	cancelpluginAction->setWhatsThis(tr("alwaysenabled")); // set flag to always make this icon clickable
	cancelpluginAction->setObjectName("cancelpluginAction");
	actions[cancelplugin_action] = cancelpluginAction;

	// actually add things to interface
	addAction(linedetectionAction);
	addAction(showbottomlinesAction);
	addAction(showtoplinesAction);
	addSeparator();
	addAction(distanceAction);
	addSeparator();
	addAction(magicAction);
	addAction(savecutAction);
	addAction(undoselectionAction);
	addAction(clearsingleselectionAction);
	addAction(clearselectionAction);
	addWidget(lbl_tolerance);
	addWidget(toleranceBox);
	addSeparator();
	addAction(cancelpluginAction);
}

/**
* Make the DocAnalyis toolbar visible
**/
void DkDocAnalysisToolBar::setVisible(bool visible) {

	qDebug() << "[DOCANALYSIS TOOLBAR] set visible: " << visible;

	QToolBar::setVisible(visible);
}

/**
* Called when the detect lines tool icon is clicked.
* Emits signal (a request) to open the configuration dialog for detecting text lines.
* \sa detectLinesSignal() DkDocAnalysisViewPort::openLineDetectionDialog() DkLineDetection
**/
void DkDocAnalysisToolBar::on_linedetectionAction_triggered() {
	emit detectLinesSignal();
}

/**
* Called when the show bottom text lines tool icon is pressed.
* Willd display the previously detected bottom text lines if available.
* Emits signal (a request) to display/hide the lines.
* \sa showBottomTextLinesSignal(bool) DkDocAnalysisViewPort::showBottomTextLines(bool show) DkLineDetection
**/
void DkDocAnalysisToolBar::on_showbottomlinesAction_triggered() {
	emit showBottomTextLinesSignal(actions[showbottomlines_action]->isChecked());
}

/**
* Called when the show top text lines tool icon is pressed.
* Willd display the previously detected top text lines if available.
* Emits signal (a request) to display/hide the lines.
* \sa showTopTextLinesSignal(bool) DkDocAnalysisViewPort::showBottomTextLines(bool show) DkLineDetection
**/
void DkDocAnalysisToolBar::on_showtoplinesAction_triggered() {
	emit showTopTextLinesSignal(actions[showtoplines_action]->isChecked());
}

/**
* Called when the measure distance tool icon is clicked.
* Emits signal (a request) to start/end this tool.
* \sa measureDistanceRequest(bool) DkDocAnalysisViewPort::pickDistancePoint(bool pick) DkDistanceMeasure
**/
void DkDocAnalysisToolBar::on_distanceAction_toggled(bool checked) {
	emit measureDistanceRequest(actions[distance_action]->isChecked());
}

/**
* Called when the magic cut tool icon is clicked.
* Emits signal (a request) to start/end this tool.
* \sa pickSeedpointRequest(bool) DkDocAnalysisViewPort::pickSeedpoint(bool pick) DkMagicCut
**/
void DkDocAnalysisToolBar::on_magicAction_toggled(bool checked) {
	emit pickSeedpointRequest(actions[magic_action]->isChecked());
}

/**
* Called when the currently selected magic cut areas shall be saved (click on the icon)
* \sa openCutDialogSignal() DkDocAnalysisViewPort::openMagicCutDialog() DkMagicCutDialog
**/
void DkDocAnalysisToolBar::on_savecutAction_triggered() {
	emit openCutDialogSignal();
}

/**
* Called when the clear single magic cut selection tool icon is clicked.
* Emits signal (a request) to start/end a tool for clearing single selected regions by selecting them again.
* \sa clearSingleSelectionRequest(bool) DkDocAnalysisViewPort::pickResetRegionPoint(bool pick) DkMagicCut
**/
void DkDocAnalysisToolBar::on_clearsingleselectionAction_toggled(bool checked) {
	emit clearSingleSelectionRequest(actions[clearsingleselection_action]->isChecked());
}

/**
* Called when the undo previously selected region icon is clicked.
* Emits a signal to reset the last selected region (in any)
* \sa undoSelectionSignal() DkMagicCut
**/
void DkDocAnalysisToolBar::on_undoselectionAction_triggered() {
	emit undoSelectionSignal();
}

/**
* Called when the clear magic cut selection tool icon is clicked.
* Emits signal (a request) to clear all selected magic cut regions.
* \sa clearSelectionSignal() DkDocAnalysisViewPort::clearMagicCut() DkMagicCut
**/
void DkDocAnalysisToolBar::on_clearselectionAction_triggered() {
	emit clearSelectionSignal();
}

/**
* Called when the user clicks the icon to cancel the Plugin.
* Emits a signal to the Plugin to close it.
* \sa cancelPlugin()
**/
void DkDocAnalysisToolBar::on_cancelpluginAction_triggered() {
	emit cancelPlugin();
}

/**
* Called when the tolerance value has changed within the toolbar.
* @param value The new tolerance value
* \sa toeranceChanged(int) DkDocAnalysisViewPort::setMagicCutTolerance(int) DkMagicCut::magicwand(QPoint)
**/
void DkDocAnalysisToolBar::on_toleranceBox_valueChanged(int value) {
	emit toleranceChanged(value);
}


/**
* Slot - called when the user canceles during picking a seedpoint in the magic cut tool.
* Untoggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::pickSeedpointCanceled() {
	actions[magic_action]->setChecked(false);
}

/**
* Slot - called when the user starts the picking seedpoints for magic cut tool (e.g. via shortcut).
* Toggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::pickSeedpointStarted() {
	actions[magic_action]->setChecked(true);
}

/**
* Slot - called when the user canceles during clearing single selected regions in the magic cut tool.
* Untoggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::clearSingleRegionCanceled() {
	actions[clearsingleselection_action]->setChecked(false);
}

/**
* Slot - called when the user starts the clearing single selected regions magic cut tool (e.g. via shortcut).
* Toggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::clearSingleRegionStarted() {
	actions[clearsingleselection_action]->setChecked(true);
}

/**
* Slot - called when the user canceles during picking a distance measure point.
* Untoggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::measureDistanceCanceled() {
	actions[distance_action]->setChecked(false);
}

/**
* Slot - called when the user requests to measure the distance (e.g. via shortcut).
* Toggles the corresponding tool icon.
**/
void DkDocAnalysisToolBar::measureDistanceStarted() {
	actions[distance_action]->setChecked(true);
}

/**
* Slot - called when the icon should be enabled or disabled (e.g. nothing selected, no image, ...)
**/
void DkDocAnalysisToolBar::enableButtonSaveCut(bool enable) {
	actions[savecut_action]->setEnabled(enable);
	// also enable the clear selection, since something has been selected
	actions[clearselection_action]->setEnabled(enable);
	actions[clearsingleselection_action]->setEnabled(enable);
	actions[undoselection_action]->setEnabled(enable);
}

/**
* Slot - called when the icon should be enabled or disabled (e.g. no image, no text lines detected...)
**/
void DkDocAnalysisToolBar::enableButtonShowTextLines(bool enable) {
	actions[showbottomlines_action]->setEnabled(enable);
	actions[showtoplines_action]->setEnabled(enable);
}

/**
* Slot - called when after calculation of text lines the bottom text lines are automatically
* displayed to also toggle the corresponding icon
**/
void DkDocAnalysisToolBar::toggleBottomTextLinesButton(bool toggle) {
	actions[showbottomlines_action]->setChecked(toggle);
}

/**
* Slot - called when after calculation of text lines the top text lines are automatically
* enabled, in case they have been enabled in a previous text line calculation
**/
void DkDocAnalysisToolBar::toggleTopTextLinesButton(bool toggle) {
	actions[showtoplines_action]->setChecked(toggle);
}


};
