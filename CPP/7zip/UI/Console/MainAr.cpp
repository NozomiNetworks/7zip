// MainAr.cpp

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#define NOZOMI_7Z_VERSION "0.0.8"

#include "StdAfx.h"

#ifdef _WIN32
#include "../../../../C/DllSecur.h"
#endif
#include "../../../../C/CpuArch.h"

#include "../../../Common/MyException.h"
#include "../../../Common/StdOutStream.h"
#include "../../../Common/StdInStream.h"

#include "../../../Windows/ErrorMsg.h"
#include "../../../Windows/NtCheck.h"

#include "../Common/ArchiveCommandLine.h"
#include "../Common/ExitCode.h"

#include "ConsoleClose.h"

using namespace NWindows;

extern
CStdOutStream *g_StdStream;
CStdOutStream *g_StdStream = NULL;
extern
CStdInStream *g_StdInStream;
CStdInStream *g_StdInStream = NULL;
extern
CStdOutStream *g_ErrStream;
CStdOutStream *g_ErrStream = NULL;

extern int Main2(
  #ifndef _WIN32
  int numArgs, char *args[]
  #endif
);

static const char * const kException_CmdLine_Error_Message = "Command Line Error:";
static const char * const kExceptionErrorMessage = "ERROR:";
static const char * const kUserBreakMessage  = "Break signaled";
static const char * const kMemoryExceptionMessage = "ERROR: Can't allocate required memory!";
static const char * const kUnknownExceptionMessage = "Unknown Error";
static const char * const kInternalExceptionMessage = "\n\nInternal Error #";

static void FlushStreams()
{
  if (g_StdStream)
    g_StdStream->Flush();
}

static void PrintError(const char *message)
{
  FlushStreams();
  if (g_ErrStream)
    *g_ErrStream << "\n\n" << message << endl;
}

#if defined(_WIN32) && defined(_UNICODE) && !defined(_WIN64) && !defined(UNDER_CE)
#define NT_CHECK_FAIL_ACTION *g_StdStream << "Unsupported Windows version"; return NExitCode::kFatalError;
#endif

static inline bool CheckIsa()
{
  // __try
  {
    // some compilers (e2k) support SSE/AVX, but cpuid() can be unavailable or return lower isa support
#ifdef MY_CPU_X86_OR_AMD64
    #if 0 && (defined(__AVX512F__) && defined(__AVX512VL__))
      if (!CPU_IsSupported_AVX512F_AVX512VL())
        return false;
    #elif defined(__AVX2__)
      if (!CPU_IsSupported_AVX2())
        return false;
    #elif defined(__AVX__)
      if (!CPU_IsSupported_AVX())
        return false;
    #elif defined(__SSE2__) && !defined(MY_CPU_AMD64) || defined(_M_IX86_FP) && (_M_IX86_FP >= 2)
      if (!CPU_IsSupported_SSE2())
        return false;
    #elif defined(__SSE__) && !defined(MY_CPU_AMD64) || defined(_M_IX86_FP) && (_M_IX86_FP >= 1)
      if (!CPU_IsSupported_SSE() ||
          !CPU_IsSupported_CMOV())
        return false;
    #endif
#endif
    /*
    __asm
    {
      _emit 0fH
      _emit 038H
      _emit 0cbH
      _emit (0c0H + 0 * 8 + 0)
    }
    */
    return true;
  }
  /*
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return false;
  }
  */
}

int MainWrapper(int numArgs, char *args[]);
int MainLoop(int numArgs, char *args[]);

int Z7_CDECL main
(
  #ifndef _WIN32
  int numArgs, char *args[]
  #endif
)
{
  g_ErrStream = &g_StdErr;
  g_StdStream = &g_StdOut;

  // #if (defined(_MSC_VER) && defined(_M_IX86))
  if (!CheckIsa())
  {
    PrintError("ERROR: processor doesn't support required ISA extension");
    return NExitCode::kFatalError;
  }
  // #endif

  NT_CHECK

  NConsoleClose::CCtrlHandlerSetter ctrlHandlerSetter;

  const int mainLoopRetVal = MainLoop(numArgs, args);
  if (mainLoopRetVal == -1)
  {
    return MainWrapper(numArgs, args);
  }

  return mainLoopRetVal;
}

int MainWrapper(int numArgs, char *args[])
{
  int res = 0;

  try
  {
    #ifdef _WIN32
    My_SetDefaultDllDirectories();
    #endif

    res = Main2(
    #ifndef _WIN32
    numArgs, args
    #endif
    );
  }
  catch(const CNewException &)
  {
    PrintError(kMemoryExceptionMessage);
    return (NExitCode::kMemoryError);
  }
/*
  catch(const NConsoleClose::CCtrlBreakException &)
  {
    PrintError(kUserBreakMessage);
    return (NExitCode::kUserBreak);
  }
*/
  catch(const CMessagePathException &e)
  {
    PrintError(kException_CmdLine_Error_Message);
    if (g_ErrStream)
      *g_ErrStream << e << endl;
    return (NExitCode::kUserError);
  }
  catch(const CSystemException &systemError)
  {
    if (systemError.ErrorCode == E_OUTOFMEMORY)
    {
      PrintError(kMemoryExceptionMessage);
      return (NExitCode::kMemoryError);
    }
    if (systemError.ErrorCode == E_ABORT)
    {
      PrintError(kUserBreakMessage);
      return (NExitCode::kUserBreak);
    }
    if (g_ErrStream)
    {
      PrintError("System ERROR:");
      *g_ErrStream << NError::MyFormatMessage(systemError.ErrorCode) << endl;
    }
    return (NExitCode::kFatalError);
  }
  catch(NExitCode::EEnum exitCode)
  {
    FlushStreams();
    if (g_ErrStream)
      *g_ErrStream << kInternalExceptionMessage << exitCode << endl;
    return (exitCode);
  }
  catch(const UString &s)
  {
    if (g_ErrStream)
    {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  }
  catch(const AString &s)
  {
    if (g_ErrStream)
    {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  }
  catch(const char *s)
  {
    if (g_ErrStream)
    {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  }
  catch(const wchar_t *s)
  {
    if (g_ErrStream)
    {
      PrintError(kExceptionErrorMessage);
      *g_ErrStream << s << endl;
    }
    return (NExitCode::kFatalError);
  }
  catch(int t)
  {
    if (g_ErrStream)
    {
      FlushStreams();
      *g_ErrStream << kInternalExceptionMessage << t << endl;
      return (NExitCode::kFatalError);
    }
  }
  catch(...)
  {
    PrintError(kUnknownExceptionMessage);
    return (NExitCode::kFatalError);
  }

  return res;
}

int MainLoop(int numArgs, char *args[])
{
  if (numArgs == 2)
  {
    if (std::string(args[1]) == "iv")
    {
      std::cout << NOZOMI_7Z_VERSION << std::endl;
      return 0;
    }
  }
  else if (numArgs == 3)
  {
    if (std::string(args[1]) == "in")
    {
      // args[2] is a full-path of a temporary file that was used as a lock.
      // might be repurposed later

      // const std::string lockFilePath = args[2];
      std::string buffer;
      while (std::getline(std::cin, buffer))
      {
        if (buffer == "exit")
        {
          return 0;
        }
        else
        {
          std::size_t pos = 0;
          std::string arg;
          std::string s(buffer);
          const std::string delimiter = " ";
          std::vector<std::string> parts;
          bool foundLast = false;
          while ((pos = s.find(delimiter)) != std::string::npos && !foundLast)
          {
            if (s.find('\"') < pos)
            {
              foundLast = true;
            }
            else
            {
              arg = s.substr(0, pos);
              parts.push_back(arg);
              s.erase(0, pos + delimiter.length());
            }
          }
          if (s.size() > 2 && s[0] == '\"' && s[s.size() - 1] == '\"')
          {
            s = s.substr(1, s.size() - 2);
          }
          parts.push_back(s.substr(0, s.size()));
          std::vector<char*> vecArgs;
          char dummy1stArg[] = "7zz";
          vecArgs.push_back(dummy1stArg);
          bool hasFoundFilePath = false;
          std::string filePath;
          for (unsigned long ii = 0; ii < parts.size(); ++ii)
          {
            if (!hasFoundFilePath && ii >= 1 && !parts[ii].empty() && parts[ii][0] != '-')
            {
              filePath = parts[ii];
              hasFoundFilePath = true;
            }
            char* partData = const_cast<char*>(parts[ii].data());
            vecArgs.push_back(partData);
          }
          MainWrapper(
            static_cast<int>(parts.size() + 1),
            &vecArgs[0]
          );

          // this string is used by n2os_sandbox as an end-of-output marker
          printf("Nozomi-7zz done.\n");
          fflush(stdout);
        }
      }
    }
  }
  return -1;
}
