/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "subcomponentmanager.h"
#include "model.h"
#include "metainfo.h"

#include <QDir>
#include <QMetaType>
#include <QUrl>

enum { debug = false };

QT_BEGIN_NAMESPACE

// Allow usage of QFileInfo in qSort

static bool operator<(const QFileInfo &file1, const QFileInfo &file2)
{
    return file1.filePath() < file2.filePath();
}


QT_END_NAMESPACE

static inline QStringList importPaths() {
    QStringList paths;

    // env import paths
    QByteArray envImportPath = qgetenv("QML_IMPORT_PATH");
    if (!envImportPath.isEmpty()) {
#if defined(Q_OS_WIN)
        QLatin1Char pathSep(';');
#else
        QLatin1Char pathSep(':');
#endif
        paths = QString::fromLatin1(envImportPath).split(pathSep, QString::SkipEmptyParts);
    }

    return paths;
}

namespace QmlDesigner {
static const QString s_qmlFilePattern = QString(QLatin1String("*.qml"));

SubComponentManager::SubComponentManager(Model *model, QObject *parent)
    : QObject(parent),
      m_model(model)
{
    connect(&m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(parseDirectory(QString)));
}

void SubComponentManager::addImport(int pos, const Import &import)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << pos << import.file().toAscii();

    if (import.isFileImport()) {
        QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
        if (dirInfo.exists() && dirInfo.isDir()) {
            const QString canonicalDirPath = dirInfo.canonicalFilePath();
            m_watcher.addPath(canonicalDirPath);
            //m_dirToQualifier.insertMulti(canonicalDirPath, import.qualifier()); ### todo: proper support for import as
        }
    } else {
        QString url = import.url();
        
        url.replace(QLatin1Char('.'), QLatin1Char('/'));

        foreach (const QString &path, importPaths()) {
            url  = path + QLatin1Char('/') + url;
            QFileInfo dirInfo = QFileInfo(url);
            if (dirInfo.exists() && dirInfo.isDir()) {
                const QString canonicalDirPath = dirInfo.canonicalFilePath();
                m_watcher.addPath(canonicalDirPath);
                //m_dirToQualifier.insertMulti(canonicalDirPath, import.qualifier()); ### todo: proper support for import as
            }
        }
        // TODO: QDeclarativeDomImport::Library
    }

    m_imports.insert(pos, import);
}

void SubComponentManager::removeImport(int pos)
{
    const Import import = m_imports.takeAt(pos);

    if (import.isFileImport()) {
        const QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
        const QString canonicalDirPath = dirInfo.canonicalFilePath();

        //m_dirToQualifier.remove(canonicalDirPath, import.qualifier()); ### todo: proper support for import as

        if (!m_dirToQualifier.contains(canonicalDirPath))
            m_watcher.removePath(canonicalDirPath);

//        foreach (const QFileInfo &monitoredFile, watchedFiles(canonicalDirPath)) { ### todo: proper support for import as
//            if (!m_dirToQualifier.contains(canonicalDirPath))
//                unregisterQmlFile(monitoredFile, import.qualifier());
//        }
    } else {
            // TODO: QDeclarativeDomImport::Library
    }
}

void SubComponentManager::parseDirectories()
{
    if (!m_filePath.isEmpty()) {
        const QString file = m_filePath.toLocalFile();
        QFileInfo dirInfo = QFileInfo(QFileInfo(file).path());
        if (dirInfo.exists() && dirInfo.isDir())
            parseDirectory(dirInfo.canonicalFilePath());
    }

    foreach (const Import &import, m_imports) {
        if (import.isFileImport()) {
            QFileInfo dirInfo = QFileInfo(m_filePath.resolved(import.file()).toLocalFile());
            if (dirInfo.exists() && dirInfo.isDir()) {
                parseDirectory(dirInfo.canonicalFilePath());
            }
        } else {
            QString url = import.url();
            foreach (const QString &path, importPaths()) {
                url.replace(QLatin1Char('.'), QLatin1Char('/'));
                url  = path + QLatin1Char('/') + url;
                QFileInfo dirInfo = QFileInfo(url);
                if (dirInfo.exists() && dirInfo.isDir()) {
                    //### todo full qualified names QString nameSpace = import.uri();
                    parseDirectory(dirInfo.canonicalFilePath(), false);
                }
            }
        }
    }
}

void SubComponentManager::parseDirectory(const QString &canonicalDirPath, bool addToLibrary, const QString& qualification)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << canonicalDirPath;

    QDir dir(canonicalDirPath);

    dir.setNameFilters(QStringList(s_qmlFilePattern));
    dir.setFilter(QDir::Files | QDir::Readable | QDir::CaseSensitive);

    QList<QFileInfo> monitoredList = watchedFiles(canonicalDirPath);
    QList<QFileInfo> newList;
    foreach (const QFileInfo &qmlFile, dir.entryInfoList()) {
        if (QFileInfo(m_filePath.toLocalFile()) == qmlFile) {
            // do not parse main file
            continue;
        }
        if (!qmlFile.fileName().at(0).isUpper()) {
            // QML sub components must be upper case
            continue;
        }
        newList << qmlFile;
    }

    qSort(monitoredList);
    qSort(newList);

    if (debug)
        qDebug() << "monitored list " << monitoredList.size() << "new list " << newList.size();
    QList<QFileInfo>::const_iterator oldIter = monitoredList.constBegin();
    QList<QFileInfo>::const_iterator newIter = newList.constBegin();

    while (oldIter != monitoredList.constEnd() && newIter != newList.constEnd()) {
        const QFileInfo oldFileInfo = *oldIter;
        const QFileInfo newFileInfo = *newIter;
        if (oldFileInfo == newFileInfo) {
            ++oldIter;
            ++newIter;
            continue;
        }
        if (oldFileInfo < newFileInfo) {
            foreach (const QString &qualifier, m_dirToQualifier.value(canonicalDirPath))
                unregisterQmlFile(oldFileInfo, qualifier);
            m_watcher.removePath(oldFileInfo.filePath());
            ++oldIter;
            continue;
        }
        // oldFileInfo > newFileInfo
        parseFile(newFileInfo.filePath(), addToLibrary, qualification);
        ++newIter;
    }

    while (oldIter != monitoredList.constEnd()) {
        foreach (const QString &qualifier, m_dirToQualifier.value(canonicalDirPath))
            unregisterQmlFile(*oldIter, qualifier);
        ++oldIter;
    }

    while (newIter != newList.constEnd()) {
        parseFile(newIter->filePath(), addToLibrary, qualification);
        if (debug)
            qDebug() << "m_watcher.addPath(" << newIter->filePath() << ')';
        ++newIter;
    }
}

void SubComponentManager::parseFile(const QString &canonicalFilePath, bool addToLibrary, const QString& /* qualification */)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << canonicalFilePath;

    QFile file(canonicalFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QString dir = QFileInfo(canonicalFilePath).path();
    foreach (const QString &qualifier, m_dirToQualifier.values(dir)) {
        registerQmlFile(canonicalFilePath, qualifier, addToLibrary);
    }
    registerQmlFile(canonicalFilePath, QString(), addToLibrary);
}

void SubComponentManager::parseFile(const QString &canonicalFilePath)
{
    parseFile(canonicalFilePath, true, QString());
}

// dirInfo must already contain a canonical path
QList<QFileInfo> SubComponentManager::watchedFiles(const QString &canonicalDirPath)
{
    QList<QFileInfo> files;

    foreach (const QString &monitoredFile, m_watcher.files()) {
        QFileInfo fileInfo(monitoredFile);
        if (fileInfo.dir().absolutePath() == canonicalDirPath) {
            files.append(fileInfo);
        }
    }
    return files;
}

void SubComponentManager::unregisterQmlFile(const QFileInfo &fileInfo, const QString &qualifier)
{
    QString componentName = fileInfo.baseName();
    if (!qualifier.isEmpty())
        componentName = qualifier + '.' + componentName;
}

static inline bool isDepricatedQtType(const QString &typeName)
{
    if (typeName.length() < 8)
        return false;

    return typeName.contains("Qt.");
}


void SubComponentManager::registerQmlFile(const QFileInfo &fileInfo, const QString &qualifier,
                                                 bool addToLibrary)
{
    if (!model())
        return;

    QString componentName = fileInfo.baseName();

    if (!qualifier.isEmpty()) {
        QString fixedQualifier = qualifier;
        if (qualifier.right(1) == QLatin1String("."))
            fixedQualifier.chop(1); //remove last char if it is a dot
        componentName = fixedQualifier + '.' + componentName;
    }

    if (debug)
        qDebug() << "SubComponentManager" << __FUNCTION__ << componentName;

    if (addToLibrary) {
        // Add file components to the library
        ItemLibraryEntry itemLibraryEntry;
        itemLibraryEntry.setType(componentName, -1, -1);
        itemLibraryEntry.setName(componentName);
        itemLibraryEntry.setCategory("QML Components");


        if (model()->metaInfo(componentName).isValid() && model()->metaInfo(componentName).isSubclassOf("QtQuick.Item", -1, -1) &&
            !model()->metaInfo().itemLibraryInfo()->containsEntry(itemLibraryEntry)) {

            model()->metaInfo().itemLibraryInfo()->addEntry(itemLibraryEntry);
        }
    }
}

Model *SubComponentManager::model() const
{
    return m_model.data();
}


/*!
  \class SubComponentManager

  Detects & monitors (potential) component files in a list of directories, and registers
  these in the metatype system.
*/

QStringList SubComponentManager::directories() const
{
    return m_watcher.directories();
}

QStringList SubComponentManager::qmlFiles() const
{
    return m_watcher.files();
}

void SubComponentManager::update(const QUrl &filePath, const QList<Import> &imports)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << filePath << imports.size();

    QFileInfo oldDir, newDir;

    if (!m_filePath.isEmpty()) {
        const QString file = m_filePath.toLocalFile();
        oldDir = QFileInfo(QFileInfo(file).path());
    }
    if (!filePath.isEmpty()) {
        const QString file = filePath.toLocalFile();
        newDir = QFileInfo(QFileInfo(file).path());
    }

    m_filePath = filePath;

    //
    // (implicit) import of local directory
    //
    if (oldDir != newDir) {
        if (!oldDir.filePath().isEmpty()) {
            m_dirToQualifier.remove(oldDir.canonicalFilePath(), QString());
            if (!m_dirToQualifier.contains(oldDir.canonicalFilePath()))
                m_watcher.removePath(oldDir.filePath());
        }

        if (!newDir.filePath().isEmpty()) {
            m_dirToQualifier.insertMulti(newDir.canonicalFilePath(), QString());
        }
    }

    //
    // Imports
    //

    // skip first list items until the lists differ
    int i = 0;
    while (i < qMin(imports.size(), m_imports.size())) {
        if (!(imports.at(i) == m_imports.at(i)))
            break;
        ++i;
    }

    for (int ii = m_imports.size() - 1; ii >= i; --ii)
        removeImport(ii);

    for (int ii = i; ii < imports.size(); ++ii) {
        addImport(ii, imports.at(ii));
    }

    parseDirectories();
}

} // namespace QmlDesigner

