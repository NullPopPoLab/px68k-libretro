/* † MKKKKKS † */
/*!	@brief  Advanced M3U Loader
	@author  NullPopPo
*/
#include "./advanced_m3u.h"
#include "./quick_text.h"
#include "./safe_alloc.h"
#include <stdlib.h>
#include <string.h>

static void am3u_device_reset(AdvancedM3UDevice* inst);
static bool am3u_device_add_media_internal(AdvancedM3UDevice* inst,int slot,bool readonly,
	const char* path,size_t pathlen,const char* label,size_t labellen);

static void am3u_media_reset(AdvancedM3UMedia* inst);
static void am3u_media_set(AdvancedM3UMedia* inst,bool readonly,const char* path,size_t pathlen,const char* label,size_t labellen);

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
AdvancedM3U* am3u_new(){

	AdvancedM3U* inst=safe_alloc(sizeof(AdvancedM3U));
	if(!inst)return NULL;
	am3u_reset(inst);
	return inst;
}

/* ----------------------------------------------------------------------- */
void am3u_free(AdvancedM3U** instp){
	if(!*instp)return;
	am3u_reset(*instp);
	safe_free(instp);
}

/* ----------------------------------------------------------------------- */
void am3u_reset(AdvancedM3U* inst){

	int c;
	for(c=AM3U_DEVICE_MIN;c<=AM3U_DEVICE_MAX;++c){
		am3u_device_reset(&inst->device_tbl[c-AM3U_DEVICE_MIN]);
	}
	inst->device_default=0;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
typedef struct AdvancedM3UParseWork_{
	AdvancedM3U* inst;
	AdvancedM3UParseError cberr;
	void* user;
	int lineloc;
	const char* linebgn;
	const char* lineend;
	bool aborted;
}AdvancedM3UParseWork;

/* ----------------------------------------------------------------------- */
static bool am3u_parse_error(AdvancedM3UParseWork* work,int code){

	if(!work->cberr)return true; // ignore all errors 
	work->aborted=!work->cberr(work->user,code,work->lineloc,work->linebgn,work->lineend);
	return !work->aborted;
}

/* ----------------------------------------------------------------------- */
static bool am3u_parse_line(void* user,const char* bgn,const char* end){

	AdvancedM3UParseWork* work=user;
	AdvancedM3U* inst=work->inst;
	const char* cur=bgn;
	const char* path;
	const char* label;
	size_t pathlen,labellen;
	int device=inst->device_default,slot=0;
	bool readonly=false;

	++work->lineloc;
	work->linebgn=bgn;
	work->lineend=end;

	// NOP 
	if(cur>=end)return true;
	if(*cur=='#')return true;

	if(*cur=='*'){
		// advanced feature 
		if(++cur>=end){
			// missing ; 
			return am3u_parse_error(work,AM3U_ERROR_SYNTAX);
		}

		// device letter 
		if(*cur>=AM3U_DEVICE_MIN && *cur<=AM3U_DEVICE_MAX){
			device=*cur-AM3U_DEVICE_MIN;
			if(++cur>=end){
				// missing ; 
				return am3u_parse_error(work,AM3U_ERROR_SYNTAX);
			}
		}
		else{
			// Invalid 
			return am3u_parse_error(work,AM3U_ERROR_INVALID_DEVICE);
		}

		// slot number (1 origin, 0 is default)
		while(*cur>='0' && *cur<='9'){
			slot=slot*10+*cur-'0';
			if(++cur>=end){
				// missing ; 
				return am3u_parse_error(work,AM3U_ERROR_SYNTAX);
			}
		}
		if(slot>inst->device_tbl[device].slot_max){
			if(!am3u_parse_error(work,AM3U_ERROR_INVALID_SLOT))return false;
			slot=0;
		}

		// read only flag 
		if(*cur=='!'){
			readonly=true;
			if(++cur>=end){
				// missing ; 
				return am3u_parse_error(work,AM3U_ERROR_SYNTAX);
			}
		}

		// end of advanced attributes 
		if(*cur==';'){
			if(++cur>=end){
				// missing path 
				return am3u_parse_error(work,AM3U_ERROR_EMPTY_PATH);
			}
		}
		else{
			// missing ; 
			return am3u_parse_error(work,AM3U_ERROR_SYNTAX);
		}
	}
	else{
		// Standard Line 
		// try sequential slot number
		slot=inst->device_tbl[device].changee_used+1;
		if(slot>inst->device_tbl[device].slot_max)slot=0;
		else if(inst->device_tbl[device].slot_tbl[slot-1]>=0)slot=0;
	}

	if(inst->device_tbl[device].changee_used>=inst->device_tbl[device].changee_max){
		return am3u_parse_error(work,AM3U_ERROR_CHANGEE_OVER);
	}

	// split custom label 
	path=cur;
	label=memchr(cur,'|',end-cur);
	if(label){
		// custom label after | 
		pathlen=label-path;
		++label;
		labellen=end-label;
	}
	else{
		// no custom label 
		pathlen=end-cur;
		labellen=0;
	}

	// known errors are already detected
	// other errors means unknown

	if(!am3u_device_add_media_internal(&inst->device_tbl[device],slot,readonly,path,pathlen,label,labellen)){
		return am3u_parse_error(work,AM3U_ERROR_UNDEFINED);
	}
	return true;
}

/* ----------------------------------------------------------------------- */
bool am3u_parse(AdvancedM3U* inst,const char* src,size_t srclen,AdvancedM3UParseError cberr,void* user){

	AdvancedM3UParseWork work;
	work.inst=inst;
	work.cberr=cberr;
	work.user=user;
	work.lineloc=0;
	work.aborted=false;

	qtext_ext_lines(&work,src,src+srclen,am3u_parse_line);
	return !work.aborted;
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
bool am3u_set_default_device(AdvancedM3U* inst,char device){

	if(device<AM3U_DEVICE_MIN)return false;
	if(device>AM3U_DEVICE_MAX)return false;
	inst->device_default=device-AM3U_DEVICE_MIN;

	return true;
}

/* ----------------------------------------------------------------------- */
AdvancedM3UDevice* am3u_get_device(AdvancedM3U* inst,char device){

	if(device<AM3U_DEVICE_MIN)return NULL;
	if(device>AM3U_DEVICE_MAX)return NULL;
	return &inst->device_tbl[device-AM3U_DEVICE_MIN];
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
static void am3u_device_changer_reset(AdvancedM3UDevice* inst){

	int i;

	for(i=0;i<inst->changee_max;++i){
		am3u_media_reset(&inst->changee_tbl[i]);
	}

	inst->changee_max=0;
	inst->changee_used=0;
	safe_free(&inst->changee_tbl);
}

/* ----------------------------------------------------------------------- */
static void am3u_device_slots_reset(AdvancedM3UDevice* inst){

	inst->slot_max=0;
	safe_free(&inst->slot_tbl);
}

/* ----------------------------------------------------------------------- */
void am3u_device_reset(AdvancedM3UDevice* inst){

	am3u_device_changer_reset(inst);
	am3u_device_slots_reset(inst);
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void am3u_device_set_changer(AdvancedM3UDevice* inst,int size){

	am3u_device_changer_reset(inst);
	if(size<1)return;
	inst->changee_tbl=safe_alloc(sizeof(AdvancedM3UMedia)*size);
	if(inst->changee_tbl)inst->changee_max=size;
}

/* ----------------------------------------------------------------------- */
void am3u_device_set_slots(AdvancedM3UDevice* inst,int size){

	int i;

	am3u_device_slots_reset(inst);
	if(size<1)return;
	inst->slot_tbl=safe_alloc(sizeof(int*)*size);
	if(inst->slot_tbl){
		inst->slot_max=size;
		for(i=0;i<size;++i)inst->slot_tbl[i]=-1;
	}
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
bool am3u_device_add_media_internal(AdvancedM3UDevice* inst,int slot,bool readonly,
	const char* path,size_t pathlen,const char* label,size_t labellen){

	AdvancedM3UMedia* media;
	int idx;

	if(inst->changee_used>=inst->changee_max)return false;

	// add media 
	idx=inst->changee_used;
	media=&inst->changee_tbl[inst->changee_used++];
	am3u_media_set(media,readonly,path,pathlen,label,labellen);

	// set slot 
	if(slot>0 && slot<=inst->slot_max)inst->slot_tbl[slot-1]=idx;
	return true;
}

/* ----------------------------------------------------------------------- */
bool am3u_device_add_media(AdvancedM3UDevice* inst,int slot,bool readonly,const char* path,const char* label){
	return am3u_device_add_media_internal(inst,slot,readonly,path,strlen(path),label,strlen(label));
}

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
void am3u_media_reset(AdvancedM3UMedia* inst){

	inst->ready=false;
	inst->readonly=false;
	qtext_free(&inst->path);
	qtext_free(&inst->label);
}

/* ----------------------------------------------------------------------- */
void am3u_media_set(AdvancedM3UMedia* inst,bool readonly,const char* path,size_t pathlen,const char* label,size_t labellen){

	am3u_media_reset(inst);
	inst->ready=true;
	inst->readonly=readonly;
	inst->path=qtext_alloc(path,path+pathlen);
	if(label)inst->label=qtext_alloc(label,label+labellen);
}
