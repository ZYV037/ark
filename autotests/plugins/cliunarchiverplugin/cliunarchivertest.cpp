/*
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "cliunarchivertest.h"
#include "jobs.h"
#include "kerfuffle/archiveentry.h"

#include <QDirIterator>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(CliUnarchiverTest)

using namespace Kerfuffle;

void CliUnarchiverTest::initTestCase()
{
    m_plugin = new Plugin(this);
    foreach (Plugin *plugin, m_pluginManger.availablePlugins()) {
        if (plugin->metaData().pluginId() == QStringLiteral("kerfuffle_cliunarchiver")) {
            m_plugin = plugin;
            return;
        }
    }
}

void CliUnarchiverTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");


    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << true << Archive::Unencrypted
            << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/multiple_toplevel_entries.rar");
    QTest::newRow("archive with multiple top-level entries")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << false << Archive::Unencrypted
            << QStringLiteral("multiple_toplevel_entries");

    archivePath = QFINDTESTDATA("data/encrypted_entries.rar");
    QTest::newRow("archive with encrypted entries")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << true << Archive::Encrypted
            << QStringLiteral("A");
}

void CliUnarchiverTest::testArchive()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, m_plugin, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cliunarchiver plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedFileName);
    QCOMPARE(QFileInfo(archive->fileName()).fileName(), expectedFileName);

    QFETCH(bool, isReadOnly);
    QCOMPARE(archive->isReadOnly(), isReadOnly);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolderArchive(), isSingleFolder);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);
}

void CliUnarchiverTest::testList_data()
{
    QTest::addColumn<QString>("jsonFilePath");
    QTest::addColumn<int>("expectedEntriesCount");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<qulonglong>("expectedCompressedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    QTest::newRow("archive with one top-level folder")
            << QFINDTESTDATA("data/one_toplevel_folder.json") << 9
            << 6 << QStringLiteral("A/B/C/") << true << false << (qulonglong) 0 << (qulonglong) 0 << QStringLiteral("2015-12-21 16:57:20 +0100");
    QTest::newRow("archive with multiple top-level entries")
            << QFINDTESTDATA("data/multiple_toplevel_entries.json") << 12
            << 4 << QStringLiteral("data/A/B/test1.txt") << false << false << (qulonglong) 7 << (qulonglong) 7 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("archive with encrypted entries")
            << QFINDTESTDATA("data/encrypted_entries.json") << 9
            << 5 << QStringLiteral("A/test1.txt") << false << true << (qulonglong) 7 << (qulonglong) 32 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("huge archive")
            << QFINDTESTDATA("data/huge_archive.json") << 250
            << 8 << QStringLiteral("PsycOPacK/Base Dictionnaries/att800") << false << false << (qulonglong) 593687 << (qulonglong) 225219 << QStringLiteral("2011-08-14 03:10:10 +0200");
}

void CliUnarchiverTest::testList()
{
    qRegisterMetaType<Archive::Entry*>("Archive::Entry*");
    CliPlugin *unarPlugin = new CliPlugin(this, {QStringLiteral("dummy.rar")});
    QSignalSpy signalSpy(unarPlugin, &CliPlugin::entry);

    QFETCH(QString, jsonFilePath);
    QFETCH(int, expectedEntriesCount);

    QFile jsonFile(jsonFilePath);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));

    QTextStream stream(&jsonFile);
    unarPlugin->setJsonOutput(stream.readAll());

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpy.count());
    Archive::Entry *entry = signalSpy.at(someEntryIndex).at(0).value<Archive::Entry*>();

    QFETCH(QString, expectedName);
    QCOMPARE(entry->property("fullPath").toString(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry->isDir(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry->property("isPasswordProtected").toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry->property("size").toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry->property("compressedSize").toULongLong(), expectedCompressedSize);

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry->property("timestamp").toString(), expectedTimestamp);

    unarPlugin->deleteLater();
}

void CliUnarchiverTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QString()
            << QStringList {
                   QStringLiteral("-json"),
                   QStringLiteral("/tmp/foo.rar")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-json"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("-password"),
                   QStringLiteral("1234")
               };
}

void CliUnarchiverTest::testListArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList listArgs = { QStringLiteral("-json"),
                                   QStringLiteral("$Archive"),
                                   QStringLiteral("$PasswordSwitch") };

    QFETCH(QString, password);
    const auto replacedArgs = plugin->substituteListVariables(listArgs, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliUnarchiverTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QList<Archive::Entry*>>("entriesToExtract");
    QTest::addColumn<ExtractionOptions>("extractionOptions");
    QTest::addColumn<int>("expectedExtractedEntriesCount");

    ExtractionOptions options;
    options[QStringLiteral("AlwaysUseTmpDir")] = true;

    ExtractionOptions optionsPreservePaths = options;
    optionsPreservePaths[QStringLiteral("PreservePaths")] = true;

    ExtractionOptions dragAndDropOptions = optionsPreservePaths;
    dragAndDropOptions[QStringLiteral("DragAndDrop")] = true;

    QTest::newRow("extract the whole multiple_toplevel_entries.rar")
            << QFINDTESTDATA("data/multiple_toplevel_entries.rar")
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 12;

    QTest::newRow("extract selected entries from a rar, without paths")
            << QFINDTESTDATA("data/one_toplevel_folder.rar")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << options
            << 2;

    QTest::newRow("extract selected entries from a rar, preserve paths")
            << QFINDTESTDATA("data/one_toplevel_folder.rar")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << optionsPreservePaths
            << 4;

    QTest::newRow("extract selected entries from a rar, drag-and-drop")
            << QFINDTESTDATA("data/one_toplevel_folder.rar")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/B/C/"), QStringLiteral("A/B/")),
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A/")),
                   new Archive::Entry(this, QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/")),
                   new Archive::Entry(this, QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/"))
               }
            << dragAndDropOptions
            << 4;

    QTest::newRow("rar with empty folders")
            << QFINDTESTDATA("data/empty_folders.rar")
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 5;
}

// TODO: we can remove this test (which is duplicated from kerfuffle/archivetest)
// if we ever ends up using a temp dir for any cliplugin, instead of only for cliunarchiver.
void CliUnarchiverTest::testExtraction()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, m_plugin, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cliunarchiver plugin. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry*>, entriesToExtract);
    QFETCH(ExtractionOptions, extractionOptions);
    auto extractionJob = archive->copyFiles(entriesToExtract, destDir.path(), extractionOptions);

    QEventLoop eventLoop(this);
    connect(extractionJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    extractionJob->start();
    eventLoop.exec(); // krazy:exclude=crashy

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);

    archive->deleteLater();
}

void CliUnarchiverTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QList<Archive::Entry*>>("files");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("encrypted, multiple files")
            << QStringLiteral("/tmp/foo.rar")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-D"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
                   QStringLiteral("-password"),
                   QStringLiteral("1234")
               };

    QTest::newRow("unencrypted, multiple files")
            << QStringLiteral("/tmp/foo.rar")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << QString()
            << QStringList {
                   QStringLiteral("-D"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };
}

void CliUnarchiverTest::testExtractArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList extractArgs = { QStringLiteral("-D"),
                                      QStringLiteral("$Archive"),
                                      QStringLiteral("$Files"),
                                      QStringLiteral("$PasswordSwitch") };

    QFETCH(QList<Archive::Entry*>, files);
    QFETCH(QString, password);

    QStringList replacedArgs = plugin->substituteCopyVariables(extractArgs, files, false, password);
    QVERIFY(replacedArgs.size() >= extractArgs.size());

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}
