/*************************************************************/
/* Author: Lizmary Delarosa                                 */
/* Filename: clientparse.h                                   */
/* Purpose: This header file declares the parsemenu function,*/
/*          responsible for parsing command-line arguments. */
/*          It also includes the struct options variables   */
/*          that will be declared in clientparse.cpp.       */
/*************************************************************/
#ifndef PARSE_H
#define PARSE_H

#include <string>
using namespace std;

struct options {
    string hostname;
    string port;
};

/*************************************************************************/
/* Function name: parsemenu                                             */
/* Description: Parses command-line arguments to determine the port,    */
/*              the directory, and the call to verbose. Port is         */
/*              required by the user, while the directory and verbose   */
/*              have default values.                                    */
/* Parameters: int argc - The number of command-line arguments          */
/*             char* argv[] - Array of command-line arguments           */
/* Return Value: struct options                                         */
/*************************************************************************/
struct options parsemenu(int argc, char* argv[]);

#endif
