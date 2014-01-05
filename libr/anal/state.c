/* radare - Apache 2.0 - Copyright 2013 - Adam Pridgen <dso@rice.edu || adam.pridgen@thecoverofnight.com> */

#include <r_anal.h>
#include <r_util.h>
#include <r_list.h>
#include <r_io.h>
#include "../config.h"

#define IFDBG if(0)

R_API RAnalInfos * r_anal_state_new (ut64 start, ut8* buffer, ut64 len) {
	RAnalInfos *state = R_NEW0 (RAnalInfos);
	state->start = start;
	state->end = start + len;
	state->buffer = buffer;
	state->len = len;
	state->current_op = NULL;
	state->current_bb = NULL;
	state->current_fcn = NULL;
	state->ht = r_hashtable64_new();
	state->ht_sz = 512;
	state->bbs = r_list_new();
	state->max_depth = 50;
	state->current_depth = 0;
	//r_hashtable64_rehash(state->ht, state->ht_sz);
	return state;
}

R_API void r_anal_state_set_depth(RAnalInfos *state, ut32 depth) {
	state->current_depth = depth;
}

R_API void r_anal_state_insert_bb (RAnalInfos* state, RAnalBlock *bb) {
	/*if (r_anal_state_need_rehash (state, bb)) {
		// XXX - do i ever need to rehash the hashtable?
		//state->ht_sz <<= 1;
		//r_hashtable64_rehash(state->ht, state->ht_sz);
	}*/

	if (r_anal_state_search_bb (state, bb->addr) == NULL &&
		state->current_fcn) {
		RHashTable64Entry *hte ;
		RAnalBlock *tmp_bb;
		IFDBG eprintf ("Inserting bb 0x%04x into hash table\n", bb->addr);
		r_list_append(state->current_fcn->bbs, bb);
        state->bytes_consumed += state->current_bb->op_sz;
		IFDBG eprintf ("[--] Consumed 0x%02x bytes, for a total of 0x%02x\n", state->current_bb->op_sz, state->bytes_consumed);
		if (r_hashtable64_insert(state->ht, bb->addr, bb)) {
			IFDBG eprintf ("Inserted bb 0x%04x successfully inserted into hashtable\n", bb->addr);
		}
		IFDBG {
			tmp_bb = r_hashtable64_lookup(state->ht, bb->addr);
			IFDBG eprintf ("Inserted bb 0x%04x matches one in hash table: %02x.\n", bb->addr, bb->addr == tmp_bb->addr);
		}
	}
}
/*
R_API int r_anal_state_need_rehash (RAnalInfos* state, RAnalBlock *bb) {
	/*
	 *   Return 0 if no rehash is needed, otherwise return 1
	 * /
	
	RAnalBlock *tmp_bb = r_hashtable64_lookup(state->ht, bb->addr);

	if (tmp_bb == NULL || tmp_bb->addr != bb->addr) {
		return 1; 
	}
	return 0;
}
*/
R_API RAnalBlock * r_anal_state_search_bb (RAnalInfos* state, ut64 addr) {
	/*
	 *   Return 0 if no rehash is needed, otherwise return 1
	 */
	RAnalBlock *tmp_bb = r_hashtable64_lookup(state->ht, addr);
	return tmp_bb;
}

R_API void r_anal_state_free (RAnalInfos * state) {
	r_list_free(state->bbs);
	r_hashtable64_free(state->ht);
	free(state);
}

R_API ut64 r_anal_state_get_len (RAnalInfos *state, ut64 addr) {
	ut64 result = 0;
	if (r_anal_state_addr_is_valid (state, addr)) {
		result = state->len - (addr - state->start);
	}
	return result;
}

R_API const ut8 * r_anal_state_get_buf_by_addr (RAnalInfos *state, ut64 addr) {
	if (r_anal_state_addr_is_valid (state, addr)) {
		ut64 offset = addr - state->start;
		return state->buffer+offset;
	}
	return NULL;
}

R_API int r_anal_state_addr_is_valid (RAnalInfos *state, ut64 addr) {
	int result = R_FALSE;
	if (addr < state->end  && addr >= state->start) {
		result = R_TRUE;
	}
	return result;
}

R_API void r_anal_state_merge_bb_list (RAnalInfos *state, RList* bbs) {
	RListIter *iter;
	RAnalBlock *bb;
	r_list_foreach (bbs, iter, bb) {
		IFDBG eprintf ("Inserting bb fron 0x%04x\n", bb->addr);
		r_anal_state_insert_bb (state, bb);
	}
}