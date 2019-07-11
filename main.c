/*
 *main.cpp
 */
/* Within
 *  Project | Properties | C/C++-Build | Settings
 *  | GCC C++ Compiler | Preprocessor
 * set the following defined Symbol:
 *  _FILENAME=${ConfigName}
 */
#define __QUOT2__(x) #x
#define __QUOT1__(x) __QUOT2__(x)
#include __QUOT1__(_FILENAME.c)
#undef __QUOT1__
#undef __QUOT2__
/* The above include directive will include the file ${CfgName}.cpp,
 * wherein ${CfgName} is the name of the build configuration currently
 * active in the project.
 *
 * When right clicking in
 *  Project Tree | (Project)
 * and selecting
 *  Build Configuration | Build all
 * this file will include the corresponding .cpp file  named after the
 * build config and thereby effectively take that file as a main file.
 *
 * Remember to exclude ALL ${CfgName}.cpp files from ALL build configurations.
 */
