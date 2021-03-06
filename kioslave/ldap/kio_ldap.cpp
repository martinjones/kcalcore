/*
  Copyright (c) 2004-2007 Szombathelyi György <gyurco@freemail.hu>
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "kio_ldap.h"

#include <kldap/ldif.h>
#include <kldap/ldapcontrol.h>
#include <kldap/ldapdefs.h>

#include <kdebug.h>
#include <kcomponentdata.h>
#include <klocalizedstring.h>

#include <netdb.h>
#include <netinet/in.h>

using namespace KIO;
using namespace KLDAP;

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

/**
 * The main program.
 */
int kdemain( int argc, char **argv )
{
  KComponentData componentData( "kio_ldap" );

  kDebug(7125) << "Starting kio_ldap instance";

  if ( argc != 4 ) {
    kError() << "Usage kio_ldap protocol pool app" << endl;
    return -1;
  }

  // let the protocol class do its work
  LDAPProtocol slave( argv[1], argv[ 2 ], argv[ 3 ] );
  slave.dispatchLoop();

  kDebug( 7125 ) << "Done";
  return 0;
}

/**
 * Initialize the ldap slave
 */
LDAPProtocol::LDAPProtocol( const QByteArray &protocol, const QByteArray &pool,
  const QByteArray &app ) : SlaveBase( protocol, pool, app )
{
  mConnected = false;
  mOp.setConnection( mConn );
  kDebug(7125) << "LDAPProtocol::LDAPProtocol (" << protocol << ")";
}

LDAPProtocol::~LDAPProtocol()
{
  closeConnection();
}

void LDAPProtocol::LDAPErr( int err )
{

  QString extramsg;
  if ( mConnected ) {
    if ( err == KLDAP_SUCCESS ) err = mConn.ldapErrorCode();
    if ( err != KLDAP_SUCCESS ) extramsg = i18n("\nAdditional info: ") + mConn.ldapErrorString();
  }
  if ( err == KLDAP_SUCCESS ) return;

  kDebug() << "error code: " << err << " msg: " << LdapConnection::errorString(err) <<
    extramsg << "'" << endl;
  QString msg;
  msg = mServer.url().prettyUrl();
  if ( !extramsg.isEmpty() ) msg += extramsg;

  /* FIXME: No need to close on all errors */
  closeConnection();

  switch (err) {
/* FIXME: is it worth mapping the following error codes to kio errors?

	LDAP_OPERATIONS_ERROR
  LDAP_STRONG_AUTH_REQUIRED
	LDAP_PROTOCOL_ERROR
	LDAP_TIMELIMIT_EXCEEDED
	LDAP_SIZELIMIT_EXCEEDED
	LDAP_COMPARE_FALSE
	LDAP_COMPARE_TRUE
	LDAP_PARTIAL_RESULTS
	LDAP_NO_SUCH_ATTRIBUTE
	LDAP_UNDEFINED_TYPE
	LDAP_INAPPROPRIATE_MATCHING
	LDAP_CONSTRAINT_VIOLATION
	LDAP_INVALID_SYNTAX
	LDAP_NO_SUCH_OBJECT
	LDAP_ALIAS_PROBLEM
	LDAP_INVALID_DN_SYNTAX
	LDAP_IS_LEAF
	LDAP_ALIAS_DEREF_PROBLEM
	LDAP_INAPPROPRIATE_AUTH
	LDAP_BUSY
	LDAP_UNAVAILABLE
	LDAP_UNWILLING_TO_PERFORM
	LDAP_LOOP_DETECT
	LDAP_NAMING_VIOLATION
	LDAP_OBJECT_CLASS_VIOLATION
	LDAP_NOT_ALLOWED_ON_NONLEAF
	LDAP_NOT_ALLOWED_ON_RDN
	LDAP_NO_OBJECT_CLASS_MODS
	LDAP_OTHER
	LDAP_LOCAL_ERROR
	LDAP_ENCODING_ERROR
	LDAP_DECODING_ERROR
	LDAP_FILTER_ERROR
*/
    case KLDAP_AUTH_UNKNOWN:
    case KLDAP_INVALID_CREDENTIALS:
    case KLDAP_STRONG_AUTH_NOT_SUPPORTED:
      error(ERR_COULD_NOT_AUTHENTICATE, msg);
      break;
    case KLDAP_ALREADY_EXISTS:
      error(ERR_FILE_ALREADY_EXIST, msg);
      break;
    case KLDAP_INSUFFICIENT_ACCESS:
      error(ERR_ACCESS_DENIED, msg);
      break;
    case KLDAP_CONNECT_ERROR:
    case KLDAP_SERVER_DOWN:
      error(ERR_COULD_NOT_CONNECT,msg);
      break;
    case KLDAP_TIMEOUT:
      error(ERR_SERVER_TIMEOUT,msg);
      break;
    case KLDAP_PARAM_ERROR:
      error(ERR_INTERNAL,msg);
      break;
    case KLDAP_NO_MEMORY:
      error(ERR_OUT_OF_MEMORY,msg);
      break;

    default:
      error( ERR_SLAVE_DEFINED,
        i18n( "LDAP server returned the error: %1 %2\nThe LDAP URL was: %3" ,
         LdapConnection::errorString(err), extramsg, mServer.url().prettyUrl() ) );
  }
}

void LDAPProtocol::controlsFromMetaData( LdapControls &serverctrls,
  LdapControls &clientctrls )
{
  QString oid; bool critical; QByteArray value;
  int i = 0;
  while ( hasMetaData( QString::fromLatin1("SERVER_CTRL%1").arg(i) ) ) {
    QByteArray val = metaData( QString::fromLatin1("SERVER_CTRL%1").arg(i) ).toUtf8();
    Ldif::splitControl( val, oid, critical, value );
    kDebug(7125) << "server ctrl #" << i << " value: " << val <<
      " oid: " << oid << " critical: " << critical << " value: " <<
      QString::fromUtf8( value, value.size() ) << endl;
    LdapControl ctrl( oid, val, critical );
    serverctrls.append( ctrl );
    i++;
  }
  i = 0;
  while ( hasMetaData( QString::fromLatin1("CLIENT_CTRL%1").arg(i) ) ) {
    QByteArray val = metaData( QString::fromLatin1("CLIENT_CTRL%1").arg(i) ).toUtf8();
    Ldif::splitControl( val, oid, critical, value );
    kDebug(7125) << "client ctrl #" << i << " value: " << val <<
      " oid: " << oid << " critical: " << critical << " value: " <<
      QString::fromUtf8( value, value.size() ) << endl;
    LdapControl ctrl( oid, val, critical );
    clientctrls.append( ctrl );
    i++;
  }

}

void LDAPProtocol::LDAPEntry2UDSEntry( const LdapDN &dn, UDSEntry &entry,
  const LdapUrl &usrc, bool dir )
{
  int pos;
  entry.clear();
  QString name = dn.toString();
  if ( (pos = name.indexOf(',')) > 0 )
    name = name.left( pos );
  if ( (pos = name.indexOf('=')) > 0 )
    name.remove( 0, pos+1 );
  name.replace(' ', "_");
  if ( !dir ) name += ".ldif";
    entry.insert( KIO::UDSEntry::UDS_NAME, name );

  // the file type
    entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, dir ? S_IFDIR : S_IFREG );

  // the mimetype
  if (!dir) {
    entry.insert( KIO::UDSEntry::UDS_MIME_TYPE, QString::fromLatin1("text/plain") );
  }

    entry.insert( KIO::UDSEntry::UDS_ACCESS, dir ? 0500 : 0400 );

  // the url
  LdapUrl url=usrc;
  url.setPath('/'+dn.toString());
  url.setScope( dir ? LdapUrl::One : LdapUrl::Base );
    entry.insert( KIO::UDSEntry::UDS_URL, url.prettyUrl() );
}


void LDAPProtocol::changeCheck( LdapUrl &url )
{
  LdapServer server;
  server.setUrl( url );

  if ( mConnected ) {
    if ( server.host() != mServer.host() ||
       server.port() != mServer.port() ||
       server.baseDn() != mServer.baseDn() ||
       server.user() != mServer.user() ||
       server.bindDn() != mServer.bindDn() ||
       server.realm() != mServer.realm() ||
       server.password() != mServer.password() ||
       server.timeLimit() != mServer.timeLimit() ||
       server.sizeLimit() != mServer.sizeLimit() ||
       server.version() != mServer.version() ||
       server.security() != mServer.security() ||
       server.auth() != mServer.auth() ||
       server.mech() != mServer.mech() ) {

      closeConnection();
      mServer = server;
      openConnection();
    }
  } else {
    mServer = server;
    openConnection();
  }
}

void LDAPProtocol::setHost( const QString& host, quint16 port,
                            const QString& user, const QString& password )
{
  if( mServer.host() != host ||
      mServer.port() != port ||
      mServer.user() != user ||
      mServer.password() != password )
    closeConnection();

  mServer.host() = host;
  if( port > 0 )
    mServer.setPort( port );
  else {
    struct servent *pse;
    if ( (pse = getservbyname(mProtocol, "tcp")) == NULL )
      if ( mProtocol == "ldaps" )
        mServer.setPort( 636 );
      else
        mServer.setPort( 389 );
    else
      mServer.setPort( ntohs( pse->s_port ) );
  }
  mServer.setUser( user );
  mServer.setPassword( password );

  kDebug(7125) << "setHost: " << host << " port: " << port << " user: " <<
    user << " pass: [protected]" << endl;
}

void LDAPProtocol::openConnection()
{
  if ( mConnected ) return;

  mConn.setServer( mServer );
  if ( mConn.connect() != 0 ) {
    error(ERR_COULD_NOT_CONNECT,mConn.connectionError() );
    return;
  }

  mConnected = true;

  AuthInfo info;
  info.url.setProtocol( mProtocol );
  info.url.setHost( mServer.host() );
  info.url.setPort( mServer.port() );
  info.url.setUser( mServer.user() );
  info.caption = i18n("LDAP Login");
  info.comment = QString::fromLatin1( mProtocol ) + "://" + mServer.host() + ':' +
    QString::number( mServer.port() );
  info.commentLabel = i18n("site:");
  info.username = mServer.auth() == LdapServer::SASL ? mServer.user() : mServer.bindDn();
  info.password = mServer.password();
  info.keepPassword = true;
  bool cached = checkCachedAuthentication( info );

  bool firstauth = true;
  int retval;

  while ( true ) {
    retval = mOp.bind_s();
    if ( retval == 0 ) {
      kDebug(7125) << "connected!";
      connected();
      return;
    }
    if ( retval == KLDAP_INVALID_CREDENTIALS ||
         retval == KLDAP_INSUFFICIENT_ACCESS ||
         retval == KLDAP_INAPPROPRIATE_AUTH  ||
         retval == KLDAP_UNWILLING_TO_PERFORM ) {

      if ( firstauth && cached ) {
        if ( mServer.auth() == LdapServer::SASL ) {
          mServer.setUser( info.username );
        } else {
          mServer.setBindDn( info.username );
        }
        mServer.setPassword( info.password );
        mConn.setServer( mServer );
        cached = false;
      } else {
        bool ok = firstauth ?
           openPasswordDialog( info ) :
           openPasswordDialog( info, i18n("Invalid authorization information.") );
        if ( !ok ) {
          error( ERR_USER_CANCELED, i18n("LDAP connection canceled.") );
          closeConnection();
          return;
        }
        if ( mServer.auth() == LdapServer::SASL ) {
          mServer.setUser( info.username );
        } else {
          mServer.setBindDn( info.username );
        }
        mServer.setPassword( info.password );
        firstauth = false;
        mConn.setServer( mServer );
      }

    } else {
      LDAPErr( retval );
      closeConnection();
      return;
    }
  }
}

void LDAPProtocol::closeConnection()
{
  if ( mConnected ) mConn.close();
  mConnected = false;

  kDebug(7125) << "connection closed!";
}

/**
 * Get the information contained in the URL.
 */
void LDAPProtocol::get( const KUrl &_url )
{
  kDebug(7125) << "get(" << _url << ")";

  LdapUrl usrc(_url);
  int ret, id;

  changeCheck( usrc );
  if ( !mConnected ) {
    finished();
    return;
  }

  LdapControls serverctrls, clientctrls;
  controlsFromMetaData( serverctrls, clientctrls );
  if ( mServer.pageSize() ) {
    LdapControls ctrls = serverctrls;
    ctrls.append( LdapControl::createPageControl( mServer.pageSize() ) );
    kDebug(7125) << "page size: " << mServer.pageSize();
    mOp.setServerControls( ctrls );
  } else {
    mOp.setServerControls( serverctrls );
  }
  mOp.setClientControls( clientctrls );

  if ( (id = mOp.search( usrc.dn(), usrc.scope(), usrc.filter(), usrc.attributes() )) == -1 ) {
    LDAPErr();
    return;
  }

  // tell the mimetype
  mimeType("text/plain");
  // collect the result
  QByteArray result;
  filesize_t processed_size = 0;

  while( true ) {
    ret = mOp.waitForResult( id, -1 );
    if ( ret == -1 || mConn.ldapErrorCode() != KLDAP_SUCCESS ) {
      LDAPErr();
      return;
    }
    kDebug(7125) << " ldap_result: " << ret;
    if ( ret == LdapOperation::RES_SEARCH_RESULT ) {

      if ( mServer.pageSize() ) {
        QByteArray cookie;
        int estsize = -1;
        for ( int i = 0; i < mOp.controls().count(); ++i ) {
          kDebug(7125) << " control oid: " << mOp.controls()[i].oid();
          estsize = mOp.controls()[i].parsePageControl( cookie );
          if ( estsize != -1 ) break;
        }
        kDebug(7125) << " estimated size: " << estsize;
        if ( estsize != -1 && !cookie.isEmpty() ) {
          LdapControls ctrls;
          ctrls = serverctrls;
          kDebug(7125) << "page size: " << mServer.pageSize() << " estimated size: " << estsize;
          ctrls.append( LdapControl::createPageControl( mServer.pageSize(), cookie ) );
          mOp.setServerControls( ctrls );
          if ( (id = mOp.search( usrc.dn(), usrc.scope(), usrc.filter(), usrc.attributes() )) == -1 ) {
            LDAPErr();
            return;
          }
          continue;
        }
      }
      break;
    }
    if ( ret != LdapOperation::RES_SEARCH_ENTRY ) continue;

    QByteArray entry = mOp.object().toString().toUtf8() + '\n';
    processed_size += entry.size();
    data(entry);
    processedSize( processed_size );
  }

  totalSize(processed_size);

  // tell we are finished
  data( QByteArray() );
  finished();
}

/**
 * Test if the url contains a directory or a file.
 */
void LDAPProtocol::stat( const KUrl &_url )
{
  kDebug(7125) << "stat(" << _url << ")";

  QStringList att,saveatt;
  LdapUrl usrc(_url);
  int ret, id;

  changeCheck( usrc );
  if ( !mConnected ) {
    finished();
    return;
  }

  // look how many entries match
  saveatt = usrc.attributes();
  att.append( "dn" );

  if ( (id = mOp.search( usrc.dn(), usrc.scope(), usrc.filter(), att )) == -1 ) {
    LDAPErr();
    return;
  }

  kDebug(7125) << "stat() getting result";
  do {
    ret = mOp.waitForResult( id, -1 );
    if ( ret == -1 || mConn.ldapErrorCode() != KLDAP_SUCCESS ) {
      LDAPErr();
      return;
    }
    if ( ret == LdapOperation::RES_SEARCH_RESULT ) {
      error( ERR_DOES_NOT_EXIST, _url.prettyUrl() );
      return;
    }
  } while ( ret != LdapOperation::RES_SEARCH_ENTRY );

  mOp.abandon( id );

  usrc.setAttributes( saveatt );

  UDSEntry uds;
  bool critical;
  LDAPEntry2UDSEntry( usrc.dn(), uds, usrc, usrc.extension("x-dir", critical) != "base" );

  statEntry( uds );
  // we are done
  finished();
}

/**
 * Deletes one entry;
 */
void LDAPProtocol::del( const KUrl &_url, bool )
{
  kDebug(7125) << "del(" << _url << ")";

  LdapUrl usrc(_url);
  int id, ret;

  changeCheck( usrc );
  if ( !mConnected ) {
    finished();
    return;
  }

  LdapControls serverctrls, clientctrls;
  controlsFromMetaData( serverctrls, clientctrls );
  mOp.setServerControls( serverctrls );
  mOp.setClientControls( clientctrls );

  kDebug(7125) << " del: " << usrc.dn().toString().toUtf8();

  if ( (id = mOp.del( usrc.dn() ) == -1) ) {
    LDAPErr();
    return;
  }
  ret = mOp.waitForResult( id, -1 );
  if ( ret == -1 || mConn.ldapErrorCode() != KLDAP_SUCCESS ) {
    LDAPErr();
    return;
  }

  finished();
}

void LDAPProtocol::put( const KUrl &_url, int, KIO::JobFlags flags )
{
  kDebug(7125) << "put(" << _url << ")";

  LdapUrl usrc(_url);

  changeCheck( usrc );
  if ( !mConnected ) {
    finished();
    return;
  }

  LdapControls serverctrls, clientctrls;
  controlsFromMetaData( serverctrls, clientctrls );
  mOp.setServerControls( serverctrls );
  mOp.setClientControls( clientctrls );

  LdapObject addObject;
  LdapOperation::ModOps modops;
  QByteArray buffer;
  int result = 0;
  Ldif::ParseValue ret;
  Ldif ldif;
  ret = Ldif::MoreData;
  int ldaperr;


  do {
    if ( ret == Ldif::MoreData ) {
      dataReq(); // Request for data
      result = readData( buffer );
      ldif.setLdif( buffer );
    }
    if ( result < 0 ) {
      //error
      return;
    }
    if ( result == 0 ) {
      kDebug(7125) << "EOF!";
      ldif.endLdif();
    }
    do {

      ret = ldif.nextItem();
      kDebug(7125) << "nextitem: " << ret;

      switch ( ret ) {
        case Ldif::None:
        case Ldif::NewEntry:
        case Ldif::MoreData:
          break;
        case Ldif::EndEntry:
          ldaperr = KLDAP_SUCCESS;
          switch ( ldif.entryType() ) {
            case Ldif::Entry_None:
              error( ERR_INTERNAL, i18n("The Ldif parser failed.") );
              return;
            case Ldif::Entry_Del:
              kDebug(7125) << "kio_ldap_del";
              ldaperr = mOp.del_s( ldif.dn() );
              break;
            case Ldif::Entry_Modrdn:
              kDebug(7125) << "kio_ldap_modrdn olddn:" << ldif.dn().toString() <<
                " newRdn: " <<  ldif.newRdn() <<
                " newSuperior: " << ldif.newSuperior() <<
                " deloldrdn: " << ldif.delOldRdn() << endl;
              ldaperr = mOp.rename_s( ldif.dn(), ldif.newRdn(),
                ldif.newSuperior(), ldif.delOldRdn() );
              break;
            case Ldif::Entry_Mod:
              kDebug(7125) << "kio_ldap_mod" ;
              ldaperr = mOp.modify_s( ldif.dn(), modops );
              modops.clear();
              break;
            case Ldif::Entry_Add:
              kDebug(7125) << "kio_ldap_add " << ldif.dn().toString();
              addObject.setDn( ldif.dn() );
              ldaperr = mOp.add_s(  addObject );
              if ( ldaperr == KLDAP_ALREADY_EXISTS && (flags & KIO::Overwrite) ) {
                kDebug(7125) << ldif.dn().toString() << " already exists, delete first";
                ldaperr = mOp.del_s( ldif.dn() );
                if ( ldaperr == KLDAP_SUCCESS )
                  ldaperr = mOp.add_s( addObject );
              }
              addObject.clear();
              break;
          }
          if ( ldaperr != KLDAP_SUCCESS ) {
            kDebug(7125) << "put ldap error: " << ldaperr;
            LDAPErr( ldaperr );
            return;
          }
          break;
        case Ldif::Item:
          switch ( ldif.entryType() ) {
            case Ldif::Entry_Mod: {
              LdapOperation::ModOp op;
              op.type = LdapOperation::Mod_None;
              switch ( ldif.modType() ) {
                case Ldif::Mod_None:
                  op.type = LdapOperation::Mod_None;
                  break;
                case Ldif::Mod_Add:
                  op.type = LdapOperation::Mod_Add;
                  break;
                case Ldif::Mod_Replace:
                  op.type = LdapOperation::Mod_Replace;
                  break;
                case Ldif::Mod_Del:
                  op.type = LdapOperation::Mod_Del;
                  break;
              }
              op.attr = ldif.attr();
              if ( !ldif.value().isNull() )
                op.values.append( ldif.value() );
              modops.append( op );
              break;
            }
            case Ldif::Entry_Add:
              if ( ldif.value().size() > 0 )
                addObject.addValue( ldif.attr(), ldif.value() );
              break;
            default:
              error( ERR_INTERNAL, i18n("The Ldif parser failed.") );
              return;
          }
          break;
        case Ldif::Control: {
          LdapControl control;
          control.setControl( ldif.oid(), ldif.value(), ldif.isCritical() );
          serverctrls.append( control );
          mOp.setServerControls( serverctrls );
          break;
        }
        case Ldif::Err:
          error( ERR_SLAVE_DEFINED,
            i18n( "Invalid Ldif file in line %1.", ldif.lineNumber() ) );
          return;
      }
    } while ( ret != Ldif::MoreData );
  } while ( result > 0 );

  finished();
}

/**
 * List the contents of a directory.
 */
void LDAPProtocol::listDir( const KUrl &_url )
{
  int ret, ret2, id, id2;
  unsigned long total=0;
  QStringList att,saveatt;
  LdapUrl usrc(_url),usrc2;
  bool critical = true;
  bool isSub = ( usrc.extension( "x-dir", critical ) == "sub" );

  kDebug(7125) << "listDir(" << _url << ")";

  changeCheck( usrc );
  if ( !mConnected ) {
    finished();
    return;
  }
  usrc2 = usrc;

  saveatt = usrc.attributes();
  // look up the entries
  if ( isSub ) {
    att.append("dn");
    usrc.setAttributes(att);
  }
  if ( _url.query().isEmpty() ) usrc.setScope( LdapUrl::One );

  if ( (id = mOp.search( usrc.dn(), usrc.scope(), usrc.filter(), usrc.attributes() )) == -1 ) {
    LDAPErr();
    return;
  }

  usrc.setAttributes( QStringList() << "" );
  usrc.setExtension( "x-dir", "base" );
  // publish the results
  UDSEntry uds;

  while( true ) {
    ret = mOp.waitForResult( id, -1 );
    if ( ret == -1 || mConn.ldapErrorCode() != KLDAP_SUCCESS ) {
      LDAPErr();
      return;
    }
    if ( ret == LdapOperation::RES_SEARCH_RESULT ) break;
    if ( ret != LdapOperation::RES_SEARCH_ENTRY ) continue;
    kDebug(7125) << " ldap_result: " << ret;

    total++;
    uds.clear();

    LDAPEntry2UDSEntry( mOp.object().dn(), uds, usrc );
    listEntry( uds, false );
//      processedSize( total );
    kDebug(7125) << " total: " << total << " " << usrc.prettyUrl();

    // publish the sub-directories (if dirmode==sub)
    if ( isSub ) {
      LdapDN dn = mOp.object().dn();
      usrc2.setDn( dn );
      usrc2.setScope( LdapUrl::One );
      usrc2.setAttributes( saveatt );
      usrc2.setFilter( usrc.filter() );
      kDebug(7125) << "search2 " << dn.toString();
      if ( (id2 = mOp.search( dn, LdapUrl::One, QString(), att )) != -1 ) {
        while ( true ) {
          kDebug(7125) << " next result ";
          ret2 = mOp.waitForResult( id2, -1 );
          if ( ret2 == -1 || ret2 == LdapOperation::RES_SEARCH_RESULT ) break;
          if ( ret2 == LdapOperation::RES_SEARCH_ENTRY ) {
            LDAPEntry2UDSEntry( dn, uds, usrc2, true );
            listEntry( uds, false );
            total++;
            mOp.abandon( id2 );
            break;
          }
        }
      }
    }
  }

//  totalSize( total );

  uds.clear();
  listEntry( uds, true );
  // we are done
  finished();
}
