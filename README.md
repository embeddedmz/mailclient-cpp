# Mail client for C++ (POP, SMTP, IMAP)
[![MIT license](https://img.shields.io/badge/license-MIT-blue.svg)](http://opensource.org/licenses/MIT)


## About
This is a simple Mail client for C++ 14. It wraps libcurl for POP, SMTP and IMAP requests and meant to be a portable
and easy-to-use API to perform e-mail related operations.

Compilation has been tested with:
- GCC 5.4.0 (GNU/Linux Ubuntu 16.04 LTS)
- Microsoft Visual Studio 2015 (Windows 10)

Underlying libraries:
- [libcurl](http://curl.haxx.se/libcurl/)

## Usage
Create an object and provide to its constructor a callable object (for log printing) having this signature :

```cpp
void(const std::string&)
```

You can disable log printing by avoiding the flag CMailClient::SettingsFlag::ENABLE_LOG when initializing a session.

```cpp
#include "SMTPClient.h"

CSMTPClient SMTPClient([](const std::string&){ return; });
```

Before performing one or more requests, a session must be initialized with server parameters (SMTP, POP or IMAP).

```cpp
SMTPClient.InitSession("smtp.gmail.com:465", "username@gmail.com", "password",
			CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);
```

You can also set parameters such as the time out (in seconds), the HTTP proxy server etc... before sending
your request.

To send a string or a file as an email :

```cpp
/* string to send : use LF (\n) instead of CRLF (\r\n) */
std::string strMail = "Subject: Test SMTP Client\n"
         + "\n" /* empty line to divide headers from body, see RFC5322 */
         + "This email is sent via MailClient-C++ API.\n";

/* send string as mail - third parameter is optional CC (copy carbon) */
bool bResSendString = SMTPClient.SendString("<foo@gmail.com>", "<toto@yahoo.com>", "", strMail);

/* or send a file as an e-mail */
bool bResSendFile = SMTPClient.SendFile("<foo@gmail.com>", "<toto@yahoo.com>", "", "test_email.txt");

/* bResSendString or bResSendFile are true if the requests are successfully performed */
```

To retrieve a mail from an IMAP or a POP server and save it in a string or a file :

```cpp
/* we need a CPOPClient object */
CPOPClient POPClient([](const std::string& strLogMsg) { std::cout << strLogMsg << std::endl;  })

POPClient.InitSession("pop.gmail.com:995", "username@gmail.com", "password",
      CMailClient::SettingsFlag::ALL_FLAGS, CMailClient::SslTlsFlag::ENABLE_SSL);

std::string strEmail;

/* retrieve the mail number 1 and store it in strEmail */
bool bResRcvStr = POPClient.GetString("1", strEmail);

/* or store it in a file*/
bool bResRcvStr = POPClient.GetFile("1", "email_no_1.txt");
```

The same can be done with a CIMAPClient object (retrieve a mail from an IMAP server.)

Finally cleanup can be done automatically if the object goes out of scope. If the log printing is enabled,
a warning message will be printed. Or you can cleanup manually by calling CleanupSession() :

```cpp
bool bRes = SMTPClient.CleanupSession();

/* if the operation succeeds, true will be returned. */
```

After cleaning the session, if you want to reuse the object, you need to re-initialize it with the
proper method.

There's also POP/IMAP methods to list the mailbox etc... This section can be extended in the future
to demonstrate the most useful methods.

## Callback to a Progress Function

A pointer or a callable object (lambda, functor etc...) to of a progress meter function, which should match the prototype shown below, can be passed to a CMailClient object.

```cpp
int ProgCallback(void* ptr, double dTotalToDownload, double dNowDownloaded, double dTotalToUpload, double dNowUploaded);
```

This function gets called by libcurl instead of its internal equivalent with a frequent interval. While data is being transferred it will be called very frequently, and during slow periods like when nothing is being transferred it can slow down to about one call per second.

Returning a non-zero value from this callback will cause libcurl to abort the transfer.

The unit test "TestGetMailStringSSL" already demonstrates how to use a progress function to display a progress bar on console.

## Thread Safety

A mutex is used to increment/decrement atomically the count of CMailClient objects (POP, IMAP and SMTP).

`curl_global_init` is called when the count reaches one and `curl_global_cleanup` is called when the counter
become zero.

Do not share CMailClient objects across threads as this would mean accessing libcurl handles from multiple threads
at the same time which is not allowed.

The method SetNoSignal can be set to skip all signal handling. This is important in multi-threaded applications as DNS
resolution timeouts use signals. The signal handlers quite readily get executed on other threads.

## HTTP Proxy Tunneling Support

An HTTP Proxy can be set to use for the upcoming request.
To specify a port number, append :[port] to the end of the host name. If not specified, `libcurl` will default to using port 1080 for proxies. The proxy string may be prefixed with `http://` or `https://`. If no HTTP(S) scheme is specified, the address provided to `libcurl` will be prefixed with `http://` to specify an HTTP proxy. A proxy host string can embedded user + password.
The operation will be tunneled through the proxy as curl option `CURLOPT_HTTPPROXYTUNNEL` is enabled by default.
A numerical IPv6 address must be written within [brackets].

```cpp
POPClient.SetProxy("https://37.187.100.23:3128");

/* or you can set it without the protocol scheme and
http:// will be prefixed by default */
POPClient.SetProxy("37.187.100.23:3128");

/* the following request will be tunneled through the proxy */
std::string strMailList;
bool bRes = POPClient.ListUIDL(strMailList);
```

## Installation

You will need CMake to generate a makefile for the static library or to build the tests/code coverage program.

Also make sure you have libcurl and Google Test installed.

You can follow this script https://gist.github.com/fideloper/f72997d2e2c9fbe66459 to install libcurl.

This tutorial will help you installing properly Google Test on Ubuntu: https://www.eriksmistad.no/getting-started-with-google-test-on-ubuntu/

The CMake script located in the tree will produce Makefiles for the creation of the static library and for the unit tests program.

To create a debug static library and a test binary, change directory to the one containing the first CMakeLists.txt and :

```Shell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE:STRING=Debug
make
```

To create a release static library, just change "Debug" by "Release".

The library will be found under "build/[Debug|Release]/lib/libmailclient.a" whereas the test program will be located in "build/[Debug|Release]/bin/test_mailclient"

To directly run the unit test binary, you must indicate the path of the INI conf file (see the section below)
```Shell
./[Debug|Release]/bin/test_mailclient /path_to_your_ini_file/conf.ini
```

### Building under Windows via Visual Studio

This can be enhanced in future...

First of all, build libcurl using this fork of build-libcurl-windows : https://github.com/ribtoks/build-libcurl-windows

In fact, this fork will allow you to build libcurl under Visual Studio 2017.

```Shell
git clone https://github.com/ribtoks/build-libcurl-windows.git
```

Then just build libcurl using :

```Shell
build.bat
```

This batch script will automatically download the latest libcurl source code and build it using the most recent Visual Studio compiler
that it will find on your computer. For a particular version, you can modify the batch script...

Under YOUR_DIRECTORY\build-libcurl-windows\third-party\libcurl, you will find the curl include directory and a lib directory containing different type of libraries : dynamic and static x86 and x64 libraries compiled in Debug and Release mode (8 libraries). Later, we will be using the libraries located in lib\dll-debug-x64 and lib\dll-release-x64 as an example.

Concerning Google Test, the library will be downloaded and built automatically from its github repository. Someone with enough free time can do the same for libcurl and submit a pull request...

Download and install the latest version of CMake : https://cmake.org/download/ (e.g. Windows win64-x64 Installer)

Open CMake (cmake-gui)

In "Where is the source code", put the mailclient-cpp path (e.g. C:/Users/Amine/Documents/Work/PROJECTS/GitHub/mailclient-cpp), where the main CMakeLists.txt file exist.

In "Where to build binaries", paste the directory where you want to build the project (e.g. C:/Users/Amine/Documents/Work/PROJECTS/GitHub/mailclient_build)

Click on "Configure". After some time, an error message will be shown, that's normal as CMake is unable to find libcurl.

Make sure "Advanced" checkbox is checked, in the list, locate the variables prefixed with "CURL_" and update them with the path of libcurl include directory and libraries, for example :

CURL_INCLUDE_DIR C:\LIBS\build-libcurl-windows\third-party\libcurl\include
CURL_LIBRARY_DEBUG C:\LIBS\build-libcurl-windows\third-party\libcurl\lib\dll-debug-x64\libcurl_debug.lib
CURL_LIBRARY_RELEASE C:\LIBS\build-libcurl-windows\third-party\libcurl\lib\dll-release-x64\libcurl.lib

Click on "Configure" again ! You should not have errors this time. You can ignore the warning related to the test configuration file.

Then click on "Generate", you can choose a Visual Studio version if it is not done before (e.g. Visual Studio 15 2017 Win64, I think it should be the same version used to build libcurl...)

Finally, click on "Open Project" to open the solution in Visual Studio.

In Visual Studio, you can change the build type (Debug -> Release). Build the solution (press F7). It must succeed without any errors. You can close Visual Studio.

The library will be found under C:\Users\Amine\Documents\Work\PROJECTS\GitHub\mailclient_build\lib\Release]\mailclient.lib

After building a program using "mailclient.lib", do not forget to copy libcurl DLL in the directory where the program binary is located.

For example, in the build directory (e.g. C:\Users\Amine\Documents\Work\PROJECTS\GitHub\mailclient_build), under "bin", directory, you may find "Debug", "Release" or both according to the build type used during the build in Visual Studio, and in it, the test program "test_mailclient.exe". Before executing it, make sure to copy the libcurl DLL in the same directory (e.g. copy C:\LIBS\build-libcurl-windows\third-party\libcurl\lib\dll-release-x64\libcurl.dll and the PDB file too if you want, do not change the name of the DLL !) The type of the library MUST correspond to the type of the .lib file fed to CMake-gui !

If you want to run the test program, in the command line, launch test_mailclient.exe with the path of you test configuration file (INI) :

```Shell
C:\Users\Amine\Documents\Work\PROJECTS\GitHub\mailclient_build\bin\[Debug | Release]\test_mailclient.exe PATH_TO_YOUR_TEST_CONF_FILE\conf.ini
```

If you have a gmail account, modify the INI file to enable only SSL tests (POP, IMAP, SMTP) and in your GMAIL settings allow less secure applications to acces your GMAIL. "Username", "password", "from" and "to" are the fields that you have to fill to run the tests. You don't need to fill the keys below "[local]" section !

Below, you can find the addresses of Google POP, SMTP and IMAP servers :

```ini
[pop-ssl]
host=pop.gmail.com:995

[smtp-ssl]
host=smtp.gmail.com:465
...

[imap]
host=imap.gmail.com:993
...
```

## Run Unit Tests

[simpleini](https://github.com/brofield/simpleini) is used to gather unit tests parameters from
an INI configuration file. You need to fill that file with your SMTP, POP and IMAP parameters.
You can also disable some tests (POP, SMTP with or witout SSL/TLS or IMAP tests) and indicate
parameters only for the enabled tests. A template of the INI file exists already under TestMAIL/

Example : (Run only SMTP with SSL tests)

```ini
[tests]
; POP without SSL/TLS are disabled
pop=no
; POP with SSL/TLS are disabled
pop-ssl=no
; SMTP without SSL/TLS are disabled
smtp=no
; SMTP with SSL/TLS are enabled
smtp-ssl=yes
; IMAP with SSL/TLS are disabled
imap=no
; HTTP Proxy tests are disabled
http-proxy=no

[smtp-ssl]
host=smtp.gmail.com:465
username=foo.bar@gmail.com
password=AqPx58#w
from=<foo.bar@gmail.com>
to=<toto.africa@yahoo.com>
cc=
```

You can also generate an XML file of test results by adding --getst_output argument when calling the test program

```Shell
./[Debug|Release]/bin/test_mailclient /path_to_your_ini_file/conf.ini --gtest_output="xml:./TestMail.xml"
```

An alternative way to compile and run unit tests :

```Shell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DTEST_INI_FILE="full_or_relative_path_to_your_test_conf.ini"
make
make test
```

You may use a tool like https://github.com/adarmalik/gtest2html to convert your XML test result in an HTML file.

## Memory Leak Check

Visual Leak Detector has been used to check memory leaks with the Windows build (Visual Sutdio 2015)
You can download it here: https://vld.codeplex.com/

To perform a leak check with the Linux build, you can do so :

```Shell
valgrind --leak-check=full ./Debug/bin/test_mailclient /path_to_ini_file/conf.ini
```

## Code Coverage

The code coverage build doesn't use the static library but compiles and uses directly the 
MailClient-C++ API in the test program.

```Shell
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Coverage -DCOVERAGE_INI_FILE:STRING="full_path_to_your_test_conf.ini"
make
make coverage_mailclient
```

If everything is OK, the results will be found under ./TestMAIL/coverage/index.html

Make sure you feed CMake with a full path to your test conf INI file, otherwise, the coverage test
will be useless.

Under Visual Studio, you can simply use OpenCppCoverage (https://opencppcoverage.codeplex.com/)

## Contribute
All contributions are highly appreciated. This includes updating documentation, writing code and unit tests
to increase code coverage and adding tools (a proper build for Windows etc...).

Try to preserve the existing coding style (Hungarian notation, indentation etc...).

## Issues

### Compiling with the macro DEBUG_CURL

If you compile the test program with the preprocessor macro DEBUG_CURL, to enable curl debug informations,
the static library used must also be compiled with that macro. Don't forget to mention a path where to store
log files in the INI file if you want to use that feature in the unit test program (curl_logs_folder under [local])

### File names format when compiling with Visual Studio (Windows users)

It is assumed that the FTP server is supporting UTF-8. You must feed the FTP client API with paths/file names encoded in UTF-8 and NOT in ANSI (Windows-1252 on Western/U.S. systems but iy can represent certain other Windows code pages on other systems, ANSI is just an extension for ASCII). Look at the unit tests for examples (look for the preprocessor macro WINDOWS to find them quickly).

If you limit to ASCII characters, you don't need to convert your ANSI strings to UTF-8.
