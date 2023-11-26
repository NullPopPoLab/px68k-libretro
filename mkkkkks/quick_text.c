/* † MKKKKKS † */
/*!	@brief  Quick Text Utilities
	@author  NullPopPo
*/
#include "./quick_text.h"
#include <stdlib.h>

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
char* qtext_alloc(const char* bgn,const char* end){

	char* inst=NULL;
	int l=end-bgn;
	const char* cur;

	if(l<0)return NULL;
	inst=malloc(l+1);
	for(cur=bgn;cur<end;++cur)inst[cur-bgn]=*cur;
	inst[l]=0;
	return inst;
}

/* ----------------------------------------------------------------------- */
void qtext_free(char** instp){

	if(!*instp)return;
	free(*instp);
	*instp=NULL;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
const char* qtext_ext_lines(void* user,const char* bgn,const char* end,QTextExtracted cbext)
{
	const char* crp=NULL;
	const char* lfp=NULL;
	const char* cur;

	// NOP 
	if(!cbext)return end;

	for(cur=bgn;cur<end;++cur){
		if(*cur=='\r'){
			if(crp){
				// CR...CR 
				if(!cbext(user,bgn,crp))return crp+1;
				bgn=crp+1;
			}
			else{
				// single CR needs postpone
			}
			crp=cur;
		}
		else if(*cur=='\n'){
			lfp=cur;
			if(crp){
				if(crp+1<lfp){
					// CR...LF 
					if(!cbext(user,bgn,crp))return crp+1;
					bgn=crp+1;
					if(!cbext(user,bgn,lfp))return lfp+1;
					bgn=lfp+1;
				}
				else{
					// CRLF 
					if(!cbext(user,bgn,crp))return lfp+1;
					bgn=lfp+1;
				}
				crp=NULL;
			}
			else{
				// LF 
				if(!cbext(user,bgn,lfp))return lfp+1;
				bgn=lfp+1;
			}
			lfp=NULL;
		}
	}

	if(crp){
		// CR...EOF 
		if(!cbext(user,bgn,crp))return crp+1;
		bgn=crp+1;
	}
	if(bgn<end){
		// ...EOF 
		cbext(user,bgn,end);
	}

	return end;
}
