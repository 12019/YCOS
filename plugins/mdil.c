#include "..\defs.h"
#include "..\config.h"
#include "..\midgard\midgard.h"
#include "..\asgard\file.h"
#include "..\asgard\security.h"
#include "..\liquid.h"
#include "..\framework\VAS.h"
#include "..\framework\SMS.h" 
#include "..\framework\dcs.h"
#include "dil.h"
#include <string.h> 

#if VAS_MDIL_ALLOCATED || VAS_VDIL_ALLOCATED
uchar dil_get_pin(uchar id) _REENTRANT_ {
	extern uchar STK_buffer[];
	uchar len;
	uchar j = 0;
	uchar params[4];
	len = VAS_load_text(0x6200, id, STK_buffer, 128);	
	params[0] = 0;
	params[1] = GET_INPUT;
	params[2] = 0x04;
	j += SAT_file_push(j, STK_TAG_CMD_DETAIL, 3, params);
	params[0] = STK_DEV_SIM;
	params[1] = STK_DEV_ME;
	j += SAT_file_push(j, STK_TAG_DEV_ID, 2, params);
	params[0] = STK_DEV_SIM;
	params[1] = STK_DEV_ME;
	j += SAT_file_push(j, STK_TAG_DEV_ID, 2, params);
	j += SAT_file_push(j, STK_TAG_TEXT_STRING, len, STK_buffer);
	params[0] = 4;
	params[1] = 4;
	j += SAT_file_push(j, STK_TAG_RESPONSE_LENGTH, 2, params);
	return j;	
}

uchar dil_display_text(uchar id) _REENTRANT_ { 
	extern uchar STK_buffer[];
	uchar j = 0;  
	uchar len;
	uchar params[4];
	len = VAS_load_text(0x6200, id, STK_buffer, 128);	
	params[0] = 0;
	params[1] = DISPLAY_TEXT;
	params[2] = 0x80;
	j += SAT_file_push(j, STK_TAG_CMD_DETAIL, 3, params);
	params[0] = STK_DEV_SIM;
	params[1] = STK_DEV_DISPLAY;
	j += SAT_file_push(j, STK_TAG_DEV_ID, 2, params);
	j += SAT_file_push(j, STK_TAG_TEXT_STRING, len, STK_buffer);
	return j;
}
#endif

#if VAS_MDIL_ALLOCATED

#define MDIL_STATE_IDLE						0
#define MDIL_STATE_DELETE_NEED_PIN			1
#define MDIL_STATE_DELETE_VERIFIED			2
#define MDIL_STATE_UPDATE_BOTH_NEED_PIN		17
#define MDIL_STATE_UPDATE_BOTH_VERIFIED		18
#define MDIL_STATE_UPDATE_BOTH_MAX			19
#define MDIL_STATE_UPDATE_BOTH_OVERWRITE	20
#define MDIL_STATE_UPDATE_BOTH_SELECTED		21
#define MDIL_STATE_UPDATE_NAME_NEED_PIN		9
#define MDIL_STATE_UPDATE_NAME_VERIFIED		10
#define MDIL_STATE_UPDATE_VAL_NEED_PIN		5
#define MDIL_STATE_UPDATE_VAL_VERIFIED		6

static uchar mdil_state = 0;

uint16 mdil_fetch(uchar * response, uint16 length) _REENTRANT_ {
	extern uchar * vas_cistr; 	 		//current input string
	extern uchar vas_cvid;				//current variable id	
	uchar i = 0;
	uchar tag;
	uchar size;
	uint16 status;
	if(SAT_init(response, length) == 0) return APDU_STK_OVERLOAD;
	while(i != length) {
		i += SAT_file_pop(i, &tag, &size, STK_buffer);
		tag |= 0x80; 
		switch(tag) {			//TERMINAL HANDLER
			case STK_TAG_RESULT:
				if((STK_buffer[0] & 0xF0) != 0) {
					mdil_state = MDIL_STATE_IDLE;
					VAS_exit_plugin();
					goto exit_fetch;
				}
				break;
			case STK_TAG_TEXT_STRING:
				STK_buffer[4] = 0xFF;
				STK_buffer[5] = 0xFF;
				STK_buffer[6] = 0xFF;
				STK_buffer[7] = 0xFF;
				status = chv_verify(1, STK_buffer);
				if(status == 0x9000) goto start_decode;		//PIN verification success
				if(status == APDU_ACCESS_DENIED) return SAT_file_flush(dil_get_pin(2));			//incorrect pin entry (try again)
				return SAT_file_flush(dil_get_pin(3));	 	//failed, turn of phone
			default: break;
		}
	}
	start_decode:
	switch(mdil_state) {
		case MDIL_STATE_DELETE_NEED_PIN: mdil_state = MDIL_STATE_DELETE_VERIFIED; break;
		case MDIL_STATE_UPDATE_BOTH_NEED_PIN: mdil_state = MDIL_STATE_UPDATE_BOTH_VERIFIED; break;
		case MDIL_STATE_UPDATE_NAME_NEED_PIN: mdil_state = MDIL_STATE_UPDATE_NAME_VERIFIED; break;
		case MDIL_STATE_UPDATE_VAL_NEED_PIN: mdil_state = MDIL_STATE_UPDATE_VAL_VERIFIED; break;
	}
	i = mdil_decode();
	if(i != 0) {
		SAT_file_flush(i);
	}
	exit_fetch:
	return VAS_decode();
}

uchar mdil_decode(void) _REENTRANT_ {			 //return response length
	extern uchar * vas_cistr; 	 		//current input string
	extern uchar vas_cvid;				//current variable id
	extern uchar STK_buffer[];
	fs_handle temp_fs;
	ef_header * curfile = NULL;
	uchar len;
	uchar i = 0;
	uchar j = 0;
	uchar mode;
	uint16 fid;
	if(vas_cistr == NULL) goto exit_plugin;		//no response, input string not exist
	len = vas_cistr[0];
	mode = vas_cistr[1];
	fid = (uint16)vas_cistr[2];
	fid <<= 8;
	fid |= vas_cistr[3];
	
	//select the corresponding file id
	_select(&temp_fs, FID_MF);
	_select(&temp_fs, FID_WIB);
	if(_select(&temp_fs, fid) < 0x9F00) goto exit_plugin;		//fid not found

	switch(mode & 0x18) {
		case MDIL_OP_DELETE:
			//check access
			if(_check_access(&temp_fs, FILE_WRITE) != APDU_SUCCESS) {
				mdil_state = MDIL_STATE_DELETE_NEED_PIN;
				return dil_get_pin(1);		
			}
			curfile = file_get_current_header(&temp_fs);
			memset(STK_buffer, 0xFF, curfile->rec_size);
			if(vas_cistr[4] > (curfile->size / curfile->rec_size)) {
				j = dil_display_text(6);
			} else {
				_writerec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
				if(mode & 1) {
					j = dil_display_text(5);
				}
			} 
			m_free(curfile);
			break;
		case MDIL_OP_ADD_UPD_BOTH:
			//check access
			if(_check_access(&temp_fs, FILE_WRITE) != APDU_SUCCESS) { 
				mdil_state = MDIL_STATE_UPDATE_BOTH_NEED_PIN;
				return dil_get_pin(1);		
			}
			curfile = file_get_current_header(&temp_fs); 
			memset(STK_buffer, 0xFF, curfile->rec_size);
			if(vas_cistr[4] > (curfile->size / curfile->rec_size)) {
				j = dil_display_text(6);  		//invalid record ID
			} else {
				i = 5;		//start of name data
				STK_buffer[0] = 0x04;		//8bit 
				j = 2;
				for(i=5, j=2; j<20 && i<len; i++, j++) { 		//copy name parameter
					if(vas_cistr[i] == 0xFF && vas_cistr[i+1] == 0xFF) {	//separator found
						STK_buffer[1] = (j-2);
						j = 20;
						i += 2;
					} else {
						STK_buffer[j] = vas_cistr[i];
					}
				}
				j = 20;
				STK_buffer[j++] = 0x04;
				STK_buffer[j++] = (len - i);
				for(i;i<len;i++,j++) {				   		//copy value parameter
					STK_buffer[j] = vas_cistr[i];	
				}
				_writerec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
				if(mode & 1) {
					j = dil_display_text(7);
				}
			} 

			m_free(curfile);
			break;
		case MDIL_OP_UPD_NAME:
			//check access
			if(_check_access(&temp_fs, FILE_WRITE) != APDU_SUCCESS) {
				mdil_state = MDIL_STATE_UPDATE_NAME_NEED_PIN;
				return dil_get_pin(1);		
			}
			curfile = file_get_current_header(&temp_fs);
			_readrec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
			memset(STK_buffer + 2, 0xFF, 18);
			if(vas_cistr[4] > (curfile->size / curfile->rec_size)) {
				j = dil_display_text(6);  		//invalid record ID
			} else {
				i = 5;		//start of name data
				STK_buffer[0] = 0x04;		//8bit 
				j = 2;
				for(i=5, j=2; j<20 && i<len; i++, j++) { 		//copy name parameter
					if(vas_cistr[i] == 0xFF && vas_cistr[i+1] == 0xFF) {	//separator found
						STK_buffer[1] = (j-2);
						j = 20;
						i += 2;
					} else {
						STK_buffer[j] = vas_cistr[i];
					}
				}
				_writerec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
				if(mode & 1) {
					j = dil_display_text(7);
				}
			} 

			m_free(curfile);
			break;
		case MDIL_OP_UPD_VAL:
			//check access
			if(_check_access(&temp_fs, FILE_WRITE) != APDU_SUCCESS) {  
				mdil_state = MDIL_STATE_UPDATE_VAL_NEED_PIN;
				return dil_get_pin(1);		
			}
			curfile = file_get_current_header(&temp_fs);
			_readrec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
			memset(STK_buffer + 20, 0xFF, curfile->rec_size - 20);
			if(vas_cistr[4] > (curfile->size / curfile->rec_size)) {
				j = dil_display_text(6);  		//invalid record ID
			} else {
				i = 5;		//start of name data
				STK_buffer[0] = 0x04;		//8bit 
				j = 2;
				for(i=5, j=2; j<20 && i<len; i++, j++) { 		//copy name parameter
					if(vas_cistr[i] == 0xFF && vas_cistr[i+1] == 0xFF) {	//separator found
						STK_buffer[1] = (j-2);
						j = 20;
						i += 2;
					}
				}
				j = 20;
				STK_buffer[j++] = 0x04;
				STK_buffer[j++] = (len - i);
				for(i;i<len;i++,j++) {				   		//copy value parameter
					STK_buffer[j] = vas_cistr[i];	
				}
				_writerec(&temp_fs, vas_cistr[4] - 1, STK_buffer, curfile->rec_size);
				if(mode & 1) {
					j = dil_display_text(7);
				}
			}
			m_free(curfile);
			break;
	}
	exit_plugin:
	mdil_state = MDIL_STATE_IDLE;
	VAS_exit_plugin();
	return j;					//0 => next instruction (no response), 
}
#endif