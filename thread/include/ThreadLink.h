#ifndef _THREAD_LINK_H_
#define _THREAD_LINK_H_


#if defined WIN32
 
	#ifdef THREAD_EXPORTS
		#define THREAD_CLASS_DECL	__declspec(dllexport)
		#define THREAD_CLASS_API  
		#define THREAD_API			__declspec(dllexport)
		#define THREAD_DATA			__declspec(dllexport)
	#else 
		#define THREAD_CLASS_DECL	__declspec(dllimport)
		#define THREAD_CLASS_API      
		#define THREAD_API			__declspec(dllimport)
		#define THREAD_DATA			__declspec(dllimport)
	#endif
    
#elif defined __SYMBIAN32__

    #define THREAD_CLASS_DECL  
	#define THREAD_CLASS_API		__declspec(dllexport)
    #define THREAD_API				__declspec(dllexport)
    #define THREAD_DATA				__declspec(dllexport)
		
#elif defined _LINUX

    #define THREAD_CLASS_DECL   
	#define THREAD_CLASS_API 
    #define THREAD_API 
    #define THREAD_DATA

#elif defined _MAC

	#ifdef THREAD_EXPORTS
		#define THREAD_CLASS_DECL	__attribute__((visibility("default")))
		#define THREAD_CLASS_API	__attribute__((visibility("default")))
		#define THREAD_API			__attribute__((visibility("default")))
		#define THREAD_DATA			__attribute__((visibility("default")))
	#else 
		#define THREAD_CLASS_DECL   
		#define THREAD_CLASS_API      
		#define THREAD_API          
		#define THREAD_DATA        
	#endif
  		
#endif //~#if defined WIN32


#endif // _THREAD_LINK_H_