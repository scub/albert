// albert - a simple application launcher for linux
// Copyright (C) 2014 Manuel Schneider
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QApplication>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QDebug>

#include "mainwidget.h"
#include "extensionhandler.h"
#include "settingsdialog.h"
#include "settings.h"
#include "globalhotkey.h"


void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QByteArray localMsg = msg.toLocal8Bit();
	switch (type) {
	case QtDebugMsg:
		fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		break;
	case QtWarningMsg:
		fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		break;
	case QtCriticalMsg:
		fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		break;
	case QtFatalMsg:
		fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
		abort();
	}
}







int main(int argc, char *argv[])
{
//	qInstallMessageHandler(myMessageOutput);

	/*
	 *  INITIALIZE APPLICATION
	 */

	QApplication          a(argc, argv);
//	QCoreApplication::setOrganizationName(QString::fromLocal8Bit("albert"));
	QCoreApplication::setApplicationName(QString::fromLocal8Bit("albert"));
	a.setWindowIcon(QIcon(":app_icon"));
	a.setQuitOnLastWindowClosed(false); // Dont quit after settings close

	MainWidget            mainWidget;
	SettingsWidget        settingsDialog(&mainWidget);
	GlobalHotkey          hotkeyManager;
	QSortFilterProxyModel sortProxyModel;
	ExtensionHandler      extensionHandler;

	extensionHandler.initialize();
	mainWidget._proposalListView->setModel(&sortProxyModel);

	// TODO STUFF
	//	{ // FIRST RUN STUFF
	//		QDir data(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
	//		if (!data.exists())
	//			data.mkpath(".");
	//		QDir conf(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
	//				  +"/"+qApp->applicationName());
	//		if (!conf.exists())
	//			conf.mkpath(".");
	//	}
	//	QFileSystemWatcher fsw;
	//	fsw.addPath(QSettings(QSettings::UserScope, "albert", "albert").fileName());
	//	connect(&fsw, QFileSystemWatcher::fileChanged,
	//			TODO ALLES WAS RELOADEN KANN, &ExtensionHandler::reloadConfig);

	/*
	 *  HOTKEY
	 */

	// Albert without hotkey is useless. Force it!
	hotkeyManager.registerHotkey(gSettings->value("hotkey", "").toString());
	if (hotkeyManager.hotkey() == 0) {
		QMessageBox msgBox(QMessageBox::Critical, "Error",
						   "Hotkey is invalid, please set it. Press ok to open"\
						   "the settings, or press close to quit albert.",
						   QMessageBox::Close|QMessageBox::Ok);
		msgBox.exec();
		if ( msgBox.result() == QMessageBox::Ok )
			settingsDialog.show();
		else
			exit(0);
	}


	/*
	 *  THEME
	 */

	{
		QString theme = gSettings->value("Theme", "Standard").toString();
		qDebug() << gSettings->fileName();
		qDebug() << QStandardPaths::standardLocations(QStandardPaths::ConfigLocation);
		qDebug() << QStandardPaths::standardLocations(QStandardPaths::DataLocation);
		qDebug() << QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
		QFileInfoList themes;
		QStringList themeDirs = QStandardPaths::locateAll(
			QStandardPaths::DataLocation, "themes", QStandardPaths::LocateDirectory
		);
		for (QDir d : themeDirs)
			themes << d.entryInfoList(QStringList("*.qss"), QDir::Files | QDir::NoSymLinks);
		// Find and apply the theme
		bool success = false;
		for (QFileInfo fi : themes){
			if (fi.baseName() == theme) {
				QFile styleFile(fi.canonicalFilePath());
				if (styleFile.open(QFile::ReadOnly)) {
					qApp->setStyleSheet(styleFile.readAll());
					styleFile.close();
					success = true;
					break;
				}
			}
		}
		if (!success) {
			qFatal("FATAL: Stylefile not found: %s", theme.toStdString().c_str());
			exit(EXIT_FAILURE);
		}
	}


	/*
	 *  SETUP SIGNAL FLOW
	 */

	// Show mainwidget if hotkey is pressed
	QObject::connect(&hotkeyManager, &GlobalHotkey::hotKeyPressed,
					 &mainWidget, &MainWidget::toggleVisibility);

	// Setup and teardown query sessions with the state of the widget
	QObject::connect(&mainWidget, &MainWidget::widgetShown,
					 &extensionHandler, &ExtensionHandler::setupSession);
	QObject::connect(&mainWidget, &MainWidget::widgetHidden,
					 &extensionHandler, &ExtensionHandler::teardownSession);

	// settingsDialogRequested closes albert + opens settings dialog
	QObject::connect(mainWidget._inputLine, &InputLine::settingsDialogRequested,
					 &mainWidget, &MainWidget::hide);
	QObject::connect(mainWidget._inputLine, &InputLine::settingsDialogRequested,
					 &settingsDialog, (void (SettingsWidget::*)())&SettingsWidget::show);

	// A change in text triggers requests
	QObject::connect(mainWidget._inputLine, &QLineEdit::textChanged,
					 &extensionHandler, &ExtensionHandler::startQuery);

	// Make the list show the results of the current query
	QObject::connect(&extensionHandler, &ExtensionHandler::currentQueryChanged,
					 &sortProxyModel, &QSortFilterProxyModel::setSourceModel);



// DEBUG
//	QObject::connect(&extensionHandler, &ExtensionHandler::currentQueryChanged,
//					 [=](QAbstractItemModel* model) { qDebug() <<  "currentQueryChanged fired:" << model;});



	// Run an activated entry in the list
//	QObject::connect(mainWidget._proposalListView, &ProposalListView::activated,
//					 &queryAdapter, &QueryAdapter::run);


	/*
	 *  E N T E R   T H E   L O O P
	 */

	return a.exec();

	/*
	 *  CLEANUP
	 */
	extensionHandler.finalize();
}
