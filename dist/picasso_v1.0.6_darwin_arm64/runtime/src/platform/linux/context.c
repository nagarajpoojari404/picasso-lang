#include "platform.h"
#include "platform/context.h"
#include <ucontext.h>
#include <stdlib.h>

/**
 * @brief Initialize a platform context
 * @param ctx Pointer to the platform context to initialize
 * @return 0 on success, non-zero on failure
 */
int  platform_ctx_init(platform_ctx_t *ctx) {
    ctx->uc = (ucontext_t*)malloc(sizeof(ucontext_t));
    getcontext(ctx->uc);

    return 0;
}

/**
 * @brief Create a new platform context with the given entry point and stack
 * @param ctx Pointer to the platform context to create
 * @param entry Entry point function to execute in the new context
 * @param a First argument to pass to the entry function
 * @param b Second argument to pass to the entry function
 * @param stack Pointer to the stack memory
 * @param stack_size Size of the stack in bytes
 * @param back_link Pointer to the context to return to when entry function returns
 * @return 0 on success, non-zero on failure
 */
int  platform_ctx_make(platform_ctx_t *ctx, void (*entry)(uintptr_t, uintptr_t), uintptr_t a, uintptr_t b, void *stack, size_t stack_size, platform_ctx_t* back_link) {
    ctx->uc->uc_stack.ss_sp = stack;
    ctx->uc->uc_stack.ss_size = stack_size;
    ctx->uc->uc_link = back_link->uc;
    makecontext(ctx->uc, (void(*)(void))entry, 2, a, b);

    return 0;
}

/**
 * @brief Switch execution from one context to another
 * @param from Pointer to the current context (will be saved)
 * @param to Pointer to the context to switch to (will be restored)
 */
void platform_ctx_switch(platform_ctx_t *from, platform_ctx_t *to) {
    swapcontext(from->uc, to->uc);
}

/**
 * @brief Destroy a platform context and free associated resources
 * @param ctx Pointer to the platform context to destroy
 */
void plarform_ctx_destroy(platform_ctx_t* ctx) {
    free(ctx->uc);
}

/**
 * @brief Get the base address of the context's stack
 * @param ctx Pointer to the platform context
 * @return Base address of the stack
 */
uintptr_t platform_ctx_get_stack_base(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->uc->uc_stack.ss_sp;
}

/**
 * @brief Get the current stack pointer of the context
 * @param ctx Pointer to the platform context
 * @return Current stack pointer value
 */
uintptr_t platform_ctx_get_stack_pointer(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->uc->uc_mcontext.sp;
}

/**
 * @brief Get the program counter (instruction pointer) of the context
 * @param ctx Pointer to the platform context
 * @return Program counter value
 */
uintptr_t platform_ctx_get_pc(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->uc->uc_mcontext.pc;
}

/**
 * @brief Get the size of the context's stack
 * @param ctx Pointer to the platform context
 * @return Stack size in bytes
 */
int64_t platform_ctx_get_stack_size(
    platform_ctx_t* ctx) {
    return (uintptr_t)ctx->uc->uc_stack.ss_size;
}

/**
 * @brief Get the value of a specific register from the context
 * @param ctx Pointer to the platform context
 * @param i Register index
 * @return Value of the specified register
 */
uint64_t platform_ctx_get_reg(platform_ctx_t *ctx, int i) {
    return ctx->uc->uc_mcontext.regs[i];
}
