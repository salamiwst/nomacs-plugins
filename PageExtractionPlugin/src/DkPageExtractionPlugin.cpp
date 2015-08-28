/*******************************************************************************************************
 DkPageExtractionPlugin.cpp

 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances

 Copyright (C) 2015 Markus Diem

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

#include "DkPageExtractionPlugin.h"
#include "DkPageSegmentation.h"

#include "DkImageStorage.h"

#pragma warning(push, 0)	// no warnings from includes - begin
#include <QAction>
#include <QDebug>
#pragma warning(pop)		// no warnings from includes - end

namespace nmc {

/**
*	Constructor
**/
DkPageExtractionPlugin::DkPageExtractionPlugin(QObject* parent) : QObject(parent) {

	// create run IDs
	QVector<QString> runIds;
	runIds.resize(id_end);

	runIds[id_crop_to_page] = "1638a7f56b814ee48c6eb8a7710e74b4";
	runIds[id_draw_to_page] = "2af5c9f018ce4a0fbfaacc5e3a48a4b5";
	mRunIDs = runIds.toList();

	// create menu actions
	QVector<QString> menuNames;
	menuNames.resize(id_end);
		
	menuNames[id_crop_to_page] = tr("Crop to Page");
	menuNames[id_draw_to_page] = tr("Draw to Page");
	mMenuNames = menuNames.toList();

	// create menu status tips
	QVector<QString> statusTips;
	statusTips.resize(id_end);

	statusTips[id_crop_to_page] = tr("Finds a page in a document image and then crops the image to that page.");
	statusTips[id_draw_to_page] = tr("Finds a page in a document image and then draws the found document boundaries.");
	mMenuStatusTips = statusTips.toList();
}

/**
*	Destructor
**/
DkPageExtractionPlugin::~DkPageExtractionPlugin() {
}


/**
* Returns unique ID for the generated dll
**/
QString DkPageExtractionPlugin::pluginID() const {

	return PLUGIN_ID;
};


/**
* Returns plugin name
* @param plugin ID
**/
QString DkPageExtractionPlugin::pluginName() const {

	return tr("Document Page Extraction");
};

/**
* Returns long description for every ID
* @param plugin ID
**/
QString DkPageExtractionPlugin::pluginDescription() const {

	return "<b>Created by:</b> Markus Diem <br><b>Modified:</b>27.08.2015<br><b>Description:</b> Detects document pages in images.";
};

/**
* Returns descriptive iamge for every ID
* @param plugin ID
**/
QImage DkPageExtractionPlugin::pluginDescriptionImage() const {

	return QImage(":/#PLUGIN_NAME/img/your-image.png");
};

/**
* Returns plugin version for every ID
* @param plugin ID
**/
QString DkPageExtractionPlugin::pluginVersion() const {

	return PLUGIN_VERSION;
};

/**
* Returns unique IDs for every plugin in this dll
**/
QStringList DkPageExtractionPlugin::runID() const {

	//GUID without hyphens generated at http://www.guidgenerator.com/
	return QStringList() << "#RUN_GUID";
};

/**
* Returns plugin name for every ID
* @param plugin ID
**/
QString DkPageExtractionPlugin::pluginMenuName(const QString &runID) const {

	return tr("Document Page Extraction");
};

/**
* Returns short description for status tip for every ID
* @param plugin ID
**/
QString DkPageExtractionPlugin::pluginStatusTip(const QString &runID) const {

	return tr("#MENU_STATUS_TIP");
};

QList<QAction*> DkPageExtractionPlugin::pluginActions(QWidget* parent) {

	if (mActions.empty()) {
		QAction* ca = new QAction(mMenuNames[id_crop_to_page], this);
		ca->setObjectName(mMenuNames[id_crop_to_page]);
		ca->setStatusTip(mMenuStatusTips[id_crop_to_page]);
		ca->setData(mRunIDs[id_crop_to_page]);	// runID needed for calling function runPlugin()
		mActions.append(ca);

		ca = new QAction(mMenuNames[id_draw_to_page], this);
		ca->setObjectName(mMenuNames[id_draw_to_page]);
		ca->setStatusTip(mMenuStatusTips[id_draw_to_page]);
		ca->setData(mRunIDs[id_draw_to_page]);	// runID needed for calling function runPlugin()
		mActions.append(ca);
	}

	return mActions;
}

/**
* Main function: runs plugin based on its ID
* @param plugin ID
* @param image to be processed
**/
QImage DkPageExtractionPlugin::runPlugin(const QString &runID, const QImage &image) const {

	if (!mRunIDs.contains(runID))
		return image;

	cv::Mat img = DkImage::qImage2Mat(image);

	// run the page segmentation
	DkPageSegmentation segM(img);
	segM.compute();
	segM.filterDuplicates();

	// do whatever the user requested
	if(runID == mRunIDs[id_crop_to_page]) {
		return segM.getCropped(image);
	}
	else if(runID == mRunIDs[id_draw_to_page]) {
		
		QImage dImg = image;
		segM.draw(dImg);
		return dImg;
	}

	// wrong runID? - do nothing
	return image;
};

};

