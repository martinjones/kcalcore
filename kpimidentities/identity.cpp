/*
    Copyright (c) 2002-2004 Marc Mutz <mutz@kde.org>
    Copyright (c) 2007 Tom Albers <tomalbers@kde.nl>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "identity.h"
#include "signature.h"

#include <kdeversion.h>
#include <sonnet/globals.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kconfiggroup.h>
#include <kurl.h>
#include <kprocess.h>
#include <kpimutils/kfileio.h>
#include <kpimutils/email.h>

#include <QFileInfo>
#include <QMimeData>
#include <QByteArray>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

using namespace KPIMIdentities;

// TODO: should use a kstaticdeleter?
static Identity *identityNull = 0;

Identity::Identity( const QString &id, const QString &fullName,
                    const QString &emailAddr, const QString &organization,
                    const QString &replyToAddr )
  : mIsDefault( false )
{
  setProperty( s_uoid, 0 );
  setProperty( s_identity, id );
  setProperty( s_name, fullName );
  setProperty( s_email, emailAddr );
  setProperty( s_organization, organization );
  setProperty( s_replyto, replyToAddr );
  setDictionary( Sonnet::defaultLanguageName() );
  setProperty( s_disabledFcc, false );
}

Identity::~Identity()
{}

const Identity &Identity::null()
{
  if ( !identityNull ) {
    identityNull = new Identity;
  }
  return *identityNull;
}

bool Identity::isNull() const
{
  bool empty = true;
  QHash<QString, QVariant>::const_iterator i = mPropertiesMap.constBegin();
  while ( i != mPropertiesMap.constEnd() ) {

    // Take into account that the dictionary for a null identity is not empty
    if ( i.key() == s_dict ) {
      ++i;
      continue;
    }
    // Take into account that disableFcc == false for a null identity 
    if( i.key() == s_disabledFcc && i.value().toBool() == false ) {
      ++i;
      continue;
    }
    // The uoid is 0 by default, so ignore this
    if ( !( i.key() == s_uoid && i.value().toUInt() == 0 ) ) {
      if ( !i.value().isNull() ||
          ( i.value().type() == QVariant::String && !i.value().toString().isEmpty() ) ) {
        empty = false;
      }
    }
    ++i;
  }
  return empty;
}

void Identity::readConfig( const KConfigGroup &config )
{
  // get all keys and convert them to our QHash.
  QMap<QString, QString> entries = config.entryMap();
  QMap<QString, QString>::const_iterator i = entries.constBegin();
  QMap<QString, QString>::const_iterator end = entries.constEnd();
  while ( i != end ) {
    if ( i.key() == s_emailAliases ) {
      // HACK: Read s_emailAliases as a stringlist
      mPropertiesMap.insert( i.key(), config.readEntry( i.key(), QStringList() ) );
    } else {
      mPropertiesMap.insert( i.key(), config.readEntry( i.key() ) );
    }
    ++i;
  }
  mSignature.readConfig( config );
}

void Identity::writeConfig( KConfigGroup &config ) const
{
  QHash<QString, QVariant>::const_iterator i = mPropertiesMap.constBegin();
  QHash<QString, QVariant>::const_iterator end = mPropertiesMap.constEnd();
  while ( i != end ) {
    config.writeEntry( i.key(), i.value() );
    kDebug( 5325 ) << "Store:" << i.key() << ":" << i.value();
    ++i;
  }
  mSignature.writeConfig( config );
}

bool Identity::mailingAllowed() const
{
  return !property( s_email ).toString().isEmpty();
}

QString Identity::mimeDataType()
{
  return "application/x-kmail-identity-drag";
}

bool Identity::canDecode( const QMimeData*md )
{
  return md->hasFormat( mimeDataType() );
}

void Identity::populateMimeData( QMimeData*md )
{
  QByteArray a;
  {
    QDataStream s( &a, QIODevice::WriteOnly );
    s << this;
  }
  md->setData( mimeDataType(), a );
}

Identity Identity::fromMimeData( const QMimeData*md )
{
  Identity i;
  if ( canDecode( md ) ) {
    QByteArray ba = md->data( mimeDataType() );
    QDataStream s( &ba, QIODevice::ReadOnly );
    s >> i;
  }
  return i;
}

// ------------------ Operators --------------------------//

QDataStream &KPIMIdentities::operator<<
( QDataStream &stream, const KPIMIdentities::Identity &i )
{
  return stream << static_cast<quint32>( i.uoid() )
         << i.identityName()
         << i.fullName()
         << i.organization()
         << i.pgpSigningKey()
         << i.pgpEncryptionKey()
         << i.smimeSigningKey()
         << i.smimeEncryptionKey()
         << i.primaryEmailAddress()
         << i.emailAliases()
         << i.replyToAddr()
         << i.bcc()
         << i.vCardFile()
         << i.transport()
         << i.fcc()
         << i.drafts()
         << i.templates()
         << i.mPropertiesMap[s_signature]
         << i.dictionary()
         << i.xface()
         << i.preferredCryptoMessageFormat()
         << i.cc()
         << i.attachVcard()
         << i.autocorrectionLanguage()
         << i.disabledFcc();


}

QDataStream &KPIMIdentities::operator>>
( QDataStream &stream, KPIMIdentities::Identity &i )
{
  quint32 uoid;
  stream
  >> uoid
  >> i.mPropertiesMap[s_identity]
  >> i.mPropertiesMap[s_name]
  >> i.mPropertiesMap[s_organization]
  >> i.mPropertiesMap[s_pgps]
  >> i.mPropertiesMap[s_pgpe]
  >> i.mPropertiesMap[s_smimes]
  >> i.mPropertiesMap[s_smimee]
  >> i.mPropertiesMap[s_email]
  >> i.mPropertiesMap[s_emailAliases]
  >> i.mPropertiesMap[s_replyto]
  >> i.mPropertiesMap[s_bcc]
  >> i.mPropertiesMap[s_vcard]
  >> i.mPropertiesMap[s_transport]
  >> i.mPropertiesMap[s_fcc]
  >> i.mPropertiesMap[s_drafts]
  >> i.mPropertiesMap[s_templates]
  >> i.mPropertiesMap[s_signature]
  >> i.mPropertiesMap[s_dict]
  >> i.mPropertiesMap[s_xface]
  >> i.mPropertiesMap[s_prefcrypt]
  >> i.mPropertiesMap[s_cc]
  >> i.mPropertiesMap[s_attachVcard]
  >> i.mPropertiesMap[s_autocorrectionLanguage]
  >> i.mPropertiesMap[s_disabledFcc];

  i.setProperty( s_uoid, uoid );
  return stream;
}

bool Identity::operator< ( const Identity &other ) const
{
  if ( isDefault() ) {
    return true;
  }
  if ( other.isDefault() ) {
    return false;
  }
  return identityName() < other.identityName();
}

bool Identity::operator> ( const Identity &other ) const
{
  if ( isDefault() ) {
    return false;
  }
  if ( other.isDefault() ) {
    return true;
  }
  return identityName() > other.identityName();
}

bool Identity::operator<= ( const Identity &other ) const
{
  return !operator> ( other );
}

bool Identity::operator>= ( const Identity &other ) const
{
  return !operator< ( other );
}

bool Identity::operator== ( const Identity &other ) const
{
  return mPropertiesMap == other.mPropertiesMap &&
         mSignature == other.mSignature;
}

bool Identity::operator!= ( const Identity &other ) const
{
  return !operator== ( other );
}

// --------------------- Getters -----------------------------//

QVariant Identity::property( const QString &key ) const
{
  return mPropertiesMap.value( key );
}

QString Identity::fullEmailAddr( void ) const
{
  const QString name = mPropertiesMap.value( s_name ).toString();
  const QString mail = mPropertiesMap.value( s_email ).toString();

  if ( name.isEmpty() ) {
    return mail;
  }

  const QString specials( "()<>@,.;:[]" );

  QString result;

  // add DQUOTE's if necessary:
  bool needsQuotes=false;
  const int nameLength( name.length() );
  for ( int i=0; i < nameLength; i++ ) {
    if ( specials.contains( name[i] ) ) {
      needsQuotes = true;
    } else if ( name[i] == '\\' || name[i] == '"' ) {
      needsQuotes = true;
      result += '\\';
    }
    result += name[i];
  }

  if ( needsQuotes ) {
    result.insert( 0, '"' );
    result += '"';
  }

  result += " <" + mail + '>';

  return result;
}

QString Identity::identityName() const
{
  return property( QLatin1String( s_identity ) ).toString();
}

QString Identity::signatureText( bool *ok ) const
{
  return mSignature.withSeparator( ok );
}

bool Identity::signatureIsInlinedHtml() const
{
  return mSignature.isInlinedHtml();
}

bool Identity::isDefault() const
{
  return mIsDefault;
}

uint Identity::uoid() const
{
  return property( QLatin1String( s_uoid ) ).toInt();
}

QString Identity::fullName() const
{
  return property( QLatin1String( s_name ) ).toString();
}

QString Identity::organization() const
{
  return property( QLatin1String( s_organization ) ).toString();
}

QByteArray Identity::pgpEncryptionKey() const
{
  return property( QLatin1String( s_pgpe ) ).toByteArray();
}

QByteArray Identity::pgpSigningKey() const
{
  return property( QLatin1String( s_pgps ) ).toByteArray();
}

QByteArray Identity::smimeEncryptionKey() const
{
  return property( QLatin1String( s_smimee ) ).toByteArray();
}

QByteArray Identity::smimeSigningKey() const
{
  return property( QLatin1String( s_smimes ) ).toByteArray();
}

QString Identity::preferredCryptoMessageFormat() const
{
  return property( QLatin1String( s_prefcrypt ) ).toString();
}

QString Identity::emailAddr() const
{
  return primaryEmailAddress();
}

QString Identity::primaryEmailAddress() const
{
  return property( QLatin1String( s_email ) ).toString();
}

const QStringList Identity::emailAliases() const
{
  return property( QLatin1String( s_emailAliases ) ).toStringList();
}

QString Identity::vCardFile() const
{
  return property( QLatin1String( s_vcard ) ).toString();
}

bool Identity::attachVcard() const
{
  return property( QLatin1String( s_attachVcard ) ).toBool();
}

QString Identity::replyToAddr() const
{
  return property( QLatin1String( s_replyto ) ).toString();
}

QString Identity::bcc() const
{
  return property( QLatin1String( s_bcc ) ).toString();
}

QString Identity::cc() const
{
  return property( QLatin1String( s_cc ) ).toString();
}

Signature &Identity::signature()
{
  return mSignature;
}

bool Identity::isXFaceEnabled() const
{
  return property( QLatin1String( s_xfaceenabled ) ).toBool();
}

QString Identity::xface() const
{
  return property( QLatin1String( s_xface ) ).toString();
}

QString Identity::dictionary() const
{
  return property( QLatin1String( s_dict ) ).toString();
}

QString Identity::templates() const
{
  const QString str = property( QLatin1String( s_templates ) ).toString();
  return verifyAkonadiId(str);
}

QString Identity::drafts() const
{
  const QString str = property( QLatin1String( s_drafts ) ).toString();
  return verifyAkonadiId(str);
}

QString Identity::fcc() const
{
  const QString str = property( QLatin1String( s_fcc ) ).toString();
  return verifyAkonadiId(str);
}

QString Identity::transport() const
{
  return property( QLatin1String( s_transport ) ).toString();
}

bool Identity::signatureIsCommand() const
{
  return mSignature.type() == Signature::FromCommand;
}

bool Identity::signatureIsPlainFile() const
{
  return mSignature.type() == Signature::FromFile;
}

bool Identity::signatureIsInline() const
{
  return mSignature.type() == Signature::Inlined;
}

bool Identity::useSignatureFile() const
{
  return signatureIsPlainFile() || signatureIsCommand();
}

QString Identity::signatureInlineText() const
{
  return mSignature.text();
}

QString Identity::signatureFile() const
{
  return mSignature.url();
}

QString Identity::autocorrectionLanguage() const
{
  return property( QLatin1String( s_autocorrectionLanguage ) ).toString();
}

// --------------------- Setters -----------------------------//

void Identity::setProperty( const QString &key, const QVariant &value )
{
  if ( value.isNull() ||
       ( value.type() == QVariant::String && value.toString().isEmpty() ) ) {
    mPropertiesMap.remove( key );
  } else {
    mPropertiesMap.insert( key, value );
  }
}

void Identity::setUoid( uint aUoid )
{
  setProperty( s_uoid, aUoid );
}

void Identity::setIdentityName( const QString &name )
{
  setProperty( s_identity, name );
}

void Identity::setFullName( const QString &str )
{
  setProperty( s_name, str );
}

void Identity::setOrganization( const QString &str )
{
  setProperty( s_organization, str );
}

void Identity::setPGPSigningKey( const QByteArray &str )
{
  setProperty( s_pgps, QString( str ) );
}

void Identity::setPGPEncryptionKey( const QByteArray &str )
{
  setProperty( s_pgpe, QString( str ) );
}

void Identity::setSMIMESigningKey( const QByteArray &str )
{
  setProperty( s_smimes, QString( str ) );
}

void Identity::setSMIMEEncryptionKey( const QByteArray &str )
{
  setProperty( s_smimee, QString( str ) );
}

void Identity::setEmailAddr( const QString &str )
{
  setPrimaryEmailAddress( str );
}

void Identity::setPrimaryEmailAddress( const QString & email )
{
  setProperty( s_email, email );
}

void Identity::setEmailAliases( const QStringList & aliases )
{
  setProperty( s_emailAliases, aliases );
}

void Identity::setVCardFile( const QString &str )
{
  setProperty( s_vcard, str );
}

void Identity::setAttachVcard(bool attachment)
{
  setProperty( s_attachVcard, attachment );
}

void Identity::setReplyToAddr( const QString&str )
{
  setProperty( s_replyto, str );
}

void Identity::setSignatureFile( const QString &str )
{
  mSignature.setUrl( str, signatureIsCommand() );
}

void Identity::setSignatureInlineText( const QString &str )
{
  mSignature.setText( str );
}

void Identity::setTransport( const QString &str )
{
  setProperty( s_transport, str );
}

void Identity::setFcc( const QString &str )
{
  setProperty( s_fcc, str );
}

void Identity::setDrafts( const QString &str )
{
  setProperty( s_drafts, str );
}

void Identity::setTemplates( const QString &str )
{
  setProperty( s_templates, str );
}

void Identity::setDictionary( const QString &str )
{
  setProperty( s_dict, str );
}

void Identity::setBcc( const QString &str )
{
  setProperty( s_bcc, str );
}

void Identity::setCc( const QString &str )
{
  setProperty( s_cc, str );
}

void Identity::setIsDefault( bool flag )
{
  mIsDefault = flag;
}

void Identity::setPreferredCryptoMessageFormat( const QString &str )
{
  setProperty( s_prefcrypt, str );
}

void Identity::setXFace( const QString &str )
{
  QString strNew = str;
  strNew.remove( ' ' );
  strNew.remove( '\n' );
  strNew.remove( '\r' );
  setProperty( s_xface, strNew );
}

void Identity::setXFaceEnabled( const bool on )
{
  setProperty( s_xfaceenabled, on );
}

void Identity::setSignature( const Signature &sig )
{
  mSignature = sig;
}

bool Identity::matchesEmailAddress( const QString & addr ) const
{
  const QString addrSpec = KPIMUtils::extractEmailAddress( addr ).toLower();
  if ( addrSpec == primaryEmailAddress().toLower() ) {
    return true;
  }

  foreach ( const QString &alias, emailAliases() ) {
    if ( alias.toLower() == addrSpec ) {
      return true;
    }
  }

  return false;
}

QString Identity::verifyAkonadiId(const QString& str) const
{
  if(str.isEmpty())
    return str;
  bool ok = false;
  const qlonglong val = str.toLongLong(&ok);
  Q_UNUSED(val);
  if(ok) {
    return str;
  } else {
    return QString();
  }
}

void Identity::setAutocorrectionLanguage(const QString& language)
{
  setProperty( s_autocorrectionLanguage, language );
}


bool Identity::disabledFcc() const
{
  const QVariant var = property(QLatin1String( s_disabledFcc ));
  if(var.isNull()) {
     return false;
  } else {
     return var.toBool();
  }
}

void Identity::setDisabledFcc(bool disable)
{
  setProperty( s_disabledFcc, disable );
}
