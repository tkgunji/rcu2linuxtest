// $Id: mrshellprim.h,v 1.15 2006/06/15 09:53:24 richter Exp $

/************************************************************************
**
**
** This file is property of and copyright by the Experimental Nuclear 
** Physics Group, Dep. of Physics and Technology
** University of Bergen, Norway, 2004
** This file has been written by Matthias Richter,
** Matthias.Richter@ift.uib.no
**
** Permission to use, copy, modify and distribute this software and its  
** documentation strictly for non-commercial purposes is hereby granted  
** without fee, provided that the above copyright notice appears in all  
** copies and that both the copyright notice and this permission notice  
** appear in the supporting documentation. The authors make no claims    
** about the suitability of this software for any purpose. It is         
** provided "as is" without express or implied warranty.                 
**
*************************************************************************/

#ifndef _MRSHELLPRIM_H
#define _MRSHELLPRIM_H

#include <stdio.h>

/**
 * @file mrshellprim.h
 * @brief A command line parser.
 * @author Matthias Richter
 */


#ifdef __cplusplus
extern "C" { 
#endif


/**
 * The operation identifiers.
 * Each of the identifiers defines what to expect as the next argument(s) and what to do after
 * successful scan.
 */
enum operation_t {
  eUnknownType = 0,
  /**
   * Expect decimal value as next argument and read value into variable
   * TaggedData_t.Int
   */
  eInteger,
  /**
   * Expects a sequence of integers, size of array specified in user.size
   * TaggedData_t.pInt & ArgDef_t.size
   */
  eIntegerArray,
  /**
   * Expect float value as next argument and read value into variable
   * TaggedData_t.Float
   */
  eFloat,
  /**
   * Expects a sequence of floats, size of array specified in user.size
   * TaggedData_t.pFloat & ArgDef_t.size
   */
  eFloatArray,
  /**
   * Expect hexadecimal value as next argument and read value into variable
   * TaggedData_t.Hex
   */
  eHex,
  /**
   * Expects a sequence of hexadecimal arguments, size of array specified in user.size
   * TaggedData_t.pHex & ArgDef_t.size
   */
  eHexArray,
  /**
   * Expect const char string as next argument, only applicable if all arguments zero seperated
   * the pString pointer of the TaggedData_t structure is set to the string
   * TaggedData_t.pString
   */
  eConstString,
  /**
   * Character array to receive the argument, size of array specified in user.size
   * the string is copied into the provided buffer
   * TaggedData_t.arrayChar & ArgDef_t.size
   */
  eCharArray,
  /**
   * Array of different data types, zero terminated.
   * <b>Note:</b> not yet implemented
   */
  eComposite,
  /**
   * Call an argument-less function
   */
  eFctNoArg,
  /**
   * Call a function with an integer argument
   * the user data is treated as integer and passed to the function.
   */
  eFctIndex,
  /**
   * Recurse scan function with remaining arguments.
   */
  eFctRemaining,
  /**
   * Recurse scan function with arguments including the current.
   */
  eFctInclusive,
  /**
   * A user defined scan function, the current argument and the remaining arguments are passed
   */
  eFctUserScan,
  /**
   * A function with an array of integers
   */
  eFctIntegerArgs,
  /**
   * A function with an array of floats
   */
  eFctFloatArgs,
  /**
   * A function with an array of hexadecimal values
   */
  eFctHexArgs,
  /**
   * A function with an array of TTagged data members, terminated by an eUnknownType element
   */
  eFctCompositeArgs,
  /**
   * A recursive scan is done with the argument definition structure and passed to the function 
   */
  eFctArgDef,
  /**
   * The Int member of the tagged data is set if the argument was found
   */
  eBool,
  /**
   * The pInt member of the tagged data is set with the pattern setflag if the argument was found
   */
  eFlag,
};

/** Type define for the argument definition structure */
typedef struct ArgDef_t  TArgDef;
/** Type define for the function mode structure */
typedef struct FctMode_t TFctMode;
/** Type define for the function arg structure */
typedef struct FctArg_t TFctArg;
/** Type define for the tagged data structure */
typedef struct TaggedData_t TTaggedData;

/** Type define for a processing function with no arguments */
typedef int (*FunctionNoArg)();
/** Type define for a processing function with an int index argument */
typedef int (*FctIndex)(int majorNo);
/** Type define for a user defined scan function */
typedef int (*FctUserScan)(const char* currentArg, const char** arrayArg, int iNofArgs, void* pUser, FILE* pOut);
/** Type define for a processing function with an integer array argument */
typedef int (*FctInteger)(int* array, int iNofArgs, FILE* pOut);
/** Type define for a processing function with a float array argument */
typedef int (*FctFloat)(float* array, int iNofArgs, FILE* pOut);
/** Type define for a processing function with a hex array argument */
typedef int (*FctHex)(unsigned long* array, int iNofArgs, FILE* pOut);
/** Type define for a processing function with a composite array argument */
typedef int (*FctComposite)(TTaggedData* array, int iNofArgs, FILE* pOut);
/** Type define for a processing function for a successful argument scan */
typedef int (*FctArgDef)(TArgDef* pDef, void* pUser, FILE* pOut);

/**
 * Flags for the @ref ScanArguments function
 */

/** require the sequence of all mandatory arguments */
#define SCANMODE_STRICT_SEQUENCE   0x0001
/** terminate at the first not-recognized argument  */
#define SCANMODE_FORCE_TERMINATION 0x0002
/** write an unknown sequence to the output  */
#define SCANMODE_PRINT_UKWN_SEQU   0x0004
/** dont report an error in case of unknown argument */
#define SCANMODE_SILENT            0x0008
/** skip an unknown sequence and continue */
#define SCANMODE_SKIP_UKWN_SEQU    0x0010
/** read only one command with all the arguments and terminate */
#define SCANMODE_READ_ONE_CMD      0x0020
/** do not clean volatile flags at the beginning of the argument scan */
#define SCANMODE_PERSISTENT        0x0040

// bitshifts and masks for return value of scan function
#define SCANRET_BITSHIFT_PROCESSED_ARGS  8
#define SCANRET_BITSHIFT_OFFSET_LAST_ARG 0
#define SCANRET_BITSHIFT_INDEX           16
#define SCANRET_MASK_PROCESSED_ARGS      0x0000ff00
#define SCANRET_MASK_OFFSET_LAST_ARG     0x000000ff
#define SCANRET_MASK_INDEX               0x00ff0000
#define SCANRET_INVAL_INDEX              0x000000ff

#define SCANRET_GET_PROCESSED_ARGS ((x&SCANRET_MASK_PROCESSED_ARGS)>>SCANRET_BITSHIFT_PROCESSED_ARGS)
#define SCANRET_GET_OFFSET_LAST_ARG ((x&SCANRET_MASK_OFFSET_LAST_ARG)>>SCANRET_BITSHIFT_OFFSET_LAST_ARG)

/**
 * flags for argument definition
 */

/** mark the argument as mandatory */
#define ARGDEF_MANDATORY      0x0001
/** mark the argument as optional */
#define ARGDEF_OPTIONAL       0x0002
/** the argument will be ignored for further check if read once */
#define ARGDEF_ONLY_ONCE      0x0004
/** skip the argument if the required additional argument scan failed */
#define ARGDEF_SKIP_IF_FAILED 0x0008
/** if additional scans fail resume and try to find another entry in the definition array */
#define ARGDEF_RESUME         0x0010
/** the short argument does not need to be terminated by a separator */
#define ARGDEF_UNTERM_SHORT   0x0020
/** the long argument does not need to be terminated by a separator */
#define ARGDEF_UNTERM_LONG    0x0040
/** terminate the argument scan after this argument was processed */
#define ARGDEF_TERMINATE      0x0080
/** terminate without processing with -EINTR if this argument was found */
#define ARGDEF_EXIT           0x0100
/** terminate without processing if this argument was found */
#define ARGDEF_BREAK          0x0200
/** dont call subfunctions for this argument definition */
#define ARGDEF_DELAY_EXECUTE  0x0400
/** this is a keywordless argument, start scanning the parameters from the argument itself */
#define ARGDEF_KEYWORDLESS    0x0800

/**
 * @struct TaggedData_t
 * @brief Named Data elements to be specified as target in an argument definition
 * @ref ArgDef_t.
 * The <i>type</i> variable specifies the name and which of the other members has
 * to be used.
 */
struct TaggedData_t {
  /** type of the data pointer */
  int   type;
  /** type dependend data structure */
  union  {
    /** void pointer element */
    void*         pVoid;
    /** int element for types @ref eInteger and @ref eBool */
    int           Int;
    /** int array element for types @ref eIntegerArray and @ref eFlag */
    int*          pInt;
    /** float element for type @ref eFloat */
    float         Float;
    /** float array element for type @ref eFloatArray */
    float*        pFloat;
    /** uint element for type @ref eHex */
    unsigned int  Hex;
    /** uint array element for type @ref eHexArray */
    unsigned int* pHex;
    /** target for type @ref eConstString */
    const char*   pString;
    /** target for type @ref eCharArray */
    char*         arrayChar;
    /** processing function for types @ref eFctRemaining and @ref eFctInclusive */
    TArgDef*      pSubArgDef;
    /**  function for type @ref eFctNoArg */
    FunctionNoArg pFctNoArg;
    /** processing function for type @ref eFctIndex */
    FctIndex      pFctIndex;
    /** processing function for type @ref eFctUserScan */
    FctUserScan   pFctUser;
    /** processing function for type @ref eFctIntegerArgs */
    FctInteger    pFctInteger;
    /** processing function for type @ref eFctFloatArgs */
    FctFloat      pFctFloat;
    /** processing function for type @ref eFctHexArgs */
    FctHex        pFctHex;
    /** processing function for type @ref eFctCompositeArgs */
    FctComposite  pFctComposite;
    /** processing function for type @ref eFctArgDef */
    FctArgDef     pFctArgDef;
  };
};

/**
 * @struct ArgDef_t
 * @brief The argument definition.
 */ 
struct ArgDef_t {
  /** short version of the argument */
  const char* s;
  /** long version of the argument */
  const char* l;
  /** type dependend data structure */
  TTaggedData data;
  /** additional type dependend data */
  union {
   /** pointer to user defined data passed to a sub-function */
    void*         pUser;
    /** mode for the next recursion */
    TFctMode*     pFctMode;
    TTaggedData*  arrayComposite;
    /** parameter to the eFctArgDef type */
    TFctArg*      pFctArg;
    /** size of arrays */
    int           size;
    /** the flag pattern for the eFlag type */
    unsigned int  setFlagPattern;
  };
  /** scanning flags specific to this argument */
  unsigned int    flags;
};

/**
 * @struct FctMode_t
 * @brief Composite scan mode parameter.
 */
struct FctMode_t {
  /** global scanning flags for the current invocation of @ref ScanArguments */
  unsigned int flags;
  /** additional delimiters for arguments */
  const char* pDelimiters;
  /** a file descriptor to redirect output */
  FILE* pOutput;
};

/**
 * @struct FctArg_t
 * @brief Composite parameter to specify argument definition, scan mode and
 * an additional user pointer for type @ref eFctArgDef.
 */
struct FctArg_t {
  /** the argument definition for the sub-scan */
  TArgDef*  pDef;
  /** the scanning flags for the sub-scan */
  TFctMode* pMode;
  /** user pointer passed to the processing function of type @ref FctArgDef */
  void*     pUser;
};

/**
 * Scan a list or single string of arguments.
 * with respect to the definitions of arguments as entries in the definition array, sub-functions can be called and float,
 * decimal and hexadecimal values can be read
 * @param arrayArg       array of argument strings
 * @param iNofArgs       size of the array
 * @param iFirstArgStart start of scan within the first element of the array
 * @param pSeparator     a zero terminated string with additional separators, the \0 value is always treated as separator
 * @param arrayDef       an array of TArgDef elements, each defining a known argument type
 * @param flags
 * @return >=0 successfull, offset coded<br>
 *   bit 8-15: number of scanned elements of argArray<br>
 *   bit  0-7: offset in last processed element<br>
 *   -EINTR if an argument with ARGDEF_TERMINATE or ARGDEF_BREAK was found
 */
int ScanArguments(const char** arrayArg, int iNofArgs, int iFirstArgStart, TArgDef* arrayDef, TFctMode *pMode);

/**
 * Print a summury on the argument definition structure.
 * The type and eventually scanned values are printed out together with the flags
 * set for the element. This is a debug feature.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param bAll
 * @return >=0 success, neg. error code if failed
 */
int PrintArgumentDefinition(TArgDef* arrayDef, int bAll);

/**
 * @name Parser debugging
 */

/** mask for the parser debugging bits */
#define DBG_SHELLPRIM_MASK          0xff0000
/** debug output of the SearchDef function */
#define DBG_SEARCH_ARG_DEF          0x010000
/** detailed debug output of the SearchDef function */
#define DBG_SEARCH_ARG_DEF_DETAIL   0x030000
/** debug output of the ScanArguments function */
#define DBG_SCAN_ARG_DEF            0x040000
/** detailed debug output of the ScanArguments function */
#define DBG_SCAN_ARG_DEF_DETAIL     0x0c0000
/** debug output of the ReadArgument function */
#define DBG_ARGUMENT_READ           0x100000
/** print debug information concerning argument conversion */
#define DBG_ARGUMENT_CONVERT        0x800000

/**
 * Set a debug flag for the parser.
 * @param flag the flag set set
 * @return the current value of the flags
 */
unsigned int mrShellPrimSetDebugFlag(unsigned int flag);

/**
 * Clear a debug flag for the parser.
 * @param flag the flag set set
 * @return the current value of the flags
 */
unsigned int mrShellPrimClearDebugFlag(unsigned int flag);

/**
 * Print help information on the parser debug flags.
 */
int mrShellPrimPrintDbgFlags();

/**
 * @name Helper functions
 */

/**
 * Remove preceeding and trailing special characters and blanks.
 * Until now the function removes blanks at the beginning and newlines
 * at the end.<br>
 * <b>Note:</b> The function alters the buffer.
 * @param pCmd   char buffer holding the command
 * @return pointer to the first non-special character of the buffer if succeeded
 */
char* removePrecAndTrailingSpecChars(char* pCmd);

/**
 * Scan a string for a hex number.
 * @param arg      the string
 * @param pNumber  target to receive the hex number
 * @param bWarning display a warning if argument could not be converted
 * @return >=0 if succeeded, neg. error code if failed
 */
int getHexNumberFromArg(const char* arg, unsigned int* pNumber, int bWarning);

/**
 * Scan a string for a decimal number.
 * @param arg      the string
 * @param pNumber  target to receive the decimal number
 * @param bWarning display a warning if argument could not be converted
 * @return >=0 if succeeded, neg. error code if failed
 */
int getDecNumberFromArg(const char* arg, int* pNumber, int bWarning);

/**
 * Scan a string for a float number.
 * @param arg      the string
 * @param pNumber  target to receive the float number
 * @param bWarning display a warning if argument could not be converted
 * @return >=0 if succeeded, neg. error code if failed
 */
int getFloatNumberFromArg(const char* arg, float* pNumber, int bWarning);

/**
 * Scan a buffer of characters, separate commands and extract pointers to single command parameters
 *
 * @param pCmdLine      pointer to zero terminated input char buffer, the content is changed if bSeparate==1!!!
 * @param pTargetArray  array to receive pointers to separated strings
 * @param iArraySize    size of the target array
 * @param bSeparate     blanks and special characters set to zero to separate two command parameters
 * @return  number of parameters if succeeded, pointers to separated strings in taget array<br>
 *          neg. error code if failed
 */
int buildArgumentsFromCommandLine(char* pCmdLine, char** pTargetArray, int iArraySize, int bSeparate, int iDebugFlag);

/**
 * Search the argument definition for a command.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param iType    if not equal to eUnknownType a type check is done
 * @return >=0 success, index of the element
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 */
int mrShellPrimGetIndex(TArgDef* arrayDef, const char* pCmd, int iType);

/**
 * Set data pointer for an argument definition element.
 * The function is only available for other arguments than of type @ref eInteger, eBool, iFloat and iHex.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param pData    void pointer to data
 * @param iType    if not equal to eUnknownType a type check is done
 * @return >=0 success, index of the element in lower 16bit
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 *              -ENOSYS  function not available for this type<br>
 */
int mrShellPrimSetData(TArgDef* arrayDef, const char* pCmd, void* pData, int iType);

/**
 * Get data pointer for an argument definition element.
 * The function is only available for other arguments than of type @ref eInteger, eBool, iFloat and iHex.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param pFloat   data target
 * @param ppData   pointer to void pointer to receive data pointer
 * @param iType    if not equal to eUnknownType a type check is done
 * @return >=0 success, index of the element in lower 16bit; bit 16 set if this argument was found.<br>
 *         the ARGPROC_INDEX and ARGPROC_EXISTS macros can be used to extract to parts<br>
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 *              -ENOSYS  function not available for this type<br>
 */
int mrShellPrimGetData(TArgDef* arrayDef, const char* pCmd, void** ppData, int iType);

/**
 * Get float value for an argument definition element.
 * The function is only available arguments of type iFloat.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param pFloat   data target
 * @return >=0 success, index of the element in lower 16bit; bit 16 set if this argument was found.<br>
 *         the ARGPROC_INDEX and ARGPROC_EXISTS macros can be used to extract to parts<br>
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 *              -ENOSYS  function not available for this type<br>
 */
int mrShellPrimGetFloat(TArgDef* arrayDef, const char* pCmd, float* pFloat);

/**
 * Get integer value for an argument definition element.
 * The function is only available arguments of type iInteger and eBool.
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param pInt     data target
 * @return >=0 success, index of the element in lower 16bit; bit 16 set if this argument was found.<br>
 *         the ARGPROC_INDEX and ARGPROC_EXISTS macros can be used to extract to parts<br>
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 *              -ENOSYS  function not available for this type<br>
 */
int mrShellPrimGetInt(TArgDef* arrayDef, const char* pCmd, int* pInt);

/**
 * Get hexadecimal value for an argument definition element.
 * The function is only available for arguments of type iHex
 * @param arrayDef argument definition array, terminated by an element of type @ref eUnknownType
 * @param pCmd     command string, both the short and the long definition are compared
 * @param pHex     data target
 * @return >=0 success, index of the element in lower 16bit; bit 16 set if this argument was found.<br>
 *         the ARGPROC_INDEX and ARGPROC_EXISTS macros can be used to extract to parts<br>
 *         neg. error code if failed<br>
 *              -ENOENT  entry not found<br>
 *              -EBADF   entry found, but of wrong type<br>
 *              -EINVAL  invalid argument<br>
 *              -ENOSYS  function not available for this type<br>
 */
int mrShellPrimGetHex(TArgDef* arrayDef, const char* pCmd, unsigned int* pHex);

/**
 * Clone an argument definition.
 * @param arrayDef pointer to definition array, terminated by @ref eUnknownType element
 * @return pointer to new definition array<br>
 * The allocated memory has to be released by the caller
 */
TArgDef* mrShellPrimCloneDef(TArgDef* arrayDef);

/**
 * Reset volatile processing flags of the definition array.
 * Reseted flags: @ref ARGPROC_FOUND
 * @param arrayDef pointer to definition array, terminated by @ref eUnknownType element
 * @return >=0 success, neg. error code if failed
 */
int mrShellPrimResetVolatileFlags(TArgDef* arrayDef);


/**
 * Result encoding for the  @ref mrShellPrimGetData, @ref mrShellPrimGetFloat, 
 * @ref mrShellPrimGetInt and @ref mrShellPrimGetHex functions
 */
#define ARGPROC_INDEX_BITSHIFT   0
/**
 * Result encoding for the  @ref mrShellPrimGetData, @ref mrShellPrimGetFloat, 
 * @ref mrShellPrimGetInt and @ref mrShellPrimGetHex functions
 */
#define ARGPROC_INDEX_WIDTH     16
/**
 * Result encoding for the  @ref mrShellPrimGetData, @ref mrShellPrimGetFloat, 
 * @ref mrShellPrimGetInt and @ref mrShellPrimGetHex functions
 */
#define ARGPROC_EXISTS_BITSHIFT 16
/**
 * Result encoding for the  @ref mrShellPrimGetData, @ref mrShellPrimGetFloat, 
 * @ref mrShellPrimGetInt and @ref mrShellPrimGetHex functions
 */
#define ARGPROC_EXISTS_WIDTH     1

/** helper macro to extract if the argument was found or not from the result of the @ref
 * mrShellPrimGetData, @ref mrShellPrimGetFloat, @ref mrShellPrimGetInt and
 * @ref mrShellPrimGetHex functions
 */
#define ARGPROC_EXISTS(x) ((x>>ARGPROC_EXISTS_BITSHIFT)&((0x1<<ARGPROC_EXISTS_WIDTH)-1))
/**
 * helper function to extract the index from the result of the @ref
 * mrShellPrimGetData, @ref mrShellPrimGetFloat, @ref mrShellPrimGetInt and
 * @ref mrShellPrimGetHex functions
 */
#define ARGPROC_INDEX(x) ((x>>ARGPROC_INDEX_BITSHIFT)&((0x1<<ARGPROC_INDEX_WIDTH)-1))

#ifdef __cplusplus
}
#endif

#endif/** _MRSHELLPRIM_H */
