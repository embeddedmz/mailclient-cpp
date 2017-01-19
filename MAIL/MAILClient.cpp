/**
* @file MAILClient.cpp
* @brief implementation of the mail client class
* @author Mohamed Amine Mzoughi <mohamed-amine.mzoughi@laposte.net>
*/

#include "MAILClient.h"

// Static members initialization
volatile int   CMailClient::s_iCurlSession = 0;
std::string    CMailClient::s_strCertificationAuthorityFile;
std::mutex     CMailClient::s_mtxCurlSession;

#ifdef DEBUG_CURL
std::string CMailClient::s_strCurlTraceLogDirectory;
#endif

/**
* @brief constructor for the mail client object
*
* @param Logger - a callabck to a logger function void(const std::string&)
*
*/
CMailClient::CMailClient(LogFnCallback Logger) :
   m_oLog(Logger),
   m_iCurlTimeout(0),
   m_eSettingsFlags(ALL_FLAGS),
   m_eSslTlsFlags(SslTlsFlag::NO_SSLTLS),
   m_pCurlSession(nullptr),   
   m_pRecipientslist(nullptr),
   m_bProgressCallbackSet(false),
   m_bNoSignal(false)
{
   s_mtxCurlSession.lock();
   if (s_iCurlSession++ == 0)
   {
      curl_global_init(CURL_GLOBAL_ALL);
   }
   s_mtxCurlSession.unlock();
}

/**
* @brief destructor for the mail client object
*
*/
CMailClient::~CMailClient()
{
   if (m_pCurlSession != nullptr)
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_WARNING_OBJECT_NOT_CLEANED);
      
      CleanupSession();
   }
   
   s_mtxCurlSession.lock();
   if (--s_iCurlSession <= 0)
   {
      curl_global_cleanup();
   }
   s_mtxCurlSession.unlock();
}

/**
 * @brief Starts a new mail session, initializes the cURL API session
 *
 * If a new session was already started, the method has no effect.
 *
 * @param [in] strHost server address with or without port number
 * @param [in] strLogin username
 * @param [in] strPassword password
 * @param [in] eSettingsFlags optional use | operator to choose multiple options
 * @param [in] eSslTlsFlags optional encryption type
 *
 * @retval true   Successfully initialized the session.
 * @retval false  The session is already initialized : call CleanupSession()
 * before initializing a new one or the Curl API is not initialized.
 *
 * Example Usage:
 * @code
 *    pMailClient->InitSession("smtp.gnet.tn:25", "amine", "my_password", ENABLE_LOG);
 * @endcode
 */
const bool CMailClient::InitSession(const std::string& strHost, const std::string& strLogin,
                                    const std::string& strPassword,
                                    const SettingsFlag& eSettingsFlags /* = ALL_FLAGS */,
                                    const SslTlsFlag& eSslTlsFlags     /* = NO_SSLTLS */)
{
   if (strHost.empty())
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_EMPTY_HOST_MSG);
      
      return false;
   }

   if (m_pCurlSession)
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_CURL_ALREADY_INIT_MSG);
      
      return false;
   }
   m_pCurlSession = curl_easy_init();

   m_eSettingsFlags = eSettingsFlags;
   m_eSslTlsFlags = eSslTlsFlags;
   m_strURL = strHost;
   ParseURL(m_strURL);
   m_strUserName = strLogin;
   m_strPassword = strPassword;

   return (m_pCurlSession != nullptr);
}

/**
 * @brief Cleans the current mail session
 *
 * If a session was not already started, this method has no effect
 *
 * @retval true   Successfully cleaned the current session.
 * @retval false  The session is not already initialized.
 *
 * Example Usage:
 * @code
 *    objMailClient.CleanupSession();
 * @endcode
 */
const bool CMailClient::CleanupSession()
{
   if (!m_pCurlSession)
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_CURL_NOT_INIT_MSG);
      
      return false;
   }

   #ifdef DEBUG_CURL
   if (m_ofFileCurlTrace.is_open())
   {
      m_ofFileCurlTrace.close();
   }
   #endif

   curl_easy_cleanup(m_pCurlSession);
   m_pCurlSession = nullptr;

   /* Free the list of recipients */
   if (m_pRecipientslist)
   {
      curl_slist_free_all(m_pRecipientslist);

      /* Curl won't send the QUIT command until you call cleanup, so you should
      * be able to re-use this connection for additional requests or messages
      * (setting CURLOPT_MAIL_FROM and CURLOPT_MAIL_RCPT as required, and calling
      * curl_easy_perform() again. It may not be a good idea to keep the connection
      * open for a very long time though (more than a few minutes may result in the 
      * server timing out the connection) and you do want to clean up in the end.
      */
      m_pRecipientslist = nullptr;
   }

   return true;
}

/**
* @brief sets the progress function callback and the owner of the client
*
* @param [in] pOwner pointer to the object owning the client, nullptr otherwise
* @param [in] fnCallback callback to progress function
*
*/
void CMailClient::SetProgressFnCallback(void* pOwner, const ProgressFnCallback& fnCallback)
{
   m_ProgressStruct.pOwner = pOwner;
   m_fnProgressCallback = fnCallback;
   m_ProgressStruct.pCurl = m_pCurlSession;
   m_ProgressStruct.dLastRunTime = 0;
   m_bProgressCallbackSet = true;
}

/**
* @brief sets the HTTP Proxy address to tunnel the operation through it
*
* @param [in] strProxy URI of the HTTP Proxy
*
*/
void CMailClient::SetProxy(const std::string& strProxy)
{
   if (strProxy.empty())
      return;

   std::string strUri = strProxy;
   std::transform(strUri.begin(), strUri.end(), strUri.begin(), ::toupper);

   if (strUri.compare(0, 4, "HTTP") != 0)
      m_strProxy = "http://" + strProxy;
   else
      m_strProxy = strProxy;
};

/**
* @brief performs the request of a mail client
*
*
* @retval true   Successfully performed the request.
* @retval false  The request couldn't be performed.
*
*/
const bool CMailClient::Perform()
{
   CURLcode res = CURLE_OK;

   if (!m_pCurlSession)
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_CURL_NOT_INIT_MSG);

      return false;
   }
   // Reset is mandatory to avoid bad surprises
   curl_easy_reset(m_pCurlSession);

   if (!PrePerform())
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_PREPERFORM_FAILED_MSG);

      return false;
   }

   /* Set username and password */
   curl_easy_setopt(m_pCurlSession, CURLOPT_USERNAME, m_strUserName.c_str());
   curl_easy_setopt(m_pCurlSession, CURLOPT_PASSWORD, m_strPassword.c_str());


   if (m_eSslTlsFlags & ENABLE_TLS)
   {
      /* With TLS, start with a plain text connection, and upgrade
      * to Transport Layer Security (TLS) using the STARTTLS (SMTP) or the STLS
      * (POP) command. Be careful of using CURLUSESSL_TRY here, because if TLS
      * upgrade fails, the transfer will continue anyway - see the security
      * discussion in the libcurl tutorial for more details.
      *
      * If your server doesn't have a valid certificate, then you can disable
      * part of the Transport Layer Security protection by setting the
      * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false).
      *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      *
      * That is, in general, a bad idea. It is still better than sending your
      * authentication details in plain text though.  Instead, you should get
      * the issuer certificate (or the host certificate if the certificate is
      * self-signed) and add it to the set of certificates that are known to
      * libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See docs/SSLCERTS
      * for more information. */
      curl_easy_setopt(m_pCurlSession, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL));
   }
   if (!s_strCertificationAuthorityFile.empty())
      curl_easy_setopt(m_pCurlSession, CURLOPT_CAINFO, s_strCertificationAuthorityFile.c_str());

   if (!m_strSSLCertFile.empty())
      curl_easy_setopt(m_pCurlSession, CURLOPT_SSLCERT, m_strSSLCertFile.c_str());

   if (!m_strSSLKeyFile.empty())
      curl_easy_setopt(m_pCurlSession, CURLOPT_SSLKEY, m_strSSLKeyFile.c_str());

   if (!m_strSSLKeyPwd.empty())
      curl_easy_setopt(m_pCurlSession, CURLOPT_KEYPASSWD, m_strSSLKeyPwd.c_str());

   if (!(m_eSettingsFlags & VERIFY_PEER))
   {
      /* If you want to connect to a site who isn't using a certificate that is
      * signed by one of the certs in the CA bundle you have, you can skip the
      * verification of the server's certificate. This makes the connection
      * A LOT LESS SECURE.
      *
      * If you have a CA cert for the server stored someplace else than in the
      * default bundle, then the CURLOPT_CAPATH option might come handy for
      * you. */
      curl_easy_setopt(m_pCurlSession, CURLOPT_SSL_VERIFYPEER, 0L);
   }

   /* If the site you're connecting to uses a different host name that what
   * they have mentioned in their server certificate's commonName (or
   * subjectAltName) fields, libcurl will refuse to connect. You can skip
   * this check, but this will make the connection less secure. */
   if (!(m_eSettingsFlags & VERIFY_HOST))
      curl_easy_setopt(m_pCurlSession, CURLOPT_SSL_VERIFYHOST, 0L); // use 2L for strict name check

   if (m_bProgressCallbackSet)
   {
      curl_easy_setopt(m_pCurlSession, CURLOPT_PROGRESSFUNCTION, *GetProgressFnCallback());
      curl_easy_setopt(m_pCurlSession, CURLOPT_PROGRESSDATA, &m_ProgressStruct);
      curl_easy_setopt(m_pCurlSession, CURLOPT_NOPROGRESS, 0L);
   }

   /* some servers need this */
   curl_easy_setopt(m_pCurlSession, CURLOPT_USERAGENT, CLIENT_USERAGENT);

   if (m_iCurlTimeout > 0)
   {
      curl_easy_setopt(m_pCurlSession, CURLOPT_TIMEOUT, m_iCurlTimeout);
      // don't want to get a sig alarm on timeout
      curl_easy_setopt(m_pCurlSession, CURLOPT_NOSIGNAL, 1);
   }

   if (!m_strProxy.empty())
   {
      curl_easy_setopt(m_pCurlSession, CURLOPT_PROXY, m_strProxy.c_str());
      curl_easy_setopt(m_pCurlSession, CURLOPT_HTTPPROXYTUNNEL, 1L);
   }

   if (m_bNoSignal)
   {
      curl_easy_setopt(m_pCurlSession, CURLOPT_NOSIGNAL, 1L);
   }

#ifdef DEBUG_CURL
   StartCurlDebug();
#endif

   // Perform the requested operation
   res = curl_easy_perform(m_pCurlSession);

#ifdef DEBUG_CURL
   EndCurlDebug();
#endif

   if (!PostPerform(res))
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(LOG_ERROR_POSTPERFORM_FAILED_MSG);

      return false;
   }

   if (res != CURLE_OK)
   {
      if (m_eSettingsFlags & ENABLE_LOG)
         m_oLog(StringFormat(LOG_ERROR_CURL_PEFORM_FAILURE_FORMAT, res, curl_easy_strerror(res)));

      return false;
   }
   return true;
}

/**
* @brief returns a formatted string
*
* @param [in] strFormat string with one or many format specifiers
* @param [in] parameters to be placed in the format specifiers of strFormat
*
* @retval string formatted string
*/
std::string CMailClient::StringFormat(const std::string strFormat, ...)
{
   int n = (static_cast<int>(strFormat.size())) * 2; // Reserve two times as much as the length of the strFormat
   
   std::unique_ptr<char[]> pFormatted;

   va_list ap;

   while(true)
   {
      pFormatted.reset(new char[n]); // Wrap the plain char array into the unique_ptr
      strcpy(&pFormatted[0], strFormat.c_str());
      
      va_start(ap, strFormat);
      int iFinaln = vsnprintf(&pFormatted[0], n, strFormat.c_str(), ap);
      va_end(ap);
      
      if (iFinaln < 0 || iFinaln >= n)
      {
         n += abs(iFinaln - n + 1);
      }
      else
      {
         break;
      }
   }

   return std::string(pFormatted.get());
}

/**
* @brief stores the server response in a string
*
* @param ptr pointer of max size (size*nmemb) to read data from it
* @param size size parameter
* @param nmemb memblock parameter
* @param data pointer to user data (string)
*
* @return (size * nmemb)
*/
size_t CMailClient::WriteInStringCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
   std::string* strWriteHere = reinterpret_cast<std::string*>(data);
   if (strWriteHere != nullptr)
   {
      strWriteHere->append(reinterpret_cast<char*>(ptr), size * nmemb);
      return size * nmemb;
   }
   return 0;
}

/**
* @brief stores the server response in an already opened file stream
*
* @param buff pointer of max size (size*nmemb) to read data from it
* @param size size parameter
* @param nmemb memblock parameter
* @param userdata pointer to user data (file stream)
*
* @return (size * nmemb)
*/
size_t CMailClient::WriteToFileCallback(void* buff, size_t size, size_t nmemb, void* data)
{
   if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1) || (data == nullptr))
      return 0;

   std::fstream* pFileStream = reinterpret_cast<std::fstream*>(data);
   if (pFileStream->is_open())
   {
      pFileStream->write(reinterpret_cast<char*>(buff), size * nmemb);
   }
   else
   {
      std::cout.write(reinterpret_cast<char*>(buff), size * nmemb);
   }
   return size * nmemb;
}

/**
* @brief sends a line from an already opened file stream (text)
*
* @param ptr pointer of max size (size*nmemb) to write data to it
* @param size size parameter
* @param nmemb memblock parameter
* @param stream pointer to user data (file stream)
*
* @return (size * nmemb)
*/
size_t CMailClient::ReadLineFromFileStreamCallback(void* ptr, size_t size, size_t nmemb, void* stream)
{
   if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1) || (stream == nullptr))
      return 0;

   std::fstream* pFileStream = reinterpret_cast<std::fstream*>(stream);
   std::string strLine;

   // what if the memory pointed by ptr was less than strLine.length() ?

   if (pFileStream->is_open() && getline(*pFileStream, strLine))
   {
      strLine.append("\r\n");
      std::memcpy(ptr, strLine.c_str(), strLine.length());
      return strLine.length();
   }
   return 0;
}

/**
* @brief sends a line from an input string stream
*
* @param ptr pointer of max size (size*nmemb) to write data to it
* @param size size parameter
* @param nmemb memblock parameter
* @param userp pointer to user data (input string stream)
*
* @return (size * nmemb)
*/
size_t CMailClient::ReadLineFromStringStreamCallback(void* ptr, size_t size, size_t nmemb, void* userp)
{
   if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1) || (userp == nullptr))
      return 0;

   std::istringstream* ssText = reinterpret_cast<std::istringstream*>(userp);
   std::string strLine;

   // what if the memory pointed by ptr was less than strLine.length() ?

   if (std::getline(*ssText, strLine, '\n'))
   {
      strLine.append("\r\n");
      std::memcpy(ptr, strLine.c_str(), strLine.length());
      return strLine.length();
   }
   return 0;
}

/**
* @brief reads the content of an already opened file stream
*
* @param ptr pointer of max size (size*nmemb) to write data to it
* @param size size parameter
* @param nmemb memblock parameter
* @param stream pointer to user data (file stream)
*
* @return (size * nmemb)
*/
size_t CMailClient::ReadFromFileCallback(void* ptr, size_t size, size_t nmemb, void* stream)
{
   std::fstream* pFileStream = reinterpret_cast<std::fstream*>(stream);
   if (pFileStream->is_open())
   {
      pFileStream->read(reinterpret_cast<char*>(ptr), size * nmemb);
      return pFileStream->gcount();
   }
   return 0;
}

#ifdef DEBUG_CURL
void CMailClient::SetCurlTraceLogDirectory(const std::string& strPath)
{
   s_strCurlTraceLogDirectory = strPath;

   if (!s_strCurlTraceLogDirectory.empty()
#ifdef WINDOWS
       && s_strCurlTraceLogDirectory.at(s_strCurlTraceLogDirectory.length() - 1) != '\\')
   {
      s_strCurlTraceLogDirectory += '\\';
   }
#else
      && s_strCurlTraceLogDirectory.at(s_strCurlTraceLogDirectory.length() - 1) != '/')
   {
      s_strCurlTraceLogDirectory += '/';
   }
#endif
}

int CMailClient::DebugCallback(CURL* curl , curl_infotype curl_info_type, char* pszTrace, size_t usSize, void* pFile)
{
   std::string strText;
   std::string strTrace(pszTrace, usSize);

   switch (curl_info_type)
   {
      case CURLINFO_TEXT:
         strText = "# Information : ";
         break;
      case CURLINFO_HEADER_OUT:
         strText = "-> Sending header : ";
         break;
      case CURLINFO_DATA_OUT:
         strText = "-> Sending data : ";
         break;
      case CURLINFO_SSL_DATA_OUT:
         strText = "-> Sending SSL data : ";
         break;
      case CURLINFO_HEADER_IN:
         strText = "<- Receiving header : ";
         break;
      case CURLINFO_DATA_IN:
         strText = "<- Receiving unencrypted data : ";
         break;
      case CURLINFO_SSL_DATA_IN:
         strText = "<- Receiving SSL data : ";
         break;
      default:
         break;
   }

   std::ofstream* pofTraceFile = reinterpret_cast<std::ofstream*>(pFile);
   if (pofTraceFile == nullptr)
   {
      std::cout << "[DEBUG] cURL debug log [" << curl_info_type << "]: " << " - " << strTrace;
   }
   else
   {
      (*pofTraceFile) << strText << strTrace;
   }

   return 0;
}

void CMailClient::StartCurlDebug()
{
   if (!m_ofFileCurlTrace.is_open())
   {
      curl_easy_setopt(m_pCurlSession, CURLOPT_VERBOSE, 1L);
      curl_easy_setopt(m_pCurlSession, CURLOPT_DEBUGFUNCTION, &CMailClient::DebugCallback);

      std::string strFileCurlTraceFullName(s_strCurlTraceLogDirectory);
      if (!strFileCurlTraceFullName.empty())
      {
         char szDate[32];
         memset(szDate, 0, 32);
         time_t tNow; time(&tNow);
         // new trace file for each hour
         strftime(szDate, 32, "%Y%m%d_%H", localtime(&tNow));
         strFileCurlTraceFullName += "TraceLog_";
         strFileCurlTraceFullName += szDate;
         strFileCurlTraceFullName += ".txt";

         m_ofFileCurlTrace.open(strFileCurlTraceFullName, std::ifstream::app | std::ifstream::binary);

         if (m_ofFileCurlTrace)
            curl_easy_setopt(m_pCurlSession, CURLOPT_DEBUGDATA, &m_ofFileCurlTrace);
      }
   }
}

void CMailClient::EndCurlDebug()
{
   if (m_ofFileCurlTrace && m_ofFileCurlTrace.is_open())
   {
      m_ofFileCurlTrace << "###########################################" << std::endl;
      m_ofFileCurlTrace.close();
   }
}
#endif
