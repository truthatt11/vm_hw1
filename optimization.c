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

static inline void shack_init(CPUState *env)
{
    env->shack = (uint64_t*) malloc(SHACK_SIZE * sizeof(uint64_t));
    env->shadow_ret_addr = (uint64_t*) malloc(SHACK_SIZE * sizeof(uint64_t));
    env->shadow_hash_list;
    env->shadow_ret_addr;
    env->shack_top = env->shack;
    env->shack_end = env->shack[SHACK_SIZE-1];
}

/*
 * shack_set_shadow()
 *  Insert a guest eip to host eip pair if it is not yet created.
 */
 void shack_set_shadow(CPUState *env, target_ulong guest_eip, unsigned long *host_eip)
{
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
    memset(ibtcTable, 0, sizeof(ibtcTable));
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
