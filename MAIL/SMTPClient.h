/*
* @file SMTPClient.h
* @brief libcurl wrapper for SMTP operations
*
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
* @date 2017-01-02
*/

#ifndef INCLUDE_SMTPCLIENT_H_
#define INCLUDE_SMTPCLIENT_H_

#include "MAILClient.h"

class CSMTPClient : public CMailClient
{
public:
   explicit CSMTPClient(LogFnCallback oLogger);

   // copy constructor and assignment operator are disabled
   CSMTPClient(const CSMTPClient& Copy) = delete;
   CSMTPClient& operator=(const CSMTPClient& Copy) = delete;

   /* send a string as an e-mail */
   const bool SendString(const std::string& strFrom, const std::string& strTo,
                   const std::string& strCc, const std::string& strMail);
   
   /* send a text file as an e-mail */
   const bool SendFile(const std::string& strFrom, const std::string& strTo,
                   const std::string& strCc, const std::string& strPath);
   
   /* verify an e-mail address */
   const bool VerifyAddress(const std::string& strAddress);

   /* expand an e-mail mailing list */
   const bool ExpandMailList(const std::string& strListName);

protected:
   enum MailOperation
   {
      SMTP_SEND_STRING,
      SMTP_SEND_FILE,
      SMTP_VRFY,
      SMTP_EXPN
   };

   const bool PrePerform() override;
   const bool PostPerform(CURLcode ePerformCode) override;
   inline void ParseURL(std::string& strURL) override final;

   MailOperation        m_eOperationType;

   std::string          m_strFrom;
   std::string          m_strTo;
   std::string          m_strCc;
   std::string          m_strMail;

};

#endif
