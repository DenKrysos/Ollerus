#ifndef REMAINDER_H
#define REMAINDER_H
/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */
/*
 *       Additional Programming-Language-Name: "PreC"
 *
 * Let me define a name for a collection of all this Preprocessor-Macro-Stuff,
 * that comes already before the actual C-Program and that'a able to change
 * the behavior of the Program, even of the Programming-Language; that
 * implements whole new functionalities and Syntaxes/Semantics.
 * I've made a collection of a lot Stuff like that, like Macro-Overloading/Overriding,
 * PreProcessor-Functions / Automations (Macro- / Function Split, Comparison,
 * Boolean Operations at Compile-Time etc.)
 * So i take the Freedom to pack all this and future additions into a new
 * "Programming-Language-Extension" called "PreC"
 */


#include "ollerus_globalsettings.h"
//#include "ollerus.h"

/*
 * Just a bunch of helpful functions/makros
 * Especially used for debugging/developing
 * Or something nice like the ansi_escape_use setting
 */

#include <pwd.h>




extern int getDigitCountofInt(int n);






#define BYTETOBINPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBIN(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

/* Use like:
char testchar=0x02;
printf("Bsp Convert Hex: %02X to Bin "BYTETOBINPATTERN"\n",testchar, BYTETOBIN(testchar));
*/





/*
 * Some absolutely Basic Macros
 *///////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//=====================================================================
//
////////////////////////////////
//////// PATTERN MATCHING
////////////////////////////////
#define CONCAT(ARG, ...) CRUDE_CONCAT(ARG, __VA_ARGS__)
#define CRUDE_CONCAT(ARG, ...) ARG ## __VA_ARGS__
#define LATE_CONCAT(ARG,...) EVAL_SMALL(CONCAT(ARG,__VA_ARGS__))
//---------------------------------------------------------------------
// Best Way is to use the following Macro for Concatenations:
//		CAT(Variable Number of Arguments)
// It does some Magic for you and can handle different Numbers of Arguments.
////////////////
#define _macro_multi_CAT_0(...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_multi_CAT_1(...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_multi_CAT_2(ARG1, ARG2) LATE_CONCAT(ARG1, ARG2)
#define _macro_multi_CAT_3(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_2(__VA_ARGS__))
#define _macro_multi_CAT_4(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_3(__VA_ARGS__))
#define _macro_multi_CAT_5(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_4(__VA_ARGS__))
#define _macro_multi_CAT_6(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_5(__VA_ARGS__))
#define _macro_multi_CAT_7(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_6(__VA_ARGS__))
#define _macro_multi_CAT_8(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_7(__VA_ARGS__))
#define _macro_multi_CAT_9(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_8(__VA_ARGS__))
#define _macro_multi_CAT_10(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_9(__VA_ARGS__))
#define _macro_multi_CAT_11(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_10(__VA_ARGS__))
#define _macro_multi_CAT_12(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_11(__VA_ARGS__))
#define _macro_multi_CAT_13(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_12(__VA_ARGS__))
#define _macro_multi_CAT_14(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_13(__VA_ARGS__))
#define _macro_multi_CAT_15(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_14(__VA_ARGS__))
#define _macro_multi_CAT_16(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_15(__VA_ARGS__))
#define _macro_multi_CAT_17(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_16(__VA_ARGS__))
#define _macro_multi_CAT_18(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_17(__VA_ARGS__))
#define _macro_multi_CAT_19(ARG1, ...) LATE_CONCAT(ARG1, _macro_multi_CAT_18(__VA_ARGS__))
////
#define CAT(...) CAT_EVALUATED(__VA_ARGS__)
#define CAT_EVALUATED(...) \
		macro_overloader(_macro_multi_CAT_, __VA_ARGS__)(__VA_ARGS__)
////////////////
// Please mind: The Concatenation doesn't like preceding or succeeding Special Characters, like dots "." or "/" etc.
//		Example:	Doesn't work:		file ## .txt
//					Doesn't work:		file. ## txt
//				Works like a charm:		Version ## 1.0
// But we can use a workaround! :D
// Here you have an Example for that:
//					#define _LogPathAppendix_1 log/MonitorBW_
//					#define _LogPathAppendix_DateTemplate XXXX-XX-XX_XX:XX:XX
//	Look here -->	#define _LogPathAppendix_Suffix log
//
//	Look here -->	#define _LogPathAppendix_Template CAT(_LogPathAppendix_1, _LogPathAppendix_DateTemplate._LogPathAppendix_Suffix)
//					#define LogPathAppendix_1 STRING_EXP(_LogPathAppendix_1)
//					#define LogPathAppendix_DateTemplate STRING_EXP(_LogPathAppendix_DateTemplate)
//					#define LogPathAppendix_Suffix STRING_EXP(_LogPathAppendix_Suffix)
//
//					#define LogPathAppendix_Template STRING_EXP(_LogPathAppendix_Template)
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//---------------------------------------------------------------------
  //Directly Stringificates the passed Argument, i.e. turns it into a string
#define STRINGIFICATE(ARG) #ARG
  //Stringificates after Macro-Expansion, due to one more Macro Passing Stage (Scan)
#define STRING_EXP(ARG) STRINGIFICATE(ARG)
//---------------------------------------------------------------------
#define IIF(c) CRUDE_CONCAT(IIF_, c)
#define IIF_0(t, ...) __VA_ARGS__
#define IIF_1(t, ...) t
//		#define A() 1
//		//This correctly expands to true
//		IIF(1)(true, false)
//		// And this will also correctly expand to true
//		IIF(A())(true, false)
//---------------------------------------------------------------------
#define COMPL(b) CRUDE_CONCAT(COMPL_, b)
#define COMPL_0 1
#define COMPL_1 0
////
#define BITAND(x) CRUDE_CONCAT(BITAND_, x)
#define BITAND_0(y) 0
#define BITAND_1(y) y
//---------------------------------------------------------------------
#define INC(x) CRUDE_CONCAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 10
////
#define DEC(x) CRUDE_CONCAT(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9

#define DEC_240 239
#define DEC_241 240
#define DEC_242 241
#define DEC_243 242
#define DEC_244 243
#define DEC_245 244
#define DEC_246 245
#define DEC_247 246
#define DEC_248 247
#define DEC_249 248
#define DEC_250 249
#define DEC_251 250
#define DEC_252 251
#define DEC_253 252
#define DEC_254 253
#define DEC_255 254
//---------------------------------------------------------------------
////////////////////////////////
//////// DETECTION
////////////////////////////////
#define CHECK_N(x, n, ...) n
#define CHECK(...) CHECK_N(__VA_ARGS__, 0,)
#define PROBE(x) x, 1,
//		CHECK(PROBE(~)) // Expands to 1
//		CHECK(xxx) // Expands to 0
//---------------------------------------------------------------------
#define IS_PAREN(x) CHECK(IS_PAREN_PROBE x)
#define IS_PAREN_PROBE(...) PROBE(~)
//		IS_PAREN(()) // Expands to 1
//		IS_PAREN(xxx) // Expands to 0
//---------------------------------------------------------------------
#define NOT(x) CHECK(CRUDE_CONCAT(NOT_, x))
#define NOT_0 PROBE(~)
//---------------------------------------------------------------------
#define BOOL(x) COMPL(NOT(x))
#define IF(c) IIF(BOOL(c))
//---------------------------------------------------------------------
#define EAT(...)
#define EXPAND(...) __VA_ARGS__
#define WHEN(c) IF(c)(EXPAND, EAT)
//---------------------------------------------------------------------
////////////////////////////////
//////// RECURSION
////////////////////////////////
#define EXPAND(...) __VA_ARGS__
#define EMPTY()
#define DEFER(id) id EMPTY()
#define OBSTRUCT(...) __VA_ARGS__ DEFER(EMPTY)()
// You want an Example for Deferred Expression?
//  Think about this:
//		#define A() 123
//		A() // Expands to 123
//		DEFER(A)() // Expands to A () because it requires one more scan to fully expand
//		EXPAND(DEFER(A)()) // Expands to 123, because the EXPAND macro forces another scan
//---------------------------------------------------------------------
#define EVAL(...)  EVAL1(EVAL1(EVAL1(__VA_ARGS__)))
#define EVAL1(...) EVAL2(EVAL2(EVAL2(__VA_ARGS__)))
#define EVAL2(...) EVAL3(EVAL3(EVAL3(__VA_ARGS__)))
#define EVAL3(...) EVAL4(EVAL4(EVAL4(__VA_ARGS__)))
#define EVAL4(...) EVAL5(EVAL5(EVAL5(__VA_ARGS__)))
#define EVAL5(...) __VA_ARGS__
////
#define EVAL_SMALL(...)  EVAL_SMALL1(EVAL_SMALL1(EVAL_SMALL1(__VA_ARGS__)))
#define EVAL_SMALL1(...) EVAL_SMALL2(EVAL_SMALL2(EVAL_SMALL2(__VA_ARGS__)))
#define EVAL_SMALL2(...) EVAL_SMALL3(EVAL_SMALL3(EVAL_SMALL3(__VA_ARGS__)))
#define EVAL_SMALL3(...) EVAL_SMALL4(EVAL_SMALL4(EVAL_SMALL4(__VA_ARGS__)))
#define EVAL_SMALL4(...) __VA_ARGS__
//---------------------------------------------------------------------
#define REPEAT(count, macro, ...) \
    WHEN(count) \
    ( \
        OBSTRUCT(REPEAT_INDIRECT) () \
        ( \
            DEC(count), macro, __VA_ARGS__ \
        ) \
        OBSTRUCT(macro) \
        ( \
            DEC(count), __VA_ARGS__ \
        ) \
    )
#define REPEAT_INDIRECT() REPEAT
//		//An example of using this macro
//		#define M(i, _) i
//		EVAL(REPEAT(8, M, ~)) // 0 1 2 3 4 5 6 7
//---------------------------------------------------------------------
#define WHILE(pred, op, ...) \
    IF(pred(__VA_ARGS__)) \
    ( \
        OBSTRUCT(WHILE_INDIRECT) () \
        ( \
            pred, op, op(__VA_ARGS__) \
        ), \
        __VA_ARGS__ \
    )
#define WHILE_INDIRECT() WHILE
//---------------------------------------------------------------------
////////////////////////////////
//////// COMPARISON
////////////////////////////////
#define PRIMITIVE_COMPARE(x, y) IS_PAREN \
( \
COMPARE_ ## x ( COMPARE_ ## y) (())  \
)
//---------------------------------------------------------------------
#define IS_COMPARABLE(x) IS_PAREN( CONCAT(COMPARE_, x) (()) )
////
#define NOT_EQUAL(x, y) \
IIF(BITAND(IS_COMPARABLE(x))(IS_COMPARABLE(y)) ) \
( \
   PRIMITIVE_COMPARE, \
   1 EAT \
)(x, y)
//---------------------------------------------------------------------
#define EQUAL(x, y) COMPL(NOT_EQUAL(x, y))
//---------------------------------------------------------------------
// The EQUAL Macro allows you to check, if two given tokens, i.e. simple text,
//   are equal.
//   Do not use PRIMITIVE_COMPARE standalone. It has a terrible behavior, if the
//   necessary COMPARE_ Macros of the given tokens aren't defined.
// To Use the EQUAL Macro you have to define a COMPARE_ Macro of the two
//   Tokens to be compared beforehand. Like:
//		#define COMPARE_foo(x) x
//		#define COMPARE_bar(x) x
//   for a use like:
//		EQUAL(foo,bar);
//		EQUAL(foo,foo);
//---------------------------------------------------------------------
//
//========================================================================
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////










//For now just hardcoded and use ANSI Escape on the Output Console
//Later on maybe something like a config File Read-In with possible Values:
// on, off, auto
//and than with the auto something like the crazy Lookup-Table Stuff, that ncurses does.
#define SET_ansi_escape_use ansi_escape_use = 1;


//And a macro for the quick programming use of changing the Console Color
#define ANSICOLORSET(ANSIcolorToSet) if(ansi_escape_use==1){ \
		printf(ANSIcolorToSet); \
	}

#define ANSICOLORRESET if(ansi_escape_use==1){ \
		printf(ANSI_COLOR_RESET); \
	}
//And to make it still shorter:
#define printfc(color,...) ANSICOLORSET(ANSI_COLOR_##color); \
printf(__VA_ARGS__); \
ANSICOLORRESET;


//Macros to Set or Unset a flag
#define FLAG_SET(FlagHolder,Flag) (FlagHolder)=(FlagHolder) | (Flag)

#define FLAG_UNSET(FlagHolder,Flag) (FlagHolder)=(FlagHolder) & (~(Flag))

#define FLAG_CHECK(FlagHolder,Flag) ((FlagHolder) & (Flag))


//It gets a Pointer to a string
//About the String-Size: Minus the "ollerus", but with +1 for the '\0'
//NOTE: The initial arguments, that are given to the program at Terminal-Call should be
//globally stored in "args".
//This means, you are able to call this Macro everywhere like "CREATE_PROGRAM_PATH(*args);"
#ifdef FILES_LOCAL_OR_IN_PROJECT
	#ifdef FILES_GLOBAL_OR_USER_SPECIFIC
		#define CREATE_PROGRAM_PATH(arg) \
			char ProgPath[]="/etc/ollerus/";
			//printf("ProgPath %s | %d\n",ProgPath[0-5],sizeof(ProgPath));exit(1);
	#else
		#define CREATE_PROGRAM_PATH(arg) \
			{ \
				char *tmppath; \
				if ((tmppath = getenv("XDG_CONFIG_HOME")) == NULL) { \
					if ((tmppath = getenv("HOME")) == NULL) { \
						tmppath = getpwuid(getuid())->pw_dir; \
					} \
				} \
				/* len=strlen(Ascertained_Path)+strlen("\ollerus\")+'\0' */ \
				err=(strlen(tmppath)+10); \
			} \
			char ProgPath[err]; \
			{ \
				char *tmppath; \
				if ((tmppath = getenv("XDG_CONFIG_HOME")) == NULL) { \
					if ((tmppath = getenv("HOME")) == NULL) { \
						tmppath = getpwuid(getuid())->pw_dir; \
					} \
				} \
				memcpy(ProgPath,tmppath,strlen(tmppath)+1); \
				snprintf(ProgPath+strlen(tmppath),10,"/ollerus/"); \
				ProgPath[sizeof(ProgPath)]='\0'; \
			}
	#endif
#else
	#define CREATE_PROGRAM_PATH(arg) char ProgPath[strlen(arg)-6]; \
		memcpy(ProgPath,arg,sizeof(ProgPath)); \
		ProgPath[sizeof(ProgPath)-1]='\0';
#endif


/*Everything is in a scope*/
/*Give this Macro a string with the preceding '/' after the last folder*/
/*This is IMPORTANT or the last folder isn't created*/
/*Example: "/folderA/folderB/folderC/"  */
#define CREATE_COMPLETE_FOLDER_PATH(path) { \
	int pathloop; \
	struct stat st = {0}; \
	char temppath[sizeof(path)]; \
	memset(temppath,0,sizeof(temppath)); \
	for(pathloop=1;pathloop<strlen(path);pathloop++){ \
		if(path[pathloop]=='/'){ \
			memcpy(temppath,path,pathloop); \
			/*temppath[pathloop]='\0';*/ \
			if (stat(temppath, &st) == -1) { \
				mkdir(temppath,S_IRWXU|S_IRWXG|S_IRWXO); \
			} \
		} \
	} \
} \



// TODO - Based on the WHILE Loop
// #define _CREATE_NTH_ARG_LIST

//Macro "returns" the number of passed arguments
//Current Implementation: A Maximum of 10
//Easily extendible (in an obvious way...) to up to 63
//(which reaches the limit of preprocessor arguments anyway...)
#define _GET_NTH_ARG(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,N,...) N
#define COUNT_VARARGS(...) _GET_NTH_ARG("ignored", ##__VA_ARGS__, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
//Ähnliche Funktion, kann aber nicht mit "leeren" variadic arguments umgehen
//		#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 10,9,8,7,6,5,4,3,2,1)
//		#define VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N



// A general MacroOverloader
// Use it the right way and the preprocessor chooses the right function/macro
// for you at compile time, depending the number of passed arguments
//Example: "depr()", "GET_REAL_TIME()"
#define macro_overloader(func, ...) \
	macro_overloader_(func, COUNT_VARARGS(__VA_ARGS__))
#define macro_overloader_(func, nargs) \
	macro_overloader__(func, nargs)
#define macro_overloader__(func, nargs) \
	func ## nargs







/*////////////////////
 * A Framing for applying a macro over a list of args, like in
 * a for-each-loop.
 */////////////////////
/*////////////////////
 * Usage:
 * #define a macro to be used over many arguments
 * #define a macro with a comma seperated list of the wanted arguments, or pass the arg list directly
 * Use: CALL_MACRO_X_FOR_EACH(MACRO, Arg_List)
 */////////////////////
// Some chained override-macros, which are getting called cascaded,
// based on the n-arity of the passed arguments list.
// The cascading Call and throughpassing, nudged by the "coupled, gliding arg lists"
// creates a macro in a for-each like style.
#define _macro_foreach_0(_macroCall, ...)
#define _macro_foreach_1(_macroCall, x) _macroCall(x)
#define _macro_foreach_2(_macroCall, x, ...) _macroCall(x) _macro_foreach_1(_macroCall, __VA_ARGS__)
#define _macro_foreach_3(_macroCall, x, ...) _macroCall(x) _macro_foreach_2(_macroCall, __VA_ARGS__)
#define _macro_foreach_4(_macroCall, x, ...) _macroCall(x) _macro_foreach_3(_macroCall, __VA_ARGS__)
#define _macro_foreach_5(_macroCall, x, ...) _macroCall(x) _macro_foreach_4(_macroCall, __VA_ARGS__)
#define _macro_foreach_6(_macroCall, x, ...) _macroCall(x) _macro_foreach_5(_macroCall, __VA_ARGS__)
#define _macro_foreach_7(_macroCall, x, ...) _macroCall(x) _macro_foreach_6(_macroCall, __VA_ARGS__)
#define _macro_foreach_8(_macroCall, x, ...) _macroCall(x) _macro_foreach_7(_macroCall, __VA_ARGS__)
#define _macro_foreach_9(_macroCall, x, ...) _macroCall(x) _macro_foreach_8(_macroCall, __VA_ARGS__)
////////////////////
//The length of the defined _GET_NTH_ARG and the used inside the CALL_MACRO_X_FOR_EACH has to be the same
// So here he gets an own one to secure that.
#define _GET_NTH_ARG_FE(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
////////////////////
//The actual Macro
// Supplies a for-each construct for Variadic Macros. Supports at most 9 args or less.
// You see, how you could expand it ;oP
//   To make it work both with directly passed many arguments and also with
//   predefined lists we need an additional Expansion-Step.
//   Therefore the two chained Macros
#define CALL_MACRO_X_FOR_EACH(x,...) CALL_MACRO_X_FOR_EACH_EVALUATED(x,__VA_ARGS__)
#define CALL_MACRO_X_FOR_EACH_EVALUATED(x, ...) \
    _GET_NTH_ARG_FE("ignored", ##__VA_ARGS__, \
    _macro_foreach_9, _macro_foreach_8, _macro_foreach_7, _macro_foreach_6, _macro_foreach_5, _macro_foreach_4, _macro_foreach_3, _macro_foreach_2, _macro_foreach_1, _macro_foreach_0)(x, ##__VA_ARGS__)
//
// TODO
#define CALL_MACRO_X_FOR_EACH_2TUPEL(x,...) CALL_MACRO_X_FOR_EACH_2TUPEL_EVALUATED(x,__VA_ARGS__)
#define CALL_MACRO_X_FOR_EACH_2TUPEL_EVALUATED(x, ...) \
    _GET_NTH_ARG_FE("ignored", ##__VA_ARGS__, \
    _macro_foreach_9, _macro_foreach_8, _macro_foreach_7, _macro_foreach_6, _macro_foreach_5, _macro_foreach_4, _macro_foreach_3, _macro_foreach_2, _macro_foreach_1, _macro_foreach_0)(x, ##__VA_ARGS__)
////////////////////
////////////////////
// Simple Example, albeit not very useful. Just to show how to use
//		#define ERRPrint_MAC(text) fprintf(stderr, text);
//		#define print_errors "error1\n", "error2\n"
//
//		CALL_MACRO_X_FOR_EACH(ERRPrint_MAC, "error1\n", "error2\n", "error3\n")
//		CALL_MACRO_X_FOR_EACH(ERRPrint_MAC, print_errors)
////////////////////
// Example usage1:
//     #define FWD_DECLARE_CLASS(cls) class cls;
//     CALL_MACRO_X_FOR_EACH(FWD_DECLARE_CLASS, Foo, Bar)
////////////////////
// Example usage 2:
//     #define START_NS(ns) namespace ns {
//     #define END_NS(ns) }
//     #define MY_NAMESPACES System, Net, Http
//     CALL_MACRO_X_FOR_EACH(START_NS, MY_NAMESPACES)
//     typedef foo int;
//     CALL_MACRO_X_FOR_EACH(END_NS, MY_NAMESPACES)
////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------
//====================================================================================================
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////





/*////////////////////
 * A Macro for use a list of args, like in
 * If-Then-Else Ladder, i.e. a switch-instruction
 */////////////////////
/*////////////////////
 * Usage:
 * - Pass the "switch_condition", "instructions" after each other as a list, with a succeeding "default"
 * 		- default: Instructions, that get executed, if no preceding condition got matched. (In other words: The last "else" in an If-Then-Else-Ladder)
 * - SWITCH(condition1, instructions1, condition2, instructions2, ... n times ..., defaultInstructions)
 */////////////////////
// Similarly to the CALL_MACRO_X_FOR_EACH, but with a recursive Call, instead of a cascaded.
#define _macro_switch_0(_macroCall, ...)
#define _macro_switch_1(_macroCall, x) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_2(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_3(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, false_inst)
#define _macro_switch_4(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_5(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_3(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_6(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_7(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_5(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_8(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_9(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_7(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_10(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_11(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_9(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_12(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_13(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_11(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_14(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_15(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_13(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_16(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_17(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_15(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_18(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_19(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_17(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_20(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_21(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_19(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_22(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_23(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_21(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_24(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_25(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_23(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_26(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_27(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_25(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_28(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_29(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_27(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_30(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_31(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_29(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_32(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_33(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_31(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_34(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_35(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_33(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_36(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_37(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_35(_macroCall, false_inst, __VA_ARGS__))
#define _macro_switch_38(_macroCall, x, ...) "ERROR. To few variadic arguments in overloaded macro"ERROR
#define _macro_switch_39(_macroCall, cond, true_inst, false_inst, ...) _macroCall(cond)(true_inst, _macro_switch_37(_macroCall, false_inst, __VA_ARGS__))
// the "false_inst" doesn't have to be present. Would also work without it. But i wrote it as well to make things clearer.
////////////////////
//The length of the defined _GET_NTH_ARG and the used inside the SWITCH_EVALUATED has to be the same
// So here he gets an own one to secure that.
#define _GET_NTH_ARG_SWITCH(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,N,...) N
////////////////////
//The actual Macro
// Supplies a Switch-Instruction on the Preprocessor Layer. Supports the number of args like the highest number inside _GET_NTH_ARG_SWITCH -1.
// You see, how you could expand it ;oP (Up to the Max of 63, whats the limit of the Preprocessor for arguments.
//   To make it work both with directly passed many arguments and also with
//   predefined lists we need an additional Expansion-Step.
//   Therefore the two chained Macros
#define SWITCH(...) SWITCH_EVALUATED(__VA_ARGS__)
#define SWITCH_EVALUATED(...) \
    _GET_NTH_ARG_SWITCH("ignored", ##__VA_ARGS__, \
    _macro_switch_39, _macro_switch_38, _macro_switch_37, _macro_switch_36, _macro_switch_35, _macro_switch_34, _macro_switch_33, _macro_switch_32, _macro_switch_31, _macro_switch_30, _macro_switch_29, _macro_switch_28, _macro_switch_27, _macro_switch_26, _macro_switch_25, _macro_switch_24, _macro_switch_23, _macro_switch_22, _macro_switch_21, _macro_switch_20, _macro_switch_19, _macro_switch_18, _macro_switch_17, _macro_switch_16, _macro_switch_15, _macro_switch_14, _macro_switch_13, _macro_switch_12, _macro_switch_11, _macro_switch_10, _macro_switch_9, _macro_switch_8, _macro_switch_7, _macro_switch_6, _macro_switch_5, _macro_switch_4, _macro_switch_3, _macro_switch_2, _macro_switch_1, _macro_switch_0)(IF, ##__VA_ARGS__)
//////////////////////
////////////////////
// Simple Example (So you can see, that you can format it nicely ;o) )
//		SWITCH( \
//			EQUAL(precision,nano), \
//				getRealTime_nanoseconds(variable); \
//				, \
//			EQUAL(precision,micro), \
//				getRealTime_microseconds(variable); \
//				, \
//			EQUAL(precision,timespec), \
//				getRealTime_nanoseconds(variable); \
//				, \
//			EQUAL(precision,timeval), \
//				getRealTime_microseconds(variable); \
//				, \
//			getRealTime(); \
//		)
////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------
//====================================================================================================
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////









//Use this to specify Mili-Seconds from Nano-Seconds
#define MS_TO_US(MS) MS ## 000
#define US_TO_NS(US) US ## 000
#define MS_TO_NS(MS) MS ## 000000

#define WAITNS(SEC,NSEC) { \
	struct timespec remainingdelay; \
	remainingdelay.tv_sec = SEC; \
	remainingdelay.tv_nsec = NSEC; \
	while (nanosleep(&remainingdelay, &remainingdelay)<0){} \
}


/*
 *GET_REAL_TIME()
 *Macro to let you choose, which kind of data format you
 *want to get returned, depending on the number of passed arguments
 *
 * double time;
 * time = GET_REAL_TIME();
 *		returns you back a double-float in seconds, with micro/nano-seconds as decimals.
 *		This one is portable over different Operating Systems.
 *		It delivers you best available precision. But it doesn't guarentee a "Time since Epoch",
 *		i.e. maybe, or rather very probably you can't use the result to calculate Date and Time.
 *		So only use this for purposes like Application Runtime, Point-in-Time comparisons and
 *		stuff like that.
 *
 * struct timespec ts;
 * char precision;
 * precision = GET_REAL_TIME(&ts);
 *		Edits the passed timespec-elements, seconds and nano-seconds
 *
 * struct timeval tm;
 * char precision;
 * precision = GET_REAL_TIME(&tm,NULL);
 *		Edits the passed timeval-elements, seconds and micro-seconds. Eventually in system available nano-second information is lost.
 *
 *	precision:
 *		see #defines below
 *
 * The 'NULL' can be anything, really ANYTHING. It just has to be ONE argument.
 * It isn't really used. It is just misused to split the macro by the argument count
 * So let us use 'NULL' to show this and particularly to make it pretty to watch. We programmers all love this NULL ^^
 *
 */
#define GetRealTimeM(...) macro_overloader(_GET_REAL_TIME_OVERRIDE, __VA_ARGS__)(__VA_ARGS__)
#define _GET_REAL_TIME_OVERRIDE0(...) getRealTime_double()
#define _GET_REAL_TIME_OVERRIDE1(...) getRealTime_double_clock(__VA_ARGS__)
#define _GET_REAL_TIME_OVERRIDE2(precision,variable,...) GetRealTime_SPLIT(precision,variable)
#define _GET_REAL_TIME_OVERRIDE3(format,variable,clock,...) GetRealTime_SPLIT_Clock(format,variable,clock)
#define _GET_REAL_TIME_OVERRIDE4(...) "ERROR. To many variadic arguments in overloaded macro"ERROR
#define _GET_REAL_TIME_OVERRIDE5(...) _GET_REAL_TIME_OVERRIDE4(__VA_ARGS__)
//////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////
//  Necessary Macros for the PreC Function-Split
////////////////////////////
//#define COMPARE_(x) x
#define COMPARE_timespec(x) x
#define COMPARE_timeval(x) x
#define COMPARE_micro(x) x
#define COMPARE_nano(x) x
#define COMPARE_double(x) x
#define COMPARE_realtime(x) x
#define COMPARE_monotonic_precise(x) x
#define COMPARE_monotonic_raw(x) x
#define COMPARE_monotonic(x) x
#define COMPARE_highres(x) x
#define COMPARE_process_cputime_id(x) x
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define GetRealTime_SPLIT(PRECISION,VARIABLE) \
		SWITCH( \
			EQUAL(PRECISION,nano), \
				getRealTime_nanoseconds(VARIABLE) \
				, \
			EQUAL(PRECISION,micro), \
				getRealTime_microseconds(VARIABLE) \
				, \
			EQUAL(PRECISION,timespec), \
				getRealTime_nanoseconds(VARIABLE) \
				, \
			EQUAL(PRECISION,timeval), \
				getRealTime_microseconds(VARIABLE) \
				, \
			EQUAL(PRECISION,double), \
			getRealTime_double_clock(VARIABLE) \
				, \
			getRealTime_double() \
		)
	// In the double case, the VARIABLE is actually the CLOCK
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define GetRealTime_SPLIT_Clock(FORMAT,VARIABLE,CLOCK) \
		SWITCH( \
			EQUAL(FORMAT,nano), \
				getRealTime_nanoseconds_clock(VARIABLE,CLOCK) \
				, \
			EQUAL(FORMAT,micro), \
				getRealTime_microseconds_clock(VARIABLE,CLOCK) \
				, \
			EQUAL(FORMAT,timespec), \
				getRealTime_nanoseconds_clock(VARIABLE,CLOCK) \
				, \
			EQUAL(FORMAT,timeval), \
				getRealTime_microseconds_clock(VARIABLE,CLOCK) \
				, \
			EQUAL(FORMAT,double), \
				getRealTime_double_clock(CLOCK) \
				, \
			getRealTime_double(); \
		)

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
#define GET_REAL_TIME_PREC_UNKNOWN 0
#define GET_REAL_TIME_PREC_SECONDS INC(GET_REAL_TIME_PREC_UNKNOWN)
#define GET_REAL_TIME_PREC_MILLI_SECONDS INC(GET_REAL_TIME_PREC_SECONDS)
#define GET_REAL_TIME_PREC_MICRO_SECONDS INC(GET_REAL_TIME_PREC_MILLI_SECONDS)
#define GET_REAL_TIME_PREC_NANO_SECONDS INC(GET_REAL_TIME_PREC_MICRO_SECONDS)
//////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------------------------------
//====================================================================================================
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////


#endif /* REMAINDER_H */
