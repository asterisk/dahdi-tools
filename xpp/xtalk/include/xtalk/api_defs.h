#ifndef	XTALK_API_DEFS_H
#define	XTALK_API_DEFS_H

/*
 * Visibility settings: taken from:
 *     http://gcc.gnu.org/wiki/Visibility
 */

/* Generic helper definitions for shared library support */
#if __GNUC__ >= 4
	#define XTALK_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
	#define XTALK_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
	#define XTALK_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
#else
	#define XTALK_HELPER_DLL_IMPORT
	#define XTALK_HELPER_DLL_EXPORT
	#define XTALK_HELPER_DLL_LOCAL
#endif

/*
 * Now we use the generic helper definitions above to define XTALK_API and XTALK_LOCAL.
 * XTALK_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
 * XTALK_LOCAL is used for non-api symbols.
 */

#ifdef XTALK_DLL /* defined if XTALK is compiled as a DLL */
	#ifdef XTALK_DLL_EXPORTS /* defined if we are building the XTALK DLL (instead of using it) */
		#define XTALK_API XTALK_HELPER_DLL_EXPORT
	#else
		#define XTALK_API XTALK_HELPER_DLL_IMPORT
	#endif /* XTALK_DLL_EXPORTS */
	#define XTALK_LOCAL XTALK_HELPER_DLL_LOCAL
#else /* XTALK_DLL is not defined: this means XTALK is a static lib. */
	#define XTALK_API
	#define XTALK_LOCAL
#endif /* XTALK_DLL */

#endif	/* XTALK_API_DEFS_H */
