#include "test_utils.h"

// Test configuration constants (to be loaded from an INI file)
bool POP_TEST_ENABLED;
bool POP_SSL_TEST_ENABLED;
bool SMTP_TEST_ENABLED;
bool SMTP_SSL_TEST_ENABLED;
bool IMAP_TEST_ENABLED;
bool HTTP_PROXY_TEST_ENABLED;

std::string CURL_LOG_FOLDER;
std::string CERT_AUTH_FILE;
std::string SSL_CERT_FILE;
std::string SSL_KEY_FILE;
std::string SSL_KEY_PWD;

std::string PROXY_SERVER;
std::string PROXY_SERVER_DISABLED;

std::string POP_SERVER;
std::string POP_USERNAME;
std::string POP_PASSWORD;

std::string SSL_POP_SERVER;
std::string SSL_POP_USERNAME;
std::string SSL_POP_PASSWORD;

std::string IMAP_SERVER;
std::string IMAP_USERNAME;
std::string IMAP_PASSWORD;

std::string SMTP_SERVER;
std::string SMTP_USERNAME;
std::string SMTP_PASSWORD;
std::string SMTP_TO;
std::string SMTP_FROM;
std::string SMTP_CC;

std::string SSL_SMTP_SERVER;
std::string SSL_SMTP_USERNAME;
std::string SSL_SMTP_PASSWORD;
std::string SSL_SMTP_TO;
std::string SSL_SMTP_FROM;
std::string SSL_SMTP_CC;

std::mutex g_mtxConsoleMutex;

void TimeStampTest(std::ostringstream& ssTimestamp)
{
   time_t tRawTime;
   tm * tmTimeInfo;
   time(&tRawTime);
   tmTimeInfo = localtime(&tRawTime);

   ssTimestamp << (tmTimeInfo->tm_year) + 1900
   << "/" << tmTimeInfo->tm_mon + 1 << "/" << tmTimeInfo->tm_mday << " at "
   << tmTimeInfo->tm_hour << ":" << tmTimeInfo->tm_min << ":" << tmTimeInfo->tm_sec;
}

bool GlobalTestInit(const std::string& strConfFile)
{
   CSimpleIniA ini;
   ini.LoadFile(strConfFile.c_str());

   std::string strTmp;
   strTmp = ini.GetValue("tests", "pop", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   POP_TEST_ENABLED = (strTmp == "YES") ? true : false;
   
   strTmp = ini.GetValue("tests", "pop-ssl", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   POP_SSL_TEST_ENABLED = (strTmp == "YES") ? true : false;
   
   strTmp = ini.GetValue("tests", "smtp", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);   
   SMTP_TEST_ENABLED = (strTmp == "YES") ? true : false;
   
   strTmp = ini.GetValue("tests", "smtp-ssl", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   SMTP_SSL_TEST_ENABLED = (strTmp == "YES") ? true : false;
   
   strTmp = ini.GetValue("tests", "imap", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   IMAP_TEST_ENABLED = (strTmp == "YES") ? true : false;
   
   strTmp = ini.GetValue("tests", "http-proxy", "");
   std::transform(strTmp.begin(), strTmp.end(), strTmp.begin(), ::toupper);
   HTTP_PROXY_TEST_ENABLED = (strTmp == "YES") ? true : false;

   // required when a build is generated with the macro DEBUG_CURL
   CURL_LOG_FOLDER = ini.GetValue("local", "curl_logs_folder", "");

   CERT_AUTH_FILE = ini.GetValue("local", "ca_file", "");
   SSL_CERT_FILE = ini.GetValue("local", "ssl_cert_file", "");
   SSL_KEY_FILE = ini.GetValue("local", "ssl_key_file", "");
   SSL_KEY_PWD = ini.GetValue("local", "ssl_key_pwd", "");

   PROXY_SERVER = ini.GetValue("http-proxy", "host", "");
   PROXY_SERVER_DISABLED = ini.GetValue("http-proxy", "host_dummy", "");

   POP_SERVER = ini.GetValue("pop", "host", "");
   POP_USERNAME = ini.GetValue("pop", "username", "");
   POP_PASSWORD = ini.GetValue("pop", "password", "");

   SSL_POP_SERVER = ini.GetValue("pop-ssl", "host", "");
   SSL_POP_USERNAME = ini.GetValue("pop-ssl", "username", "");
   SSL_POP_PASSWORD = ini.GetValue("pop-ssl", "password", "");
   
   IMAP_SERVER = ini.GetValue("imap", "host", "");
   IMAP_USERNAME = ini.GetValue("imap", "username", "");
   IMAP_PASSWORD = ini.GetValue("imap", "password", "");

   SMTP_SERVER = ini.GetValue("smtp", "host", "");
   SMTP_USERNAME = ini.GetValue("smtp", "username", "");
   SMTP_PASSWORD = ini.GetValue("smtp", "password", "");
   SMTP_FROM = ini.GetValue("smtp", "from", "");
   SMTP_TO = ini.GetValue("smtp", "to", "");
   SMTP_CC = ini.GetValue("smtp", "cc", "");

   SSL_SMTP_SERVER = ini.GetValue("smtp-ssl", "host", "");
   SSL_SMTP_USERNAME = ini.GetValue("smtp-ssl", "username", "");
   SSL_SMTP_PASSWORD = ini.GetValue("smtp-ssl", "password", "");
   SSL_SMTP_FROM = ini.GetValue("smtp-ssl", "from", "");
   SSL_SMTP_TO = ini.GetValue("smtp-ssl", "to", "");
   SSL_SMTP_CC = ini.GetValue("smtp-ssl", "cc", "");
   
   if (POP_TEST_ENABLED && POP_SERVER.empty()
       || POP_SSL_TEST_ENABLED && SSL_POP_SERVER.empty()
       || SMTP_TEST_ENABLED && SMTP_SERVER.empty()
       || SMTP_SSL_TEST_ENABLED && SSL_SMTP_SERVER.empty()
       || IMAP_TEST_ENABLED && IMAP_SERVER.empty()
       || HTTP_PROXY_TEST_ENABLED && (PROXY_SERVER.empty() || PROXY_SERVER_DISABLED.empty())
       )
   {
       std::clog << "[ERROR] Check your INI file parameters."
       " Disable tests that don't have a server value."
       << std::endl;
       return false;
   }

   return true;
}

void GlobalTestCleanUp(void)
{

   return;
}

int TestProgressCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded)
{
   // ensure that the file to be downloaded is not empty
   // because that would cause a division by zero error later on
   if (dTotalToDownload <= 0.0)
      return 0;

   // how wide you want the progress meter to be
   const int iTotalDots = 20;
   double dFractionDownloaded = dNowDownloaded / dTotalToDownload;
   // part of the progressmeter that's already "full"
   int iDots = round(dFractionDownloaded * iTotalDots);

   // create the "meter"
   int iDot = 0;
   std::cout << static_cast<unsigned>(dFractionDownloaded * 100) << "% [";

   // part  that's full already
   for (; iDot < iDots; iDot++)
      std::cout << "=";

   // remaining part (spaces)
   for (; iDot < iTotalDots; iDot++)
      std::cout << " ";

   // and back to line begin - do not forget the fflush to avoid output buffering problems!
   std::cout << "]           \r" << std::flush;

   // if you don't return 0, the transfer will be aborted - see the documentation
   return 0;
}
