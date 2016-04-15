/*
 *  (C) 2010 by Computer System Laboratory, IIS, Academia Sinica, Taiwan.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "exec-all.h"
#include "tcg-op.h"
#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"
#include "optimization.h"

extern uint8_t *optimization_ret_addr;

/*
 * Shadow Stack
 */
list_t *shadow_hash_list;

static inline unsigned long** hash_lookup(CPUState* env, target_ulong guest_eip) {
    int index = guest_eip & (MAX_CALL_SLOT-1);
    struct shadow_pair* ptr = ((struct shadow_pair**)env->shadow_hash_list)[index];

    while(ptr != NULL && ptr->guest_eip != guest_eip) ptr = ptr->next;

    if(ptr) return ptr->shadow_slot;
    return NULL;
}

static inline void hash_insert(CPUState *env, target_ulong guest_eip, unsigned long* host_eip) {
    int index = guest_eip & (MAX_CALL_SLOT-1);
    struct shadow_pair* temp = (struct shadow_pair**) malloc(sizeof(struct shadow_pair));
    temp->guest_eip = guest_eip;
    temp->shadow_slot = 0;
    temp->next = ((struct shadow_pair**)env->shadow_hash_list)[index];
    ((struct shadow_pair**)env->shadow_hash_list)[index] = temp;
}

static inline void shack_init(CPUState *env)
{
    env->shack = (uint32_t*) malloc(SHACK_SIZE * sizeof(uint64_t));
    env->shadow_ret_addr = (target_ulong*) malloc(SHACK_SIZE * sizeof(uint64_t));
    env->shadow_hash_list = (struct shadow_pair*) malloc(MAX_CALL_SLOT * sizeof(struct shadow_pair));
//    env->shadow_ret_addr;
    env->shack_top = env->shack + SHACK_SIZE -1;
    env->shadow_ret_addr_top = env->shadow_ret_addr + SHACK_SIZE -1;
}

/*
 * shack_set_shadow()
 *  Insert a guest eip to host eip pair if it is not yet created.
 */
void shack_set_shadow(CPUState *env, target_ulong guest_eip, unsigned long *host_eip)
{
    unsigned long **host = hash_lookup(env, guest_eip);
    if(host == NULL)
        hash_insert(env, guest_eip, host_eip);
    else
        *host = host_eip;
}

/*
 * helper_shack_flush()
 *  Reset shadow stack.
 */
void helper_shack_flush(CPUState *env)
{
}

/*
 * push_shack()
 *  Push next guest eip into shadow stack.
 */
void push_shack(CPUState *env, TCGv_ptr cpu_env, target_ulong next_eip)
{
}

/*
 * pop_shack()
 *  Pop next host eip from shadow stack.
 */
void pop_shack(TCGv_ptr cpu_env, TCGv next_eip)
{
    TCGv_ptr shack_top_ptr, shadow_ret_top_ptr;
    TCGv guest_eip, host_eip;
    int elseLabel;

    shack_top_ptr = tcg_temp_new();
    shadow_ret_top_ptr = tcg_temp_new();
    guest_eip = tcg_temp_new();
    host_eip = tcg_temp_new();
    elseLabel = gen_new_label();

    /*
        if(*shack_top == next_eip) {
            host_eip = shadow_ret_addr(next_eip);
            if(host_eip != NULL) {
                shack_top++;
                shadow_ret_addr_top++;

                tcg_gen_...
            }
        }
    */

    tcg_gen_ld_ptr(shack_top_ptr, cpu_env, offsetof(CPUState, shack_top));
    tcg_gen_ld_ptr(shadow_ret_top_ptr, cpu_env, offsetof(CPUState, shadow_ret_addr_top));
    tcg_gen_ld_tl(guest_eip, shack_top_ptr, 0);
    tcg_gen_brcond_tl(TCG_COND_NE, guest_eip, next_eip, elseLabel);

    tcg_gen_ld_tl(host_eip, shadow_ret_top_ptr, 0);
    tcg_gen_brcond_tl(TCG_COND_EQ, host_eip, tcg_const_tl(0), elseLabel);
    tcg_gen_add_tl(shadow_ret_top_ptr, shadow_ret_top_ptr, tcg_const_tl(sizeof(uint32_t)));
    tcg_gen_add_tl(shack_top_ptr, shack_top_ptr, tcg_const_tl(sizeof(target_ulong)));


    *gen_opc_ptr++ = INDEX_op_jmp;
    *gen_opparam_ptr++ = GET_TCGV_I32(host_eip);
    gen_set_label(elseLabel);

    tcg_temp_free_ptr(shack_top_ptr);
    tcg_temp_free_ptr(shadow_ret_top_ptr);
    tcg_temp_free(guest_eip);
    tcg_temp_free(host_eip);
}

/*
 * Indirect Branch Target Cache
 */
__thread int update_ibtc;
static target_ulong temp_guest_eip;
struct ibtc_table *ibtcTable;

/*
 * helper_lookup_ibtc()
 *  Look up IBTC. Return next host eip if cache hit or
 *  back-to-dispatcher stub address if cache miss.
 */
void *helper_lookup_ibtc(target_ulong guest_eip)
{
    int index = guest_eip & IBTC_CACHE_MASK;
    temp_guest_eip = guest_eip;

    if(ibtcTable->htable[index].guest_eip == guest_eip) {
        return ibtcTable->htable[index].tb;
    }else {
        update_ibtc = 1;
    }

    return optimization_ret_addr;
}

/*
 * update_ibtc_entry()
 *  Populate eip and tb pair in IBTC entry.
 */
void update_ibtc_entry(TranslationBlock *tb)
{
    int index = temp_guest_eip & IBTC_CACHE_MASK;

    update_ibtc = 0;
    ibtcTable->htable[index].guest_eip = temp_guest_eip;
    ibtcTable->htable[index].tb = tb;
}

/*
 * ibtc_init()
 *  Create and initialize indirect branch target cache.
 */
static inline void ibtc_init(CPUState *env)
{
    ibtcTable = malloc(sizeof(struct ibtc_table));
    memset(ibtcTable, 0, sizeof(struct ibtc_table));
}

/*
 * init_optimizations()
 *  Initialize optimization subsystem.
 */
int init_optimizations(CPUState *env)
{
    shack_init(env);
    ibtc_init(env);

    return 0;
}

/*
 * vim: ts=8 sts=4 sw=4 expandtab
 */
