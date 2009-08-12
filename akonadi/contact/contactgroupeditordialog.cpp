/*
    This file is part of Akonadi Contact.

    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "contactgroupeditordialog.h"

#include "collectioncombobox.h"
#include "collectionfiltermodel_p.h"
#include "contactgroupeditor.h"

#include <akonadi/descendantsproxymodel.h>
#include <akonadi/item.h>
#include <kabc/contactgroup.h>
#include <klocale.h>

#include <QtGui/QGridLayout>
#include <QtGui/QLabel>

using namespace Akonadi;

class ContactGroupEditorDialog::Private
{
  public:
    ContactGroupEditor *mEditor;
};

ContactGroupEditorDialog::ContactGroupEditorDialog( Mode mode, QAbstractItemModel *collectionModel, QWidget *parent )
  : KDialog( parent ), d( new Private )
{
  setCaption( mode == CreateMode ? i18n( "New Contact Group" ) : i18n( "Edit Contact Group" ) );
  setButtons( Ok | Cancel );

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QGridLayout *layout = new QGridLayout( mainWidget );

  d->mEditor = new Akonadi::ContactGroupEditor( mode == CreateMode ?
                                                Akonadi::ContactGroupEditor::CreateMode : Akonadi::ContactGroupEditor::EditMode,
                                                this );

  if ( mode == CreateMode ) {
    QLabel *label = new QLabel( i18n( "Add to:" ), mainWidget );

    // flatten the collection tree structure to a collection list
    Akonadi::DescendantsProxyModel *descendantModel = new Akonadi::DescendantsProxyModel( this );
    descendantModel->setSourceModel( collectionModel );

    // filter for collections that support contact groups
    CollectionFilterModel *filterModel = new CollectionFilterModel( this );
    filterModel->addContentMimeTypeFilter( KABC::ContactGroup::mimeType() );
    filterModel->setRightsFilter( Akonadi::Collection::CanCreateItem );
    filterModel->setSourceModel( descendantModel );

    CollectionComboBox *box = new CollectionComboBox( mainWidget );
    box->setModel( filterModel );

    layout->addWidget( label, 0, 0 );
    layout->addWidget( box, 0, 1 );

    connect( box, SIGNAL( selectionChanged( const Akonadi::Collection& ) ),
             d->mEditor, SLOT( setDefaultCollection( const Akonadi::Collection& ) ) );

    d->mEditor->setDefaultCollection( box->selectedCollection() );
  }

  layout->addWidget( d->mEditor, 1, 0, 1, 2 );
  layout->setColumnStretch( 1, 1 );

  connect( d->mEditor, SIGNAL( contactGroupStored( const Akonadi::Item& ) ),
           this, SIGNAL( contactGroupStored( const Akonadi::Item& ) ) );

  setInitialSize( QSize( 420, 120 ) );
}

ContactGroupEditorDialog::~ContactGroupEditorDialog()
{
  delete d;
}

void ContactGroupEditorDialog::setCompletionModel( QAbstractItemModel *model )
{
  d->mEditor->setCompletionModel( model );
}

void ContactGroupEditorDialog::setContactGroup( const Akonadi::Item &group )
{
  d->mEditor->loadContactGroup( group );
}

void ContactGroupEditorDialog::slotButtonClicked( int button )
{
  if ( button == KDialog::Ok ) {
    if ( d->mEditor->saveContactGroup() )
      accept();
  } else if ( button == KDialog::Cancel ) {
    reject();
  }
}

#include "contactgroupeditordialog.moc"