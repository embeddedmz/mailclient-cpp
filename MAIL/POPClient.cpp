/**
* @file POPClient.cpp
* @brief implementation of the POP client class
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
*/

#include "POPClient.h"

CPOPClient::CPOPClient(LogFnCallback oLogger) :
   CMailClient(oLogger),
   m_pstrText(nullptr),
   m_eOperationType(POP3_NOOP)
{

}

const bool CPOPClient::CleanupSession()
{
   m_pstrText = nullptr;
   return CMailClient::CleanupSession();
}

const bool CPOPClient::List(std::string& strList)
{
   m_pstrText = &strList;
   m_eOperationType = POP3_LIST;
   return Perform();
}

const bool CPOPClient::ListUIDL(std::string& strList)
{
   m_pstrText = &strList;
   m_eOperationType = POP3_UIDL;
   return Perform();
}

const bool CPOPClient::GetString(const std::string& strMsgNumber, std::string& strOutput)
{
   m_pstrText = &strOutput;
   m_strMsgNumber = strMsgNumber;
   m_eOperationType = POP3_RETR_STRING;
   return Perform();
}

const bool CPOPClient::GetFile(const std::string& strMsgNumber, const std::string& strFilePath)
{
   m_strLocalFile = strFilePath;
   m_strMsgNumber = strMsgNumber;
   m_eOperationType = POP3_RETR_FILE;
   return Perform();
}

const bool CPOPClient::GetHeaders(const std::string& strMsgNumber, std::string& strOutput)
{
   m_pstrText = &strOutput;
   m_strMsgNumber = strMsgNumber;
   m_eOperationType = POP3_TOP;
   return Perform();
}

const bool CPOPClient::Delete(const std::string& strMsgNumber)
{
   m_strMsgNumber = strMsgNumber;
   m_eOperationType = POP3_DELE;
   return Perform();
}

const bool CPOPClient::Noop()
{
   m_eOperationType = POP3_NOOP;
   return Perform();
}

const bool CPOPClient::Stat(std::string& strStat)
{
   m_pstrText = &strStat;
   m_eOperationType = POP3_STAT;
   return Perform();
}

void CPOPClient::ParseURL(std::string& strURL)
{
   std::string strTmp = strURL;
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);

   if (strTmp.compare(0, 8, "POP3S://") == 0 || strTmp.compare(0, 7, "POP3://") == 0)
   {
      if (m_eSslTlsFlags != SslTlsFlag::ENABLE_SSL)
         m_eSslTlsFlags = SslTlsFlag::ENABLE_SSL;
   }
   else if (m_eSslTlsFlags == SslTlsFlag::ENABLE_SSL)
   {
      strURL.insert(0, "pop3s://");
   }
   else
      strURL.insert(0, "pop3://");

   /* append a '/' to the end of the URL so that we can easily add the message number
    * at the end of it */
   if (strURL.at(strURL.length() - 1) != '/')
      strURL += '/';

}

/**
* @brief configures the curl session according to requested POP
* operation.
*
*
* @retval true   Successfully configured the curl session.
* @retval false  The configuration couldn't be performed.
*
*/
const bool CPOPClient::PrePerform()
{
   std::string strRequestURL(m_strURL);

   switch (m_eOperationType)
   {
      case POP3_TOP:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         if (!m_strMsgNumber.empty())
         {
            /* Set the TOP command for message 'm_strMsgNumber' to only include the headers */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST,
               ("TOP " + m_strMsgNumber + " 0").c_str());
         }
         else
            return false;

         break;

      case POP3_NOOP:
         /* Set the NOOP command */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "NOOP");

         /* Do not perform a transfer as NOOP returns no data */
         curl_easy_setopt(m_pCurlSession, CURLOPT_NOBODY, 1L);
         break;

      case POP3_UIDL:
         /* Set the UIDL command */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "UIDL");
      case POP3_LIST:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         break; // POP3_UIDL & POP3_LIST break

      case POP3_DELE:
         /* You can specify the message either in the URL or DELE command */
         if (!m_strMsgNumber.empty())
         {
            /* Set the DELE command */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, ("DELE " + m_strMsgNumber).c_str());

            /* Do not perform a transfer as DELE returns no data */
            curl_easy_setopt(m_pCurlSession, CURLOPT_NOBODY, 1L);
         }
         else
            return false;
         
         break;

      case POP3_RETR_STRING:
         if (!m_strMsgNumber.empty())
         {
            strRequestURL += m_strMsgNumber;
         }
         else
            return false;

         /* This will retrieve message 'm_strMsgNumber' from the user's mailbox */
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;
         break;

      case POP3_RETR_FILE:
         if (!m_strMsgNumber.empty())
         {
            strRequestURL += m_strMsgNumber;
         }
         else
            return false;

         m_fLocalFile.open(m_strLocalFile, std::fstream::out | std::fstream::binary | std::fstream::trunc);
         if (m_fLocalFile)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteToFileCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, &m_fLocalFile);
         }
         else
         {
            if (m_eSettingsFlags & ENABLE_LOG)
               m_oLog(StringFormat("[POPClient][Error] Unable to open local file %s in "
                  "CMailClient::Perform() in case POP3_RETR_FILE.", m_strLocalFile.c_str()));

            return false;
         }
         break;

      case POP3_STAT:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         /* Set the STAT command */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "STAT");

         /* Do not perform a transfer as the data is in the response */
         curl_easy_setopt(m_pCurlSession, CURLOPT_NOBODY, 1L);
         break;

      default:
         if (m_eSettingsFlags & ENABLE_LOG)
            m_oLog("[POPClient][Error] Unknown operation.");

         break;
   }

   curl_easy_setopt(m_pCurlSession, CURLOPT_URL, strRequestURL.c_str());

   return true;
}

/**
* @brief performs operations that need to be done after performing
* a POP request.
*
*
* @retval true   Successfully performed post request operations.
* @retval false  The post request operations couldn't be performed.
*
*/
const bool CPOPClient::PostPerform(CURLcode ePerformCode)
{
   ePerformCode;

   if (m_eOperationType == POP3_RETR_FILE)
      if (m_fLocalFile.is_open())
         m_fLocalFile.close();

   return true;
}
