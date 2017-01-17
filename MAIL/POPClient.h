/*
* @file POPClient.h
* @brief libcurl wrapper for POP operations
*
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
* @date 2017-01-02
*/

#ifndef INCLUDE_POPCLIENT_H_
#define INCLUDE_POPCLIENT_H_

#include "MAILClient.h"

class CPOPClient : public CMailClient
{
public:
   explicit CPOPClient(LogFnCallback oLogger);

   // copy constructor and assignment operator are disabled
   CPOPClient(const CPOPClient& Copy) = delete;
   CPOPClient& operator=(const CPOPClient& Copy) = delete;

   const bool CleanupSession() override;
   
   /* list the contents of a mailbox and save it in strList */
   const bool List(std::string& strList);

   /* list the contents of a mailbox by unique ID and save it in strList */
   const bool ListUIDL(std::string& strList);
   
   /* retrieve e-mail and save its content in strOutput */
   const bool GetString(const std::string& strMsgNumber, std::string& strOutput);
   
   /* retrieve e-mail and save its content in a file */
   const bool GetFile(const std::string& strMsgNumber, const std::string& strFilePath);

   /* retrieve only the headers of an e-mail */
   const bool GetHeaders(const std::string& strMsgNumber, std::string& strOutput);

   /* delete an existing e-mail from the mailbox */
   const bool Delete(const std::string& strMsgNumber);

   /* perform a noop */
   const bool Noop();

   /* obtain message statistics and save it in strStat */
   const bool Stat(std::string& strStat);

protected:
   enum MailOperation
   {
      POP3_LIST,
      POP3_RETR_STRING,
      POP3_RETR_FILE,
      POP3_DELE,
      POP3_UIDL,
      POP3_TOP,
      POP3_STAT,
      POP3_NOOP
   };

   const bool PrePerform() override;
   const bool PostPerform(CURLcode ePerformCode) override;
   inline void ParseURL(std::string& strURL) override final;

   MailOperation        m_eOperationType;

   std::string          m_strMsgNumber;
   std::string*         m_pstrText;

};

#endif
