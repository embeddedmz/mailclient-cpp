#include "gtest/gtest.h"   // Google Test Framework
#include "test_utils.h"    // Helpers for tests

// Test subjects (SUT)
#include "MAILClient.h"
#include "POPClient.h"
#include "SMTPClient.h"
#include "IMAPClient.h"

#define PRINT_LOG [](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl;  }

// Test parameters
extern bool POP_TEST_ENABLED;
extern bool POP_SSL_TEST_ENABLED;
extern bool SMTP_TEST_ENABLED;
extern bool SMTP_SSL_TEST_ENABLED;
extern bool IMAP_TEST_ENABLED;
extern bool HTTP_PROXY_TEST_ENABLED;

extern std::string CURL_LOG_FOLDER;
extern std::string CERT_AUTH_FILE;
extern std::string SSL_CERT_FILE;
extern std::string SSL_KEY_FILE;
extern std::string SSL_KEY_PWD;

extern std::string PROXY_SERVER;
extern std::string PROXY_SERVER_DISABLED;

extern std::string POP_SERVER;
extern std::string POP_USERNAME;
extern std::string POP_PASSWORD;

extern std::string SSL_POP_SERVER;
extern std::string SSL_POP_USERNAME;
extern std::string SSL_POP_PASSWORD;

extern std::string IMAP_SERVER;
extern std::string IMAP_USERNAME;
extern std::string IMAP_PASSWORD;

extern std::string SMTP_SERVER;
extern std::string SMTP_USERNAME;
extern std::string SMTP_PASSWORD;
extern std::string SMTP_TO;
extern std::string SMTP_FROM;
extern std::string SMTP_CC;

extern std::string SSL_SMTP_SERVER;
extern std::string SSL_SMTP_USERNAME;
extern std::string SSL_SMTP_PASSWORD;
extern std::string SSL_SMTP_TO;
extern std::string SSL_SMTP_FROM;
extern std::string SSL_SMTP_CC;

extern std::mutex g_mtxConsoleMutex;

namespace
{
// Fixture for POP tests
class POPClientTest : public ::testing::Test
{
protected:
   std::unique_ptr<CPOPClient> m_pPOPClient;
   POPClientTest() : m_pPOPClient(nullptr)
   {
      CMailClient::SetCertificateFile(CERT_AUTH_FILE);
         
      #ifdef DEBUG_CURL
      CMailClient::SetCurlTraceLogDirectory(CURL_LOG_FOLDER);
      #endif
   }
   virtual ~POPClientTest() { }
   virtual void SetUp()
   {
      m_pPOPClient.reset(new CPOPClient(PRINT_LOG));
   }
   
   virtual void TearDown()
   {
      m_pPOPClient->CleanupSession();
      m_pPOPClient.reset();
   }
};

// Fixture for SMTP tests
class SMTPClientTest : public ::testing::Test
{
protected:
   std::unique_ptr<CSMTPClient> m_pSMTPClient;
   SMTPClientTest() : m_pSMTPClient(nullptr)
   {
      CMailClient::SetCertificateFile(CERT_AUTH_FILE);

#ifdef DEBUG_CURL
      CMailClient::SetCurlTraceLogDirectory(CURL_LOG_FOLDER);
#endif
   }
   virtual ~SMTPClientTest() { }
   virtual void SetUp()
   {
      m_pSMTPClient.reset(new CSMTPClient(PRINT_LOG));
   }

   virtual void TearDown()
   {
      m_pSMTPClient->CleanupSession();
      m_pSMTPClient.reset();
   }
};

// Fixture for IMAP tests
class IMAPClientTest : public ::testing::Test
{
protected:
   std::unique_ptr<CIMAPClient> m_pIMAPClient;
   IMAPClientTest() : m_pIMAPClient(nullptr)
   {
      CMailClient::SetCertificateFile(CERT_AUTH_FILE);

#ifdef DEBUG_CURL
      CMailClient::SetCurlTraceLogDirectory(CURL_LOG_FOLDER);
#endif
   }
   virtual ~IMAPClientTest() { }
   virtual void SetUp()
   {
      m_pIMAPClient.reset(new CIMAPClient(PRINT_LOG));
   }

   virtual void TearDown()
   {
      m_pIMAPClient->CleanupSession();
      m_pIMAPClient.reset();
   }
};

   // Unit tests

// Tests without a fixture (testing the common features of the base class)
TEST(MailClient, TestSession)
{
   CMailClient MailClient(PRINT_LOG);

   EXPECT_TRUE(MailClient.GetUsername().empty());
   EXPECT_TRUE(MailClient.GetPassword().empty());
   EXPECT_TRUE(MailClient.GetURL().empty());
   EXPECT_TRUE(MailClient.GetProxy().empty());
   EXPECT_TRUE(MailClient.GetSSLCertFile().empty());
   EXPECT_TRUE(MailClient.GetSSLKeyFile().empty());
   EXPECT_TRUE(MailClient.GetSSLKeyPwd().empty());
   EXPECT_TRUE(CMailClient::GetCertificateFile().empty());
   
   EXPECT_EQ(0, MailClient.GetTimeout());
   
   EXPECT_TRUE(MailClient.GetCurlPointer() == nullptr);
   
   EXPECT_EQ(CMailClient::SettingsFlag::ALL_FLAGS, MailClient.GetFlags());
   EXPECT_EQ(CMailClient::SslTlsFlag::NO_SSLTLS, MailClient.GetSslTlsFlags());

   ASSERT_TRUE(MailClient.InitSession("localhost", "foobar", "magic",
               CMailClient::SettingsFlag::ENABLE_LOG | CMailClient::SettingsFlag::VERIFY_PEER,
               CMailClient::SslTlsFlag::ENABLE_TLS));
   EXPECT_EQ(CMailClient::SettingsFlag::ENABLE_LOG | CMailClient::SettingsFlag::VERIFY_PEER, MailClient.GetSettingsFlags());
   EXPECT_TRUE(MailClient.GetCurlPointer() != nullptr);
   MailClient.SetProxy("my_proxy");
   CMailClient::SetCertificateFile("ca.pem");
   MailClient.SetSSLCertFile("file.cert");
   MailClient.SetSSLKeyFile("key.key");
   MailClient.SetSSLKeyPassword("passphrase");
   MailClient.SetTimeout(10);

   EXPECT_STREQ("localhost", MailClient.GetURL().c_str());
   EXPECT_STREQ("foobar", MailClient.GetUsername().c_str());
   EXPECT_STREQ("magic", MailClient.GetPassword().c_str());

   EXPECT_STREQ("http://my_proxy", MailClient.GetProxy().c_str());
   EXPECT_STREQ("ca.pem", CMailClient::GetCertificateFile().c_str());
   EXPECT_STREQ("file.cert", MailClient.GetSSLCertFile().c_str());
   EXPECT_STREQ("key.key", MailClient.GetSSLKeyFile().c_str());
   EXPECT_STREQ("passphrase", MailClient.GetSSLKeyPwd().c_str());
   
   EXPECT_EQ(10, MailClient.GetTimeout());
   
   MailClient.SetProgressFnCallback(nullptr, &TestProgressCallback);
   EXPECT_EQ(&TestProgressCallback, *MailClient.GetProgressFnCallback());
   EXPECT_EQ(nullptr, MailClient.GetProgressFnCallbackOwner());

   EXPECT_TRUE(MailClient.CleanupSession());
}

TEST(MailClient, TestDoubleInitializingSession)
{
   CMailClient MailClient([](const std::string& strMsg) { std::cout << strMsg << std::endl; });

   ASSERT_TRUE(MailClient.InitSession("localhost", "foobar", "*****"));
   EXPECT_FALSE(MailClient.InitSession("localhost", "foobar", "*****"));
   EXPECT_TRUE(MailClient.CleanupSession());
}

TEST(MailClient, TestDoubleCleanUp)
{
   CMailClient MailClient(PRINT_LOG);

   ASSERT_TRUE(MailClient.InitSession("localhost", "foobar", "*****"));
   EXPECT_TRUE(MailClient.CleanupSession());
   EXPECT_FALSE(MailClient.CleanupSession());
}

TEST(MailClient, TestMultithreading)
{
   const char* arrDataArray[3] = { "Thread 1", "Thread 2", "Thread 3" };

   auto ThreadFunction = [](const char* pszThreadName)
   {
      CMailClient MailClient([](const std::string& strMsg) { std::cout << strMsg << std::endl; });
      if (pszThreadName != nullptr)
      {
         g_mtxConsoleMutex.lock();
         std::cout << pszThreadName << std::endl;
         g_mtxConsoleMutex.unlock();
      }
   };

   std::thread FirstThread(ThreadFunction, arrDataArray[0]);
   std::thread SecondThread(ThreadFunction, arrDataArray[1]);
   std::thread ThirdThread(ThreadFunction, arrDataArray[2]);

   // synchronize threads
   FirstThread.join();                 // pauses until first finishes
   SecondThread.join();                // pauses until second finishes
   ThirdThread.join();                 // pauses until third finishes
}

// SMTP Tests

TEST_F(SMTPClientTest, TestVerifyAddress)
{
   ASSERT_TRUE(m_pSMTPClient->InitSession(SSL_SMTP_SERVER, SSL_SMTP_USERNAME, SSL_SMTP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (SMTP_SSL_TEST_ENABLED)
   {
      EXPECT_TRUE(m_pSMTPClient->VerifyAddress(SSL_SMTP_FROM));
   }
   else
      std::cout << "SMTP (without SSL/TLS) tests are disabled !" << std::endl;
}

// SMTP Client tests
TEST_F(SMTPClientTest, TestSendStringMailSSL)
{
   ASSERT_TRUE(m_pSMTPClient->InitSession(SSL_SMTP_SERVER, SSL_SMTP_USERNAME, SSL_SMTP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (SMTP_SSL_TEST_ENABLED)
   {
      std::ostringstream ssTimestamp;
      TimeStampTest(ssTimestamp);
      /* LF must be used to separate lines, they will be replaced with CRLF when sending the mail */
      std::string strMail = "Subject: Unit Test TestSendStringMailSSL executed on "
         + ssTimestamp.str() + "\n"
         + "\n" /* empty line to divide headers from body, see RFC5322 */
         + "This email is sent via MailClient-C++ API.\n"
         + "\n"
         + "If you receive this mail, that means that the unit test is passed.\n"
         + "It could be a lot of lines, could be MIME encoded, whatever.\n"
         + "Check RFC5322.\n";

      EXPECT_TRUE(m_pSMTPClient->SendString(SSL_SMTP_FROM, SSL_SMTP_TO, SSL_SMTP_CC, strMail));
   }
   else
      std::cout << "SMTP (with SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(SMTPClientTest, TestSendFileMailSSL)
{
   ASSERT_TRUE(m_pSMTPClient->InitSession(SSL_SMTP_SERVER, SSL_SMTP_USERNAME, SSL_SMTP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (SMTP_SSL_TEST_ENABLED)
   {
      std::ostringstream ssTimestamp;
      TimeStampTest(ssTimestamp);

      // create dummy test file
      std::ofstream ofTestEmail("test_email.txt");
      ASSERT_TRUE(static_cast<bool>(ofTestEmail));

      ofTestEmail << "Subject: Unit Test TestSendFileMailSSL executed on "
         + ssTimestamp.str() + "\n"
         + "\n" /* empty line to divide headers from body, see RFC5322 */
         + "This email is sent via MailClient-C++ API.\n"
         + "\n"
         + "If you receive this mail, that means that the unit test is passed.\n"
         + "It could be a lot of lines, could be MIME encoded, whatever.\n"
         + "Check RFC5322.\n";
      ASSERT_TRUE(static_cast<bool>(ofTestEmail));
      ofTestEmail.close();

      EXPECT_TRUE(m_pSMTPClient->SendFile(SSL_SMTP_FROM, SSL_SMTP_TO, SSL_SMTP_CC, "test_email.txt"));

      // delete test file
      EXPECT_TRUE(remove("test_email.txt") == 0);
   }
   else
      std::cout << "SMTP (with SSL/TLS) tests are disabled !" << std::endl;
}

// POP Client tests

TEST_F(POPClientTest, TestListMail)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(POP_SERVER, POP_USERNAME, POP_PASSWORD,
      CMailClient::SettingsFlag::ENABLE_LOG, CMailClient::SslTlsFlag::NO_SSLTLS));

   if (POP_TEST_ENABLED)
   {
      std::string strMailList;
      
      EXPECT_TRUE(m_pPOPClient->List(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "POP (without SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(POPClientTest, TestListMailSSL)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(SSL_POP_SERVER, SSL_POP_USERNAME, SSL_POP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (POP_SSL_TEST_ENABLED)
   {
      std::string strMailList;
      
      EXPECT_TRUE(m_pPOPClient->List(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "POP (with SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(POPClientTest, TestListUIDLMailSSL)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(SSL_POP_SERVER, SSL_POP_USERNAME, SSL_POP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (POP_SSL_TEST_ENABLED)
   {
      std::string strMailList;
      
      EXPECT_TRUE(m_pPOPClient->ListUIDL(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "POP (with SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(POPClientTest, TestGetMailStringSSL)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(SSL_POP_SERVER, SSL_POP_USERNAME, SSL_POP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (POP_SSL_TEST_ENABLED)
   {
      /* Mailbox must contain at least one e-mail to pass this test */
      std::string strEmail;
      
      // get the mail no 1
      EXPECT_TRUE(m_pPOPClient->GetString("1", strEmail));
      EXPECT_FALSE(strEmail.empty());
   }
   else
      std::cout << "POP (with SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(POPClientTest, TestGetMailFileSSL)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(SSL_POP_SERVER, SSL_POP_USERNAME, SSL_POP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (POP_SSL_TEST_ENABLED)
   {
      /* Mailbox must contain at least one mail to pass this test */
      
      bool bRes;

      // get the mail no 1
      EXPECT_TRUE(bRes = m_pPOPClient->GetFile("1", "email_1.txt"));

      // delete test file
      if (bRes)
         EXPECT_TRUE(remove("email_1.txt") == 0);
   }
   else
      std::cout << "POP (with SSL/TLS) tests are disabled !" << std::endl;
}

TEST_F(POPClientTest, TestProxy)
{
   ASSERT_TRUE(m_pPOPClient->InitSession(SSL_POP_SERVER, SSL_POP_USERNAME, SSL_POP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (POP_SSL_TEST_ENABLED && HTTP_PROXY_TEST_ENABLED)
   {
      std::string strMailList;

      m_pPOPClient->SetProxy(PROXY_SERVER);

      EXPECT_TRUE(m_pPOPClient->ListUIDL(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "HTTP Proxy tests are disabled !" << std::endl;
}

// IMAP Client tests

TEST_F(IMAPClientTest, TestListINBOX)
{
   ASSERT_TRUE(m_pIMAPClient->InitSession(IMAP_SERVER, IMAP_USERNAME, IMAP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (IMAP_TEST_ENABLED)
   {
      std::string strMailList;
      
      EXPECT_TRUE(m_pIMAPClient->List(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "IMAP tests are disabled !" << std::endl;
}

TEST_F(IMAPClientTest, TestListSubFolderSSL)
{
   ASSERT_TRUE(m_pIMAPClient->InitSession(IMAP_SERVER, IMAP_USERNAME, IMAP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (IMAP_TEST_ENABLED)
   {
      std::string strMailList;

      EXPECT_TRUE(m_pIMAPClient->ListSubFolders(strMailList));
      EXPECT_FALSE(strMailList.empty());
   }
   else
      std::cout << "IMAP tests are disabled !" << std::endl;
}

TEST_F(IMAPClientTest, TestGetMailStringSSL)
{
   ASSERT_TRUE(m_pIMAPClient->InitSession(IMAP_SERVER, IMAP_USERNAME, IMAP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (IMAP_TEST_ENABLED)
   {
      m_pIMAPClient->SetProgressFnCallback(nullptr, &TestProgressCallback);

      std::string strTmp;
      // Get the email 
      EXPECT_TRUE(m_pIMAPClient->GetString("1", strTmp));
      std::cout << std::endl; // to avoid erasing the console progress bar
   }
   else
      std::cout << "IMAP tests are disabled !" << std::endl;
}

TEST_F(IMAPClientTest, TestGetMailFileSSL)
{
   ASSERT_TRUE(m_pIMAPClient->InitSession(IMAP_SERVER, IMAP_USERNAME, IMAP_PASSWORD,
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL));

   if (IMAP_TEST_ENABLED)
   {  
      bool bRes;

      m_pIMAPClient->SetProgressFnCallback(nullptr, &TestProgressCallback);

      /* Mailbox must contain at least one mail to pass this test */
      EXPECT_TRUE(bRes = m_pIMAPClient->GetFile("1", "email_1.txt"));

      // to avoid erasing the console progress bar
      std::cout << std::endl;

      // delete test file
      if (bRes)
      {
         EXPECT_TRUE(remove("email_1.txt") == 0);
      }
   }
   else
      std::cout << "IMAP tests are disabled !" << std::endl;
}

} // namespace

int main(int argc, char **argv)
{
   if (argc > 1 && GlobalTestInit(argv[1]))
   {
      ::testing::InitGoogleTest(&argc, argv);
      int iTestResults = RUN_ALL_TESTS();

      GlobalTestCleanUp();

      return iTestResults;
   }
   else
   {
      std::cerr << "[ERROR] Encountered an error while loading test parameters from INI file !" << std::endl;
      return 1;
   }
}
