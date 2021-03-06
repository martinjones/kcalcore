#ifndef _IMAPCOMMAND_H
#define _IMAPCOMMAND_H
/**********************************************************************
 *
 *   imapcommand.h  - IMAP4rev1 command handler
 *   Copyright (C) 2000 Sven Carstens <s.carstens@gmx.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *   Send comments and bug fixes to
 *
 *********************************************************************/

#include <QStringList>
#include <QString>
#include <QMap>

#include <boost/shared_ptr.hpp>

class imapCommand;
typedef boost::shared_ptr<imapCommand> CommandPtr;

/**
 *  @brief encapulate a IMAP command
 *  @author Svenn Carstens
 *  @date 2000
 *  @todo fix the documentation
 */

class imapCommand
{
public:

   /**
    * @brief Constructor
    */
  imapCommand ();
   /**
    * @fn imapCommand (const QString & command, const QString & parameter);
    * @brief Constructor
    * @param command Imap command
    * @param parameter Parameters to the command
    * @return none
    */
  imapCommand (const QString & command, const QString & parameter);
  /**
   * @fn bool isComplete ();
   * @brief is it complete?
   * @return whether the command is completed
   */
  bool isComplete ();
  /**
   * @fn const QString & result ();
   * @brief get the result of the command
   * @return The result, i.e. first word of the result line, like OK
   */
  const QString & result ();
  /**
   * @fn const QString & resultInfo ();
   * @brief get information about the result
   * @return Information about the result, i.e. the rest of the result line
   */
  const QString & resultInfo ();
  /**
   * @fn const QString & parameter ();
   * @brief get the parameter
   * @return the parameter
   */
  const QString & parameter ();
  /**
   * @fn const QString & command ();
   * @brief get the command
   * @return the command
   */
  const QString & command ();
  /**
   * @fn const QString & id ();
   * @brief get the id
   * @return the id
   */
  const QString & id ();

  /**
   * @fn void setId (const QString &);
   * @brief set the id
   * @param id the id used by the command
   * @return none
   */
  void setId (const QString &);
  /**
   * @fn void setComplete ();
   * @brief set the completed state
   * @return none
   */
  void setComplete ();
  /**
   * @fn void setResult (const QString &);
   * @brief set the completed state
   * @param result the command result
   * @return none
   */
  void setResult (const QString &);
  /**
   * @fn void setResultInfo (const QString &);
   * @brief set the completed state
   * @param result the command result information
   * @return none
   */
  void setResultInfo (const QString &);
  /**
   * @fn void setCommand (const QString &);
   * @brief set the command
   * @param command the imap command
   * @return none
   */
  void setCommand (const QString &);
  /**
   * @fn void setParameter (const QString &);
   * @brief set the command parameter(s)
   * @param parameter the comand parameter(s)
   * @return none
   */
  void setParameter (const QString &);
  /**
   * @fn const QString getStr ();
   * @brief returns the data to send to the server
   * The function returns the complete data to be sent to
   * the server (\<id\> \<command\> [\<parameter\>])
   * @return the data to send to the server
   * @todo possibly rename function to be clear of it's purpose
   */
  const QString getStr ();

  /**
   * @fn static imapCommand *clientNoop ();
   * @brief Create a NOOP command
   * @return a NOOP imapCommand
   */
  static CommandPtr clientNoop ();
  /**
   * @fn static imapCommand *clientFetch (ulong uid, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param uid Uid of the message to fetch
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a single uid
   */
  static CommandPtr clientFetch (ulong uid, const QString & fields,
                                 bool nouid = false);
  /**
   * @fn static imapCommand *clientFetch (ulong fromUid, ulong toUid, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param fromUid start uid of the messages to fetch
   * @param toUid last uid of the messages to fetch
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a range of uids
   */
  static CommandPtr clientFetch (ulong fromUid, ulong toUid,
                                 const QString & fields, bool nouid =
                                 false);
  /**
   * @fn static imapCommand *clientFetch (const QString & sequence, const QString & fields, bool nouid = false);
   * @brief Create a FETCH command
   * @param sequence a IMAP FETCH sequence string
   * @param fields options to pass to the server
   * @param nouid Perform a FETCH or UID FETCH command
   * @return a FETCH imapCommand
   * Fetch a range of uids. The other clientFetch functions are just
   * wrappers around this function.
   */
  static CommandPtr clientFetch (const QString & sequence,
                                 const QString & fields, bool nouid =
                                 false);
  /**
   * @fn static imapCommand *clientList (const QString & reference, const QString & path, bool lsub = false);
   * @brief Create a LIST command
   * @param reference
   * @param path The path to list
   * @param lsub Perform a LIST or a LSUB command
   * @return a LIST imapCommand
   */
  static CommandPtr clientList (const QString & reference,
                                const QString & path, bool lsub = false);
  /**
   * @fn static imapCommand *clientSelect (const QString & path, bool examine = false);
   * @brief Create a SELECT command
   * @param path The path to select
   * @param lsub Perform a SELECT or a EXAMINE command
   * @return a SELECT imapCommand
   */
  static CommandPtr clientSelect (const QString & path, bool examine =
                                  false);
  /**
   * @fn static imapCommand *clientClose();
   * @brief Create a CLOSE command
   * @return a CLOSE imapCommand
   */
  static CommandPtr clientClose();
  /**
   * @brief Create a STATUS command
   * @param path
   * @param parameters
   * @return a STATUS imapCommand
   */
  static CommandPtr clientStatus (const QString & path,
                                  const QString & parameters);
  /**
   * @brief Create a COPY command
   * @param box
   * @param sequence
   * @param nouid Perform a COPY or UID COPY command
   * @return a COPY imapCommand
   */
  static CommandPtr clientCopy (const QString & box,
                                const QString & sequence, bool nouid =
                                false);
  /**
   * @brief Create a APPEND command
   * @param box
   * @param flags
   * @param size
   * @return a APPEND imapCommand
   */
  static CommandPtr clientAppend (const QString & box,
                                  const QString & flags, ulong size);
  /**
   * @brief Create a CREATE command
   * @param path
   * @return a CREATE imapCommand
   */
  static CommandPtr clientCreate (const QString & path);
  /**
   * @brief Create a DELETE command
   * @param path
   * @return a DELETE imapCommand
   */
  static CommandPtr clientDelete (const QString & path);
  /**
   * @brief Create a SUBSCRIBE command
   * @param path
   * @return a SUBSCRIBE imapCommand
   */
  static CommandPtr clientSubscribe (const QString & path);
  /**
   * @brief Create a UNSUBSCRIBE command
   * @param path
   * @return a UNSUBSCRIBE imapCommand
   */
  static CommandPtr clientUnsubscribe (const QString & path);
  /**
   * @brief Create a EXPUNGE command
   * @return a EXPUNGE imapCommand
   */
  static CommandPtr clientExpunge ();
  /**
   * @brief Create a RENAME command
   * @param src Source
   * @param dest Destination
   * @return a RENAME imapCommand
   */
  static CommandPtr clientRename (const QString & src,
                                  const QString & dest);
  /**
   * @brief Create a SEARCH command
   * @param search
   * @param nouid Perform a UID SEARCH or a SEARCH command
   * @return a SEARCH imapCommand
   */
  static CommandPtr clientSearch (const QString & search, bool nouid =
                                  false);
  /**
   * @brief Create a STORE command
   * @param set
   * @param item
   * @param data
   * @param nouid Perform a UID STORE or a STORE command
   * @return a STORE imapCommand
   */
  static CommandPtr clientStore (const QString & set, const QString & item,
                                 const QString & data, bool nouid = false);
  /**
   * @brief Create a LOGOUT command
   * @return a LOGOUT imapCommand
   */
  static CommandPtr clientLogout ();
  /**
   * @brief Create a STARTTLS command
   * @return a STARTTLS imapCommand
   */
  static CommandPtr clientStartTLS ();

  //////////// ACL support (RFC 2086) /////////////
  /**
   * @brief Create a SETACL command
   * @param box mailbox name
   * @param user authentication identifier
   * @param acl access right modification (starting with optional +/-)
   * @return a SETACL imapCommand
   */
  static CommandPtr clientSetACL ( const QString& box, const QString& user, const QString& acl );

  /**
   * @brief Create a DELETEACL command
   * @param box mailbox name
   * @param user authentication identifier
   * @return a DELETEACL imapCommand
   */
  static CommandPtr clientDeleteACL ( const QString& box, const QString& user );

  /**
   * @brief Create a GETACL command
   * @param box mailbox name
   * @return a GETACL imapCommand
   */
  static CommandPtr clientGetACL ( const QString& box );

  /**
   * @brief Create a LISTRIGHTS command
   * @param box mailbox name
   * @param user authentication identifier
   * @return a LISTRIGHTS imapCommand
   */
  static CommandPtr clientListRights ( const QString& box, const QString& user );

  /**
   * @brief Create a MYRIGHTS command
   * @param box mailbox name
   * @return a MYRIGHTS imapCommand
   */
  static CommandPtr clientMyRights ( const QString& box );

  //////////// ANNOTATEMORE support /////////////
  /**
   * @brief Create a SETANNOTATION command
   * @param box mailbox name
   * @param entry entry specifier
   * @param attributes map of attribute names + values
   * @return a SETANNOTATION imapCommand
   */
  static CommandPtr clientSetAnnotation ( const QString& box, const QString& entry, const QMap<QString, QString>& attributes );

  /**
   * @brief Create a GETANNOTATION command
   * @param box mailbox name
   * @param entry entry specifier
   * @param attributeNames attribute specifier
   * @return a GETANNOTATION imapCommand
   */
  static CommandPtr clientGetAnnotation ( const QString& box, const QString& entry, const QStringList& attributeNames );

  /**
   * @brief Create a NAMESPACE command
   * @return a NAMESPACE imapCommand
   */
  static CommandPtr clientNamespace ();

  /**
   * @brief Create a GETQUOTAROOT command
   * @param box mailbox name
   * @return a GETQUOTAROOT imapCommand
   */
  static CommandPtr clientGetQuotaroot ( const QString& box );

  /**
   * @brief Create a custom command
   * @param command The custom command
   * @param arguments The custom arguments
   * @return a custom imapCommand
   */
  static CommandPtr clientCustom ( const QString& command, const QString& arguments );

protected:
  QString aCommand;
  QString mId;
  bool mComplete;
  QString aParameter;
  QString mResult;
  QString mResultInfo;

private:
  imapCommand & operator = (const imapCommand &);
};

#endif
