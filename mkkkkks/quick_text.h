/* † MKKKKKS † */
/*!	@brief  Quick Text Utilities
	@author  NullPopPo
*/
#ifndef QUICK_TEXT_H__
#define QUICK_TEXT_H__

#include <stdbool.h>

//! callback by text extracted
/*!	@param user  any user pointer
	@param bgn  first letter pointer of the text
	@param end  terminate pointer of the text
	@param attention  the text is not terminated and don't use for standard C string
	@retval false  the caller function will aborted
	@retval true  the caller function is continueable
*/
typedef bool (*QTextExtracted)(void* user,const char* bgn,const char* end);

#ifdef __cplusplus
extern "C" {
#endif

//! create standard C string from QText 
/*!	@param bgn  first letter pointer of target text
	@param end  terminate pointer of target text
	@return  pointer of copied standard C string
*/
char* qtext_alloc(const char* bgn,const char* end);

//! free allocated string 
/*!	@param instp  pointer of stored from qtext_alloc()
	@note  free target memory and instance pointer be NULL
*/
void qtext_free(char** instp);

//! extract lines from QText 
/*!	@param user  any user pointer
	@param bgn  first letter pointer of target text
	@param end  terminate pointer of target text
	@param cbext  call by each extracted QText
	@return  pointer of processed in the function
*/
const char* qtext_ext_lines(void* user,const char* bgn,const char* end,QTextExtracted cbext);

#ifdef __cplusplus
}
#endif

#endif // QUICK_LOADER_H__
