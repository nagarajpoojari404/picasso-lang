#include "context.h"
#include "task.h"
#include "scheduler.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern void platform_ctx_entry(void);

void task_completed_callback(platform_ctx_t* current, platform_ctx_t* parent) {
    /* Use the parent context that was stored on the stack during task creation */
    extern __thread task_t* current_task;
    /* The parent pointer was stored on the stack and should point to the scheduler context */
    if (parent) {
        platform_ctx_switch(current, parent);
    } else {
        printf("[ERROR] No parent context to return to!\n");
        exit(1);
    }
    
    while(1) {}
}

/**
 * @brief Initialize a platform context
 * @param ctx Pointer to the platform context to initialize
 * @return 0 on success, non-zero on failure
 */
int platform_ctx_init(platform_ctx_t *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(platform_ctx_t));
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
int platform_ctx_make(platform_ctx_t *ctx, void (*entry)(uintptr_t, uintptr_t),  uintptr_t a, uintptr_t b, void *stack, size_t stack_size, platform_ctx_t* back_link) {
    if (!ctx || !stack) return -1;
    
    uintptr_t top = (uintptr_t)stack + stack_size;
    top &= ~(uintptr_t)0x0FL;
    top -= 128;
    
    memset(&ctx->reg, 0, sizeof(registers_t));
    ctx->stack = stack;
    ctx->stack_size = stack_size;
    ctx->back_link = back_link;
    
    // Store BOTH current context and back_link on stack
    // Stack layout from top: [current_ctx] [back_link] [rest of stack]
    top -= 16;
    *(platform_ctx_t**)top = ctx;        // Current context pointer
    top -= 16;
    *(platform_ctx_t**)top = back_link;  // Parent context pointer
    
    ctx->reg.regs[0] = (uintptr_t)entry;           // x19: entry function
    ctx->reg.regs[1] = a;                          // x20: arg a
    ctx->reg.regs[2] = b;                          // x21: arg b
    ctx->reg.regs[10] = 0;                         // x29: frame pointer
    ctx->reg.regs[11] = (uintptr_t)platform_ctx_entry; // x30: link register
    ctx->reg.sp = (uintptr_t)top;
    
    return 0;
}

/**
 * @brief Destroy a platform context and free associated resources
 * @param ctx Pointer to the platform context to destroy
 */
void platform_ctx_destroy(platform_ctx_t* ctx) {
    if (ctx) memset(ctx, 0, sizeof(platform_ctx_t));
}

/**
 * @brief Get the base address of the context's stack
 * @param ctx Pointer to the platform context
 * @return Base address of the stack
 */
uintptr_t platform_ctx_get_stack_base(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->stack;
}

/**
 * @brief Get the current stack pointer of the context
 * @param ctx Pointer to the platform context
 * @return Current stack pointer value
 */
uintptr_t platform_ctx_get_stack_pointer(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->reg.sp;
}

/**
 * @brief Get the program counter (instruction pointer) of the context
 * @param ctx Pointer to the platform context
 * @return Program counter value
 */
uintptr_t platform_ctx_get_pc(platform_ctx_t* ctx) {
    return (uintptr_t)ctx->reg.regs[11];
}

/**
 * @brief Get the size of the context's stack
 * @param ctx Pointer to the platform context
 * @return Stack size in bytes
 */
int64_t platform_ctx_get_stack_size(platform_ctx_t* ctx) {
    return (int64_t)ctx->stack_size;
}

/**
 * @brief Get the value of a specific register from the context
 * @param ctx Pointer to the platform context
 * @param i Register index
 * @return Value of the specified register
 */
uint64_t platform_ctx_get_reg(platform_ctx_t *ctx, int i) {
    if (i >= 19 && i <= 30) return ctx->reg.regs[i - 19];
    if (i == 31) return (uint64_t)ctx->reg.sp;
    return 0; 
}
