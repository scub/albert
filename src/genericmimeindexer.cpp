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

#include <QSettings>
#include "genericmimeitem.h"
#include "genericmimeindexer.h"

/**************************************************************************//**
 * @brief GenericMimeIndexer::GenericMimeIndexer
 */
GenericMimeIndexer::GenericMimeIndexer()
{
}

/**************************************************************************//**
 * @brief GenericMimeIndexer::~GenericMimeIndexer
 */
GenericMimeIndexer::~GenericMimeIndexer()
{

}

/**************************************************************************//**
 * @brief GenericMimeIndexer::buildIndex
 */
void GenericMimeIndexer::buildIndex()
{
  QSettings conf;
	std::cout << "Config: " << conf.fileName().toStdString() << std::endl;
	QStringList paths = conf.value(QString::fromLocal8Bit("paths")).toStringList();

	// Define a lambda for recursion
	std::function<void(const QString& p)> rec_dirsearch = [&] (const QString& p)
  {
		QDir dir(p);
		dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks);
		_index.push_back(new GenericMimeItem(dir.dirName(), dir.canonicalPath()));

		// go recursive into subdirs
		QFileInfoList list = dir.entryInfoList();
		for ( QFileInfo &fi : list)
		{
			if (fi.isDir())
				rec_dirsearch(fi.absoluteFilePath());
      _index.push_back(new GenericMimeItem(fi.completeBaseName(), fi.absoluteFilePath()));
		}
	};

	// Finally do this recursion for all paths
	for ( auto path : paths)
		rec_dirsearch(path);

	std::sort(_index.begin(), _index.end(), lexicographically);
	qDebug() << "Found" << _index.size() << "items.";
}


/**************************************************************************//**
 * @brief GenericMimeIndexer::configWidget
 * @return
 */
QWidget *GenericMimeIndexer::configWidget()
{
	return new QWidget;
}