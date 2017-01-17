/**
* @file IMAPClient.cpp
* @brief implementation of the IMAP client class
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
*/

#include "IMAPClient.h"

CIMAPClient::CIMAPClient(LogFnCallback oLogger) :
   CMailClient(oLogger),
   m_pstrText(nullptr),
   m_eOperationType(IMAP_NOOP),
   m_eMailProperty(MailProperty::Flagged),
   m_eSearchOption(SearchOption::FLAGGED)
{

}

const bool CIMAPClient::CleanupSession()
{
   m_pstrText = nullptr;
   return CMailClient::CleanupSession();
}

const bool CIMAPClient::List(std::string& strList, const std::string& strFolderName)
{
   m_strFolderName = strFolderName;
   m_pstrText = &strList;
   m_eOperationType = IMAP_LIST;

   return Perform();
}

const bool CIMAPClient::ListSubFolders(std::string& strList)
{
   m_pstrText = &strList;
   m_eOperationType = IMAP_LSUB;

   return Perform();
}

const bool CIMAPClient::SendString(const std::string& strMail)
{
   m_strMail = strMail;
   m_eOperationType = IMAP_SEND_STRING;

   return Perform();
}

const bool CIMAPClient::SendFile(const std::string& strPath)
{
   m_strLocalFile = strPath;
   m_eOperationType = IMAP_SEND_FILE;

   return Perform();
}

const bool CIMAPClient::GetString(const std::string& strMsgNumber, std::string& strOutput)
{
   m_strMsgNumber = strMsgNumber;
   m_pstrText = &strOutput;
   m_eOperationType = IMAP_RETR_STRING;

   return Perform();
}

const bool CIMAPClient::GetFile(const std::string& strMsgNumber, const std::string& strFilePath)
{
   m_strMsgNumber = strMsgNumber;
   m_strLocalFile = strFilePath;
   m_eOperationType = IMAP_RETR_FILE;

   return Perform();
}

const bool CIMAPClient::DeleteFolder(const std::string& strFolderName)
{
   m_strFolderName = strFolderName;
   m_eOperationType = IMAP_DELETE_FOLDER;

   return Perform();
}

const bool CIMAPClient::Noop()
{
   m_eOperationType = IMAP_NOOP;

   return Perform();
}

const bool CIMAPClient::CopyMail(const std::string& strMsgNumber, const std::string& strFolderName)
{
   m_strMsgNumber = strMsgNumber;
   m_strFolderName = strFolderName;
   m_eOperationType = IMAP_COPY;

   return Perform();
}

const bool CIMAPClient::CreateFolder(const std::string& strFolderName)
{
   m_strFolderName = strFolderName;
   m_eOperationType = IMAP_CREATE;

   return Perform();
}

const bool CIMAPClient::SetMailProperty(const std::string& strMsgNumber, MailProperty eNewProperty)
{
   m_strMsgNumber = strMsgNumber;
   m_eMailProperty = eNewProperty;
   m_eOperationType = IMAP_STORE;

   return Perform();
}

const bool CIMAPClient::Search(std::string& strRes, SearchOption eSearchOption)
{
   m_pstrText = &strRes;
   m_eSearchOption = eSearchOption;
   m_eOperationType = IMAP_SEARCH;

   return Perform();
}

const bool CIMAPClient::InfoFolder(std::string& strFolderName, std::string& strInfo)
{
   m_strFolderName = strFolderName;
   m_pstrText = &strInfo;
   m_eOperationType = IMAP_INFO_FOLDER;

   return Perform();
}

void CIMAPClient::ParseURL(std::string& strURL)
{
   std::string strTmp = strURL;
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);

   if (strTmp.compare(0, 8, "IMAPS://") == 0 || strTmp.compare(0, 7, "IMAP://") == 0)
   {
      if (m_eSslTlsFlags != SslTlsFlag::ENABLE_SSL)
         m_eSslTlsFlags = SslTlsFlag::ENABLE_SSL;
   }
   else if (m_eSslTlsFlags == SslTlsFlag::ENABLE_SSL)
   {
      strURL.insert(0, "imaps://");
   }
   else
      strURL.insert(0, "imap://");

   /* append a '/' to the end of the URL so that we can easily add the message number
   * at the end of it */
   if (strURL.at(strURL.length() - 1) != '/')
      strURL += '/';

}

/**
* @brief configures the curl session according to requested
* IMAp operation.
*
*
* @retval true   Successfully configured the curl session.
* @retval false  The configuration couldn't be performed.
*
*/
const bool CIMAPClient::PrePerform()
{
   std::string strRequestURL(m_strURL);
   std::string strCmd; // for SEARCH or STORE
   //curl_off_t fsize;
   //size_t uFileSize = 0;
   //size_t uCountLF = 0;
   m_ssString.clear();

   switch (m_eOperationType)
   {
      case IMAP_SEND_STRING:
         /* This will create a new message 100. Note that you should perform an
         * EXAMINE command to obtain the UID of the next message to create and a
         * SELECT to ensure you are creating the message in the OUTBOX. */
         strRequestURL += m_strMsgNumber;
         m_ssString.str(m_strMail);
         
         /* LF will be replaced by CRLF when sending the mail.
         * This must be taken in consideration in the total size of the payload */
         /*uCountLF = std::count_if(m_strMail.cbegin(), m_strMail.cend(),
                                       [](const char c) { return c == '\n'; });*/

         //curl_easy_setopt(m_pCurlSession, CURLOPT_INFILESIZE, uCountLF + m_strMail.length());
         curl_easy_setopt(m_pCurlSession, CURLOPT_READFUNCTION, ReadLineFromStringStreamCallback);
         curl_easy_setopt(m_pCurlSession, CURLOPT_READDATA, &m_ssString);
         curl_easy_setopt(m_pCurlSession, CURLOPT_UPLOAD, 1L);
         break;

      case IMAP_SEND_FILE:
         if (!m_strLocalFile.empty())
         {
            // Request file size
            /*struct stat file_info;
            if (stat(m_strLocalFile.c_str(), &file_info))
            {
               if (m_eSettingsFlags & ENABLE_LOG)
                  m_oLog(StringFormat("[IMAPClient][Error] Unable to request local file size "
                  "%s : %s - in CIMAPClient::PrePerform() in case IMAP_SEND_FILE.", m_strLocalFile.c_str(),
                     strerror(errno)));

               return false;
            }
            fsize = (curl_off_t)file_info.st_size;*/

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
                  m_oLog(StringFormat("[IMAPClient][Error] Unable to open local file %s in CIMAPClient::PrePerform()"
                                      "in case IMAP_SEND_FILE.", m_strLocalFile.c_str()));

               return false;
            }
         }
         else
            return false;

         break;

      case IMAP_NOOP:
         /* Set the NOOP command */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "NOOP");

         /* Do not perform a transfer as NOOP returns no data */
         curl_easy_setopt(m_pCurlSession, CURLOPT_NOBODY, 1L);
         break;

      case IMAP_LIST:
         if (m_pstrText != nullptr)
         {
            if (!m_strFolderName.empty())
               strRequestURL += m_strFolderName;

            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         break;

      case IMAP_DELETE_FOLDER:
         /* You can specify the message either in the URL or DELE command */
         if (!m_strFolderName.empty())
         {
            /* Set the DELE command */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, ("DELETE " + m_strFolderName).c_str());
         }
         else
            return false;

         break;

      case IMAP_RETR_STRING:
         if (!m_strMsgNumber.empty())
            strRequestURL += "INBOX/;UID=" + m_strMsgNumber;
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

      case IMAP_RETR_FILE:
         if (!m_strMsgNumber.empty())
            strRequestURL += "INBOX/;UID=" + m_strMsgNumber;
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
               m_oLog(StringFormat("[IMAPClient][Error] Unable to open local file %s in CIMAPClient::PrePerform()"
                  " in case IMAP_RETR_FILE.", m_strLocalFile.c_str()));

            return false;
         }
         break;

      case IMAP_INFO_FOLDER:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         /* Set the EXAMINE command specifing the mailbox folder */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, ("EXAMINE " + m_strFolderName).c_str());

         break;

      case IMAP_LSUB:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         /* Set the LSUB command. Note the syntax is very similar to that of a LIST
         command. */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "LSUB \"\" *");

         break;

      case IMAP_COPY:
         if (!m_strMsgNumber.empty() && !m_strFolderName.empty())
         {
            strRequestURL += "INBOX";
            /* Set the COPY command specifing the message ID and destination folder */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST,
               ("COPY "+ m_strMsgNumber + " " + m_strFolderName).c_str());

            /* Note that to perform a move operation you will need to perform the copy,
            * then mark the original mail as Deleted and EXPUNGE or CLOSE. Please see
            * imap-store.c for more information on deleting messages. */
         }
         else
            return false;

         break;

      case IMAP_CREATE:
         if (!m_strFolderName.empty())
         {
            /* Set the CREATE command specifing the new folder name */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, ("CREATE " + m_strFolderName).c_str());
         }
         else
            return false;

         break;

      case IMAP_SEARCH:
         if (m_pstrText != nullptr)
         {
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEFUNCTION, &CMailClient::WriteInStringCallback);
            curl_easy_setopt(m_pCurlSession, CURLOPT_WRITEDATA, m_pstrText);
         }
         else
            return false;

         strRequestURL += "INBOX";

         if (m_eSearchOption == SearchOption::ANSWERED)
            strCmd = "ANSWERED";
         else if (m_eSearchOption == SearchOption::DELETED)
            strCmd = "DELETED";
         else if (m_eSearchOption == SearchOption::DRAFT)
            strCmd = "DRAFT";
         else if (m_eSearchOption == SearchOption::FLAGGED)
            strCmd = "FLAGGED";
         else if (m_eSearchOption == SearchOption::NEW)
            strCmd = "NEW";
         else if (m_eSearchOption == SearchOption::RECENT)
            strCmd = "RECENT";
         else if (m_eSearchOption == SearchOption::SEEN)
            strCmd = "SEEN";
         else
         {
            return false;
         }

         /* Set the SEARCH command specifing what we want to search for. Note that
         * this can contain a message sequence set and a number of search criteria
         * keywords including flags such as ANSWERED, DELETED, DRAFT, FLAGGED, NEW,
         * RECENT and SEEN. For more information about the search criteria please
         * see RFC-3501 section 6.4.4.   */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, ("SEARCH " + strCmd).c_str());

         break;

      case IMAP_STORE:
         if (!m_strMsgNumber.empty())
         {
            if (m_eMailProperty == MailProperty::Deleted)
               strCmd = "Deleted";
            else if (m_eMailProperty == MailProperty::Seen)
               strCmd = "Seen";
            else if (m_eMailProperty == MailProperty::Answered)
               strCmd = "Answered";
            else if (m_eMailProperty == MailProperty::Flagged)
               strCmd = "Flagged";
            else if (m_eMailProperty == MailProperty::Draft)
               strCmd = "Draft";
            else if (m_eMailProperty == MailProperty::Recent)
               strCmd = "Recent";
            else
            {
               return false;
            }

            strRequestURL += "INBOX";

            /* Set the STORE command with the Deleted flag for message m_strMsgNumber */
            curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST,
               ("STORE " + m_strMsgNumber + " +Flags \\" + strCmd).c_str());
         }
         else
            return false;

         break;

      default:
         if (m_eSettingsFlags & ENABLE_LOG)
            m_oLog("[IMAPClient][Error] Unknown operation.");

         break;
   }

   curl_easy_setopt(m_pCurlSession, CURLOPT_URL, strRequestURL.c_str());

   return true;
}

/**
* @brief performs operations that need to be done after performing
* an IMAP request.
*
*
* @retval true   Successfully performed post request operations.
* @retval false  The post request operations couldn't be performed.
*
*/
const bool CIMAPClient::PostPerform(CURLcode ePerformCode)
{
   if (m_eOperationType == IMAP_SEND_FILE || m_eOperationType == IMAP_RETR_FILE)
      if (m_fLocalFile.is_open())
         m_fLocalFile.close();

   if (m_eOperationType == IMAP_STORE)
   {
      if (ePerformCode != CURLE_OK)
      {
         /* Set the EXPUNGE command, although you can use the CLOSE command if you
         * don't want to know the result of the STORE */
         curl_easy_setopt(m_pCurlSession, CURLOPT_CUSTOMREQUEST, "EXPUNGE");

         /* Perform the second custom request */
         ePerformCode = curl_easy_perform(m_pCurlSession);

         /* Check for errors */
         if (ePerformCode != CURLE_OK)
            if (m_eSettingsFlags & ENABLE_LOG)
               m_oLog(StringFormat(LOG_ERROR_CURL_PEFORM_FAILURE_FORMAT, ePerformCode, curl_easy_strerror(ePerformCode)));
      }
   }

   return true;
}
