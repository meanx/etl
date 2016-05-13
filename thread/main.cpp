// main.cpp : Defines the entry point for the DLL application.
//

#if   _MSC_VER >= 1500	// >= vs2008

#include <SDKDDKVer.h>

#elif _MSC_VER <= 1400	// <= vs2005

# if defined _WIN32_WCE
  
  #pragma comment(linker, "/nodefaultlib:libc.lib")
  #pragma comment(linker, "/nodefaultlib:libcd.lib")

  // NOTE - this value is not strongly correlated to the Windows CE OS version being targeted
  #define WINVER _WIN32_WCE

# elif defined WIN32 //!_WIN32_WCE

  // Modify the following defines if you have to target a platform prior to the ones specified below.
  // Refer to MSDN for the latest info on corresponding values for different platforms.
  #ifndef WINVER			// Allow use of features specific to Windows XP or later.
  #define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
  #endif

  #ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
  #define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
  #endif						

  #ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
  #define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
  #endif

  #ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
  #define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
  #endif

# endif //~_WIN32_WCE

#endif //~_MSC_VER

#ifdef _WIN32_WCE
#include <ceconfig.h>
#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
#define SHELL_AYGSHELL
#endif

#ifdef _CE_DCOM
#define _ATL_APARTMENT_THREADED
#endif
#endif //~_WIN32_WCE

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>


#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif

#endif //~WIN32