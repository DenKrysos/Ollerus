#ifndef OLLERUS_BASE_H
#define OLLERUS_BASE_H

/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 * Based on Experience from iw (Johannes Berg)
 */


/*
 * Definition of return-error-codes
 */
#define MAIN_ERR_NONE 100
#define MAIN_ERR_STD 101
#define MAIN_ERR_WARNING 102
#define MAIN_ERR_CLEARED 103
#define MAIN_ERR_FEW_CMDS 104
#define MAIN_ERR_BAD_CMDLINE 105
#define MAIN_ERR_FUNC_INCOMPLETE 106
#define MAIN_ERR_TIMEOUT 107
#define MAIN_ERR_BAD_RUN 109
#define MAIN_ERR_CANCELED 110
#define OPERATION_ERR_STD 10
#define OPERATION_ERR_NOTHING_TO_DO 11
#define OPERATION_ERR_NEVER_HAPPEN 12
#define OPERATION_ERR_NOT_SUPPORTED 13
#define OPERATION_ERR_UNSUCCESSFUL 14
#define STRUCT_ERR_STD 20
#define STRUCT_ERR_DMG 21
#define STRUCT_ERR_NOT_EXIST 22
#define STRUCT_ERR_INCOMPLETE 23
#define FILE_ERR_NOT_OPENED 31
#define FILE_ERR_NOT_CLOSED 32
#define FILE_ERR_PERMISSION_DENIED 33
#define FUNC_ERR_TRY_AGAIN 41
#define NETWORK_ERR_CONNECTION_CLOSED 1000
#define NETWORK_ERR_NO_CONNECTION 1001
#define NETWORK_ERR_RARE_INTERFACES 1002

#define ERR_OVER_9000 9000






extern unsigned char do_debug;
//TODO: Define certain specified Debug-Levels
extern char ansi_escape_use;

extern char **args;


//extern double getRealTime();



extern char system_endianess;
/* ===================================================================== */
#define ENDIANESS_LITTLE 0
#define ENDIANESS_BIG 1
#define ENDIANESS_UNKNOWN 2
/* ===================================================================== */





#define ANSI_COLOR_BLACK "\033[22;30m"
#define ANSI_COLOR_RED "\033[22;31m"
#define ANSI_COLOR_GREEN "\033[22;32m"
#define ANSI_COLOR_BROWN "\033[22;33m"
#define ANSI_COLOR_BLUE "\033[22;34m"
#define ANSI_COLOR_MAGENTA "\033[22;35m"
#define ANSI_COLOR_CYAN "\033[22;36m"
#define ANSI_COLOR_GRAY "\033[22;37m"
#define ANSI_COLOR_DARK_GRAY "\033[01;30m"
#define ANSI_COLOR_LIGHT_RED "\033[01;31m"
#define ANSI_COLOR_LIGHT_GREEN "\033[01;32m"
#define ANSI_COLOR_YELLOW "\033[01;33m"
#define ANSI_COLOR_LIGHT_BLUE "\033[01;34m"
#define ANSI_COLOR_LIGHT_MAGENTA "\033[01;35m"
#define ANSI_COLOR_LIGHT_CYAN "\033[01;36m"
#define ANSI_COLOR_WHITE "\033[01;37m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define ANSI_COLOR_black ANSI_COLOR_BLACK
#define ANSI_COLOR_red ANSI_COLOR_RED
#define ANSI_COLOR_green ANSI_COLOR_GREEN
#define ANSI_COLOR_brown ANSI_COLOR_BROWN
#define ANSI_COLOR_blue ANSI_COLOR_BLUE
#define ANSI_COLOR_magenta ANSI_COLOR_MAGENTA
#define ANSI_COLOR_cyan ANSI_COLOR_CYAN
#define ANSI_COLOR_gray ANSI_COLOR_GRAY
#define ANSI_COLOR_dark_gray ANSI_COLOR_DARK_GRAY
#define ANSI_COLOR_light_red ANSI_COLOR_LIGHT_RED
#define ANSI_COLOR_light_green ANSI_COLOR_LIGHT_GREEN
#define ANSI_COLOR_yellow ANSI_COLOR_YELLOW
#define ANSI_COLOR_light_blue ANSI_COLOR_LIGHT_BLUE
#define ANSI_COLOR_light_magenta ANSI_COLOR_LIGHT_MAGENTA
#define ANSI_COLOR_light_cyan ANSI_COLOR_LIGHT_CYAN
#define ANSI_COLOR_white ANSI_COLOR_WHITE
#define ANSI_COLOR_reset ANSI_COLOR_RESET

#define ANSI_COLOR_Black ANSI_COLOR_BLACK
#define ANSI_COLOR_Red ANSI_COLOR_RED
#define ANSI_COLOR_Green ANSI_COLOR_GREEN
#define ANSI_COLOR_Brown ANSI_COLOR_BROWN
#define ANSI_COLOR_Blue ANSI_COLOR_BLUE
#define ANSI_COLOR_Magenta ANSI_COLOR_MAGENTA
#define ANSI_COLOR_Cyan ANSI_COLOR_CYAN
#define ANSI_COLOR_Gray ANSI_COLOR_GRAY
#define ANSI_COLOR_Dark_gray ANSI_COLOR_DARK_GRAY
#define ANSI_COLOR_Light_Red ANSI_COLOR_LIGHT_RED
#define ANSI_COLOR_Light_Green ANSI_COLOR_LIGHT_GREEN
#define ANSI_COLOR_Yellow ANSI_COLOR_YELLOW
#define ANSI_COLOR_Light_Blue ANSI_COLOR_LIGHT_BLUE
#define ANSI_COLOR_Light_Magenta ANSI_COLOR_LIGHT_MAGENTA
#define ANSI_COLOR_Light_Cyan ANSI_COLOR_LIGHT_CYAN
#define ANSI_COLOR_White ANSI_COLOR_WHITE
#define ANSI_COLOR_Reset ANSI_COLOR_RESET


#endif /* OLLERUS_BASE_H */
