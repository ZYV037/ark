/*


 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// Qt includes
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qstring.h>

// KDE includes
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdir.h>
#include <kdirlistbox.h>
#include <klocale.h>
#include <kfileinfo.h>
#include <kfilefilter.h>
#include <ktoolbar.h>
#include <kmessagebox.h>

// ark includes
#include "zipExtractDlg.h"
#include "zipExtractDlg.moc"


ZipExtractDlg::ZipExtractDlg( ArkData *_d, bool _selection, QString _dir, QWidget *_parent, const char *_name )
	: KFileBaseDialog( _dir, QString::null, _parent, _name, true, false )
{
	m_data = _d;
	m_selection = _selection;
	boxLayout = 0;
	lafBox = 0;
	init();	
}

void ZipExtractDlg::initGUI()
{
	kdebug(0, 1601, "+initGUI");

	setCaption( i18n("Extract to...") );
	
	QVBoxLayout *mainLayout = new QVBoxLayout(this, 5);

        toolbar->setItemEnabled(1009, false);

	mainLayout->addWidget(toolbar);
        mainLayout->addWidget(fileList->widget(), 4);
        mainLayout->addSpacing(3);

	mainLayout->addSpacing( 10 );

	QHBoxLayout *hbl1 = new QHBoxLayout();
	mainLayout->addLayout( hbl1 );

	locationLabel->setText( i18n("Extract to: ") );
	locationLabel->setFixedSize( locationLabel->sizeHint() );
	hbl1->addWidget( locationLabel, 0, AlignTop );
	locationEdit->setFixedHeight( locationEdit->sizeHint().height() );
	locationEdit->setMinimumWidth( locationEdit->sizeHint().width() );
	hbl1->addWidget( locationEdit, 0, AlignTop );

	QVBoxLayout *vbl1 = new QVBoxLayout();
	hbl1->addLayout( vbl1 );
	QPushButton *bExtract = new QPushButton( i18n("Extract"), this );
	bExtract->setFixedSize( bExtract->sizeHint() );
	connect(bExtract, SIGNAL(clicked()), SLOT(onExtract()));
	vbl1->addWidget( bExtract );
	locationLabel->setFixedHeight( bExtract->sizeHint().height() );
	locationEdit->setFixedHeight( bExtract->sizeHint().height() );

	QPushButton *cancel = new QPushButton( i18n("Cancel"), this );
	cancel->setFixedSize( cancel->sizeHint() );
	vbl1->addWidget( cancel );
        connect(cancel, SIGNAL(clicked()), SLOT(reject()));

	// *******
	// *@@*  *
	// *******
	
	QHBoxLayout *hbl2 = new QHBoxLayout();
	mainLayout->addLayout( hbl2 );
	
	QButtonGroup *bg1 = new QButtonGroup( i18n("Files"), this );
	hbl2->addWidget( bg1 );
	
	QVBoxLayout *vblg1 = new QVBoxLayout( bg1, 10 );
	vblg1->addSpacing( 10 );
	
	r1 = new QRadioButton( i18n("Selected files"), bg1 );
	r1->setFixedSize( r1->sizeHint() );
	r1->setEnabled( m_selection );
	vblg1->addWidget( r1, 0, AlignLeft );
	
	r2 = new QRadioButton( i18n("All files"), bg1 );
	r2->setFixedSize( r2->sizeHint() );
	r2->setChecked( true );
	vblg1->addWidget( r2, 0, AlignLeft );
	
	r3 = new QRadioButton( i18n("Files: "), bg1 );
	r3->setFixedSize( r3->sizeHint() );
	QHBoxLayout *hblg1 = new QHBoxLayout();
	vblg1->addLayout( hblg1 );
	
	hblg1->addWidget( r3 );
	
	le1 = new QLineEdit( bg1 );
	le1->setMinimumSize( le1->sizeHint() );
	hblg1->addWidget( le1 );
	connect( le1, SIGNAL(textChanged(const QString &)), SLOT(onPatternChanged(const QString&)));

	// *******
	// *  *@@*
	// *******
	
	QButtonGroup *bg2 = new QButtonGroup( i18n("Options"), this );
	hbl2->addWidget( bg2 );
	
	QVBoxLayout *vblg2 = new QVBoxLayout( bg2, 10 );
	vblg2->addSpacing( 10 );
	
	r4 = new QCheckBox( i18n("Overwrite existing files"), bg2 );
	r4->setFixedSize( r4->sizeHint() );
	r4->setChecked( m_data->getZipExtractOverwrite() );
	vblg2->addWidget( r4, 0, AlignLeft );
	
	r5 = new QCheckBox( i18n("Junk paths"), bg2 );
	r5->setFixedSize( r5->sizeHint() );
	r5->setChecked( m_data->getZipExtractJunkPaths() );
	vblg2->addWidget( r5, 0, AlignLeft );
	
	r6 = new QCheckBox( i18n("Make (some) names lowercase"), bg2 );
	r6->setFixedSize( r6->sizeHint() );
	r6->setChecked( m_data->getZipExtractLowerCase() );
	vblg2->addWidget( r6, 0, AlignLeft );
	
	if ( myStatusLine )
		mainLayout->addWidget( myStatusLine, 0 );

	bOk->hide();
	bCancel->hide();

	mainLayout->activate();

	resize( minimumSize() );

	fileList->connectDirSelected(this, SLOT(dirActivated(KFileInfo*)));
	fileList->connectFileSelected(this, SLOT(fileActivated(KFileInfo*)));
	fileList->connectFileHighlighted(this, SLOT(fileHighlighted(KFileInfo*)));

	kdebug(0, 1601, "-initGUI");
}

bool ZipExtractDlg::getShowFilter()
{
	return false;
}

KFileInfoContents* ZipExtractDlg::initFileList( QWidget *parent )
{
	bool useSingleClick = kapp->config()->readBoolEntry("SingleClick", true);
	return new KDirListBox( useSingleClick, dir->sorting(), parent, "_dirs" );
}

bool ZipExtractDlg::lowerCase()
{
	return r6->isChecked();
}

bool ZipExtractDlg::overwrite()
{
	return r4->isChecked();
}

bool ZipExtractDlg::junkPaths()
{
	return r5->isChecked();
}

int ZipExtractDlg::selection()
{
	if( r1->isChecked() )
		return Selection;
	else if(r2->isChecked() )
		return All;
	else
		return Pattern;
}

QString ZipExtractDlg::pattern()
{
	return le1->text();
}

void ZipExtractDlg::onExtract()
{
	QString dest = locationEdit->currentText();
	
	if( dest.isEmpty() ){
		KMessageBox::sorry(this, i18n("Destination is empty"));
	}
	else{
		kdebug(0, 1601, "ZipExtractDlg::onExtract: Destination is %s", dest.ascii());
		saveConfig();
		accept();	
	}
}

void ZipExtractDlg::onPatternChanged(const QString &)
{
	r3->setChecked( true );	
}

QString ZipExtractDlg::getDestination() const
{
	return locationEdit->currentText();
}

void ZipExtractDlg::saveConfig()
{
	m_data->setZipExtractOverwrite( r4->isChecked() );	
	m_data->setZipExtractJunkPaths( r5->isChecked() );	
	m_data->setZipExtractLowerCase( r6->isChecked() );	
}

