#ifndef FUNCTION_CREATOR_FUNC_H
#define FUNCTION_CREATOR_FUNC_H

/*
 * Authored by
 * Dennis Krummacker (06.03.2017-)
 */


#include "function_creator_funcname.h"




//////////////////////////////////////////////////////////////////////////////////////////////////////
// ***		remainder.c		***
//====================================================================================================
//----------------------------------------------------------------------------------------------------
#ifdef REMAINDER_C
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define COMPARE_buffer(x) x
#define CREATE_PRINT_DAYTIME_PRINT_FORMAT "%04d-%02d-%02d, %02d:%02d:%02d.%09d",(int)time.tm_year,(int)time.tm_mon,(int)time.tm_mday,(int)time.tm_hour,(int)time.tm_min,(int)time.tm_sec,(int)ts.tv_nsec
//The Buffer needs at least 31 Bytes (the Null-Terminating-Character included)
#define CREATE_PRINT_DAYTIME(APPENDIX,ARGUMENT,LOCATION) CREATE_PRINT_DAYTIME_FUNCNAME(APPENDIX,ARGUMENT){ \
	struct timespec ts; \
	struct tm time; \
	GetRealTimeM(nano,&ts,CLOCK_REALTIME); \
	localtime_r(&(ts.tv_sec), &time); \
	time.tm_mon+=1; \
	time.tm_year+=1900; \
	SWITCH( \
		EQUAL(LOCATION,buffer), \
		snprintf(LOCATION,31,CREATE_PRINT_DAYTIME_PRINT_FORMAT); \
		, \
		fprintf(LOCATION,CREATE_PRINT_DAYTIME_PRINT_FORMAT); \
	) \
	return 0; \
}
//////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
//----------------------------------------------------------------------------------------------------
//====================================================================================================
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////






#endif /* FUNCTION_CREATOR_FUNC_H */
