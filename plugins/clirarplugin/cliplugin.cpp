/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliplugin.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDateTime>
#include <QDir>
#include <QString>
#include <QStringList>

#include <KDebug>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
        : CliInterface(parent, args)
        , m_isFirstLine(true)
        , m_isInContentListing(false)
{
}

CliPlugin::~CliPlugin()
{
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = "unrar";
        p[DeleteProgram] = p[AddProgram] = "rar";

        p[ListArgs] = QStringList() << "v" << "-c-" << "-v" << "$Archive";
        p[ExtractArgs] = QStringList() << "-kb" << "-p-"
                                       << "$PreservePathSwitch"
                                       << "$PasswordSwitch"
                                       << "$RootNodeSwitch" << "$Archive"
                                       << "$Files";
        p[PreservePathSwitch] = QStringList() << "x" << "e";
        p[RootNodeSwitch] = QStringList() << "-ap$Path";
        p[PasswordSwitch] = QStringList() << "-p$Password";

        p[DeleteArgs] = QStringList() << "d" << "$Archive" << "$Files";

        p[FileExistsExpression] = "^(.+) already exists. Overwrite it";
        p[FileExistsInput] = QStringList()
                             << "Y" //overwrite
                             << "N" //skip
                             << "A" //overwrite all
                             << "E" //autoskip
                             << "Q" //cancel
                             ;

        p[AddArgs] = QStringList() << "a" << "$Archive" << "$Files";

        p[WrongPasswordPatterns] = QStringList() << "password incorrect";
        p[ExtractionFailedPatterns] = QStringList() << "CRC failed" << "Cannot find volume";
    }

    return p;
}

bool CliPlugin::readListLine(const QString &line)
{
    const QString m_headerString = "-----------------------------------------";

    // skip the heading
    if (!m_isInContentListing) {
        if (line.startsWith(m_headerString))
            m_isInContentListing = true;
        return true;
    }

    // catch final line
    if (line.startsWith(m_headerString)) {
        m_isFirstLine = true;
        m_isInContentListing = false;
        return true;
    }

    // rar gives one line for the filename and a line after it with some file properties
    if (m_isFirstLine) {
        m_internalId = line.trimmed();
        //m_entryFilename.chop(1); // handle newline
        if (!m_internalId.isEmpty() && m_internalId.at(0) == '*') {
            m_isPasswordProtected = true;
            m_internalId.remove(0, 1);   // and the spaces in front
        } else
            m_isPasswordProtected = false;

        m_isFirstLine = false;
        return true;
    }

    QStringList fileprops = line.split(' ', QString::SkipEmptyParts);
    m_internalId = QDir::fromNativeSeparators(m_internalId);
    bool isDirectory = (bool)(fileprops[ 5 ].contains('d', Qt::CaseInsensitive));

    QDateTime ts(QDate::fromString(fileprops[ 3 ], "dd-MM-yy"),
                 QTime::fromString(fileprops[ 4 ], "hh:mm"));
    // rar output date with 2 digit year but QDate takes is as 19??
    // let's take 1950 is cut-off; similar to KDateTime
    if (ts.date().year() < 1950)
        ts = ts.addYears(100);

    m_entryFilename = m_internalId;
    if (isDirectory && !m_internalId.endsWith('/')) {
        m_entryFilename += '/';
    }

    //kDebug() << m_entryFilename << " : " << fileprops ;
    ArchiveEntry e;
    e[ FileName ] = m_entryFilename;
    e[ InternalID ] = m_internalId;
    e[ Size ] = fileprops[ 0 ];
    e[ CompressedSize] = fileprops[ 1 ];
    e[ Ratio ] = fileprops[ 2 ];
    e[ Timestamp ] = ts;
    e[ IsDirectory ] = isDirectory;
    e[ Permissions ] = fileprops[ 5 ].remove(0, 1);
    e[ CRC ] = fileprops[ 6 ];
    e[ Method ] = fileprops[ 7 ];
    e[ Version ] = fileprops[ 8 ];
    e[ IsPasswordProtected] = m_isPasswordProtected;
    kDebug() << "Added entry: " << e ;

    entry(e);
    m_isFirstLine = true;
    return true;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)

#include "cliplugin.moc"
