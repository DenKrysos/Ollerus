#ifndef DEBUG_H
#define DEBUG_H
/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include <limits.h>//For maximum limits of Data-Types
#include <float.h>//Same for floats

#include <remainder.h>
#include "ollerus_globalsettings.h"
//#include "ollerus.h"

/*
 * Some Functions like the Bandwidth-Measurement and Connection Continuity Check
 */


#define DEBUG_SERVER 0
#define DEBUG_CLIENT 1

#define PORTBWMEASURE 24289

#define RECVBUFFSIZEBWMEASURE 524288//0.5 MB (512k * 1024)
#define BWMEASURETIMEINTERVALL 1//In seconds
#define BWNUMAVERAGEINTERVALLS 10//Number of Intervalls to do a moving average over



int monitorbandwidth(char mode, char **dest);

int monitorBandwidthAndContinuityCheck(char mode, char **dest);

int continuityCheck(char mode, char **dest);


#define print_malloc_size(voidpoint) { \
	size_t mallocsize=malloc_usable_size(voidpoint); \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("Allocated size: %d",(int)mallocsize); \
	ANSICOLORRESET; \
	printf("\n"); \
}
	
	
	

//Debug Print (simple ones)
//A sophisticated, big brother, which covers this all here
//together by overloading mechanism lays down under
#ifdef DEBUG
	extern int DebugPrintCount;
#endif
#ifdef DEBUG
	#define deprl(...) printfc(blue,"\n==> "); \
		printfc(green,"Debug "); \
		printfc(cyan,"Line "); \
		printfc(red,"%d",__LINE__); \
		printfc(green," (#%d)",DebugPrintCount); \
		printf(" | "__VA_ARGS__); \
		puts(""); \
		DebugPrintCount++;
#else
	#define deprl(...)
#endif
#ifdef DEBUG
	#define depri(idx,...) printfc(blue,"\n==> "); \
		printfc(green,"Debug "); \
		printfc(cyan,"Idx "); \
		printfc(red,#idx); \
		printfc(green," (#%d)",DebugPrintCount); \
		printf(" | "__VA_ARGS__); \
		puts(""); \
		DebugPrintCount++;
#else
	#define depri(...)
#endif


//Debug Print
//Works differently, with a differing Number of delivered Arguments
//Could be anything from 0 to gratuitious amounts of Arguments
/* Use like this:
 * The Arguments:
 * depr(); Prints Line Number of Makro Usage and an incrementing Counter
 * depr(Index); Additionally the delivered Index
 * depr(Index,String); Additionally a String, like basic printf
 * depr(Index,String,StringArgs); Additionally Arguments for the preceding String, like with printf
 * Full Command Example:
 * depr(1,"Test float: %lf",floatvariable);
 * Simple Examples for the different Usage Methods:
 * depr()
 * depr(1)
 * depr(2,"test")
 * depr(3,"test2 :%d",5)
 */
#ifdef DEBUG
	extern int DebugPrintCount;
#endif
#ifdef DEBUG
	#define deprColorPrefix blue
	#define deprColorDebug green
	#define deprColorIndex magenta
	#define deprColorIndexNumber red
	#define deprColorLine cyan
	#define deprColorFile cyan
	#define deprColorCounter brown
	#define deprExpandprintfc(color,...) printfc(color,__VA_ARGS__)

	//The MacroOverloader is specified inside "remainder.h"

	#define depr(...) macro_overloader(_depr, __VA_ARGS__)(__VA_ARGS__)
	#define _depr0(...) deprExpandprintfc(deprColorPrefix,"\n==> "); \
		deprExpandprintfc(deprColorDebug,"Debug "); \
		deprExpandprintfc(deprColorLine,"Line "); \
		deprExpandprintfc(deprColorIndexNumber,"%d ",__LINE__); \
		deprExpandprintfc(deprColorFile,"File "); \
		deprExpandprintfc(deprColorIndexNumber,"%s",__FILE__); \
		deprExpandprintfc(deprColorCounter," (#%d)",DebugPrintCount); \
		puts(""); \
		DebugPrintCount++;
	#define _depr1(idx) deprExpandprintfc(deprColorPrefix,"\n==> "); \
		deprExpandprintfc(deprColorDebug,"Debug "); \
		deprExpandprintfc(deprColorIndex,"Idx "); \
		deprExpandprintfc(deprColorIndexNumber,#idx); \
		deprExpandprintfc(deprColorLine," Line "); \
		deprExpandprintfc(deprColorIndexNumber,"%d ",__LINE__); \
		deprExpandprintfc(deprColorFile,"File "); \
		deprExpandprintfc(deprColorIndexNumber,"%s",__FILE__); \
		deprExpandprintfc(deprColorCounter," (#%d)",DebugPrintCount); \
		puts(""); \
		DebugPrintCount++;
	#define _depr2(idx,...) deprExpandprintfc(deprColorPrefix,"\n==> "); \
		deprExpandprintfc(deprColorDebug,"Debug "); \
		deprExpandprintfc(deprColorIndex,"Idx "); \
		deprExpandprintfc(deprColorIndexNumber,#idx); \
		deprExpandprintfc(deprColorLine," Line "); \
		deprExpandprintfc(deprColorIndexNumber,"%d ",__LINE__); \
		deprExpandprintfc(deprColorFile,"File "); \
		deprExpandprintfc(deprColorIndexNumber,"%s",__FILE__); \
		deprExpandprintfc(deprColorCounter," (#%d)",DebugPrintCount); \
		printf(" | "__VA_ARGS__); \
		puts(""); \
		DebugPrintCount++;
	#define _depr3(idx,...) deprExpandprintfc(deprColorPrefix,"\n==> "); \
		deprExpandprintfc(deprColorDebug,"Debug "); \
		deprExpandprintfc(deprColorIndex,"Idx "); \
		deprExpandprintfc(deprColorIndexNumber,#idx); \
		deprExpandprintfc(deprColorLine," Line "); \
		deprExpandprintfc(deprColorIndexNumber,"%d ",__LINE__); \
		deprExpandprintfc(deprColorFile,"File "); \
		deprExpandprintfc(deprColorIndexNumber,"%s",__FILE__); \
		deprExpandprintfc(deprColorCounter," (#%d)",DebugPrintCount); \
		printf(" | "__VA_ARGS__); \
		puts(""); \
		DebugPrintCount++;
	#define _depr4(idx,...) _depr3(idx,__VA_ARGS__)
	#define _depr5(idx,...) _depr3(idx,__VA_ARGS__)
	#define _depr6(idx,...) _depr3(idx,__VA_ARGS__)
	#define _depr7(idx,...) _depr3(idx,__VA_ARGS__)
	#define _depr8(idx,...) _depr3(idx,__VA_ARGS__)
	#define _depr9(idx,...) _depr3(idx,__VA_ARGS__)
#else
	#define depr(...)
#endif
//NOTE:
// - depr2(idx,...) could be depr2(idx, str) instead
// - depr3(idx,...) could be depr3(idx, str, format) instead





// ===========================================================================
// ===========================================================================
// Could be extracted as "CAssert.h":
//
//		C Assert - CAssert - CASSERT
//			Compile Time Assertions for C
//
/** A compile time assertion check.
 *
 *  Validate at compile time that the predicate is true without
 *  generating code. This can be used at any point in a source file
 *  where typedef is legal.
 *
 *  On success, compilation proceeds normally.
 *
 *  On failure, attempts to typedef an array type of negative size. The
 *  offending line will look like
 *      typedef assertion_failed_file_h_42[-1]
 *  where file is the content of the second parameter which should
 *  typically be related in some obvious way to the containing file
 *  name, 42 is the line number in the file on which the assertion
 *  appears, and -1 is the result of a calculation based on the
 *  predicate failing.
 *
 *  \param predicate The predicate to test. It must evaluate to
 *  something that can be coerced to a normal C boolean.
 *
 *  \param file A sequence of legal identifier characters that should
 *  uniquely identify the source file in which this condition appears.
 */
#define CASSERT(predicate, file) _impl_CASSERT_LINE(predicate,__LINE__,file)

#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
	typedef char _impl_PASTE(assertion_failed_##file##_,line)[2*!!(predicate)-1];
// ===========================================================================
// ===========================================================================
//				Notices
// ===========================================================================
	//		Typical usage like this
// ===========================================================================
		//	#include "CAssert.h"
		//	...
		//	struct foo {
		//	    ...  /* 76 bytes of members */
		//	};
		//	CASSERT(sizeof(struct foo) == 76, demo_c);
// ===========================================================================
	//		In GCC a Assertion-Failure would look like
// ===========================================================================
		//	$ gcc -c demo.c
		//	demo.c:32: error: size of array `assertion_failed_demo_c_32' is negative
		//	$
// ===========================================================================
// ===========================================================================
//				CAssert.h End
// ===========================================================================
// ===========================================================================
// ===========================================================================
// ===========================================================================





#endif /* DEBUG_H */
