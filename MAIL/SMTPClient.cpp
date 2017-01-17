/**
* @file SMTPClient.cpp
* @brief implementation of the SMTP client class
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
*/

#include "SMTPClient.h"

CSMTPClient::CSMTPClient(LogFnCallback oLogger) :
   CMailClient(oLogger),
   m_eOperationType(SMTP_SEND_STRING)
{

}

const bool CSMTPClient::SendString(const std::string& strFrom, const std::string& strTo,
                             const std::string& strCc, const std::string& strMail)
{
   m_strFrom = strFrom;
   m_strTo = strTo;
   m_strCc = strCc;
   m_strMail = strMail;
   m_eOperationType = SMTP_SEND_STRING;
   
   return Perform();
}

const bool CSMTPClient::SendFile(const std::string& strFrom, const std::string& strTo,
                             const std::string& strCc, const std::string& strPath)
{
   m_strFrom = strFrom;
   m_strTo = strTo;
   m_strCc = strCc;
   m_strLocalFile = strPath;
   m_eOperationType = SMTP_SEND_FILE;

   return Perform();
}

const bool CSMTPClient::VerifyAddress(const std::string& strAddress)
{
   m_strTo = strAddress;
   m_eOperationType = SMTP_VRFY;

   return Perform();
}

const bool CSMTPClient::ExpandMailList(const std::string& strListName)
{
   m_strMail = strListName;
   m_eOperationType = SMTP_EXPN;

   return Perform();
}

void CSMTPClient::ParseURL(std::string& strURL)
{
   /* Note the use of smtps:// rather than smtp:// to request a SSL based connection. 
    * for TLS you don't need to change the scheme to smtps. */

   std::string strTmp = strURL;
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);

   if (strTmp.compare(0, 8, "SMTPS://") == 0 || strTmp.compare(0, 7, "SMTP://") == 0)
   {
      if (m_eSslTlsFlags != SslTlsFlag::ENABLE_SSL)
         m_eSslTlsFlags = SslTlsFlag::ENABLE_SSL;
   }
   else if (m_eSslTlsFlags == SslTlsFlag::ENABLE_SSL)
   {
      strURL.insert(0, "smtps://");
   }
   else
      strURL.insert(0, "smtp://");
}

/**
* @brief configures the curl session according to requested
* SMTP operation.
*
*
* @retval true   Successfully configured the curl session.
* @retval false  The configuration couldn't be performed.
*
*/
const bool CSMTPClient::PrePerform()
{
   //size_t uFileSize = 0;
   //size_t uCountLF = 0;

   m_ssString.clear();

   switch (m_eOperationType)
   {
      case SMTP_SEND_STRING:
         if (!m_strFrom.empty() && !m_strTo.empty())
         {
            m_ssString.str(m_strMail);
            /* Note that this option isn't strictly required, omitting it will result
            * in libcurl sending the MAIL FROM command with empty sender data. All
            * autoresponses should have an empty reverse-path, and should be directed
            * to the address in the reverse-path which triggered them. Otherwise,
            * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
            * details.
            */
            curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_FROM, m_strFrom.c_str());

            /* Add two recipients, in this particular case they correspond to the
            * To: and Cc: addressees in the header, but they could be any kind of
            * recipient. */
            m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strTo.c_str());
            
            if (!m_strCc.empty())
               m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strCc.c_str());
            
            curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_RCPT, m_pRecipientslist);

            /* We're using a callback function to specify the payload (the headers and
            * body of the message). You could just use the CURLOPT_READDATA option to
            * specify a FILE pointer to read from. */

            // LF will be replaced by CRLF when sending the mail
            /*uCountLF = std::count_if(m_strMail.cbegin(), m_strMail.cend(),
               [](const char c) { return c == '\n'; });*/

            //curl_easy_setopt(m_pCurlSession, CURLOPT_INFILESIZE, uCountLF + m_strMail.length());
            curl_easy_setopt(m_pCurlSession, CURLOPT_READFUNCTION, ReadLineFromStringStreamCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_READDATA, &m_ssString);
            curl_easy_setopt(m_pCurlSession, CURLOPT_UPLOAD, 1L);
         }
         else
            return false;
         break;

      case SMTP_SEND_FILE:
         if (!m_strLocalFile.empty() && !m_strFrom.empty() && !m_strTo.empty())
         {
            // Request file size
            /*struct stat file_info;
            if (stat(m_strLocalFile.c_str(), &file_info))
            {
               if (m_eSettingsFlags & ENABLE_LOG)
                  m_oLog(StringFormat("[SMTPClient][Error] Unable to request local file size %s : %s"
                  "- in MAILClient::Perform() in case SMTP_SEND.", m_strLocalFile.c_str(), strerror(errno)));

               return false;
            }
            curl_off_t fsize = (curl_off_t)file_info.st_size;*/

            // I want to compute the size of the file without CR bytes
            // does this work properly ?
            m_fLocalFile.open(m_strLocalFile, std::fstream::in);

            // LF will be replaced by CRLF when sending the mail
            /*uCountLF = std::count_if((std::istreambuf_iterator<char>(m_fLocalFile)),
                                      std::istreambuf_iterator<char>(),
                                      [&uFileSize](const char c) -> bool
                                      { ++uFileSize; return c == '\n'; });*/

            if (m_fLocalFile)
            {
               m_fLocalFile.seekg(0);

               curl_easy_setopt(m_pCurlSession, CURLOPT_READFUNCTION, CMailClient::ReadLineFromFileStreamCallback);
               curl_easy_setopt(m_pCurlSession, CURLOPT_READDATA, &m_fLocalFile);
               curl_easy_setopt(m_pCurlSession, CURLOPT_UPLOAD, 1L);
               //curl_easy_setopt(m_pCurlSession, CURLOPT_INFILESIZE_LARGE, uFileSize + uCountLF);
            }
            else
            {
               if (m_eSettingsFlags & ENABLE_LOG)
                  m_oLog(StringFormat("[SMTPClient][Error] Unable to open local file %s in CSMTPClient::PrePerform()"
                                      "in case SMTP_SEND_FILE.", m_strLocalFile.c_str()));

               return false;
            }
            curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_FROM, m_strFrom.c_str());
            m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strTo.c_str());

            if (!m_strCc.empty())
               m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strCc.c_str());
            
            curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_RCPT, m_pRecipientslist);
         }
         else
            return false;

         break;

      case SMTP_VRFY:
         /* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array  */
         if (!m_strTo.empty())
         {
            if (m_strTo.at(0) != '<')
               m_strTo.insert(0, "<");
            
            if (m_strTo.at(m_strTo.length() - 1) != '>')
               m_strTo += '>';

            m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strTo.c_str());
            curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_RCPT, m_pRecipientslist);
         }
         else
            return false;

         break;

      case SMTP_EXPN:
         /* Note that the CURLOPT_MAIL_RCPT takes a list, not a char array. e.g. "Friends" */
         m_pRecipientslist = curl_slist_append(m_pRecipientslist, m_strMail.c_str());
         curl_easy_setopt(m_pCurlSession, CURLOPT_MAIL_RCPT, m_pRecipientslist);

         /* Set the EXPN command */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "EXPN");
         break;

      default:
         if (m_eSettingsFlags & ENABLE_LOG)
            m_oLog("[SMTPClient][Error] Unknown operation.");

         break;
   }

   curl_easy_setopt(m_pCurlSession, CURLOPT_URL, m_strURL.c_str());

   return true;
}

/**
* @brief performs operations that need to be done after performing
* an SMTP request.
*
*
* @retval true   Successfully performed post request operations.
* @retval false  The post request operations couldn't be performed.
*
*/
const bool CSMTPClient::PostPerform(CURLcode ePerformCode)
{
   ePerformCode;

   if (m_eOperationType == SMTP_SEND_FILE)
      if (m_fLocalFile.is_open())
         m_fLocalFile.close();

   return true;
}
