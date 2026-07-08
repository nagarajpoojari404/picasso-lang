#ifndef PLATFORM_CONTEXT_H
#define PLATFORM_CONTEXT_H

#include "platform.h"
#include <stdint.h>
#include <stddef.h>
#if defined(__linux__)
    #include <ucontext.h>
#endif

#if defined(__APPLE__)
    typedef struct registers_t {
        /* x19-28 */
        uint64_t regs[12];
        uintptr_t sp;
    } registers_t;
#endif


typedef struct platform_ctx {
#if defined(__linux__)
    ucontext_t *uc;    
#elif defined(__APPLE__)
    registers_t reg;
    void* stack;
    size_t stack_size;

    /* back link stores scheduler context to switch back when task finish*/
    struct platform_ctx* back_link;
#endif
} platform_ctx_t;

/**
 * @brief Initialize a platform context
 * @param ctx Pointer to the platform context to initialize
 * @return 0 on success, non-zero on failure
 */
int  platform_ctx_init(platform_ctx_t *ctx);

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
int  platform_ctx_make(platform_ctx_t *ctx, void (*entry)(uintptr_t, uintptr_t), uintptr_t a, uintptr_t b, void *stack, size_t stack_size, platform_ctx_t* back_link);

/**
 * @brief Switch execution from one context to another
 * @param from Pointer to the current context (will be saved)
 * @param to Pointer to the context to switch to (will be restored)
 */
void platform_ctx_switch(platform_ctx_t *from, platform_ctx_t *to);

/**
 * @brief Destroy a platform context and free associated resources
 * @param ctx Pointer to the platform context to destroy
 */
void plarform_ctx_destroy(platform_ctx_t* ctx);

/**
 * @brief Get the base address of the context's stack
 * @param ctx Pointer to the platform context
 * @return Base address of the stack
 */
uintptr_t platform_ctx_get_stack_base(platform_ctx_t* ctx);

/**
 * @brief Get the current stack pointer of the context
 * @param ctx Pointer to the platform context
 * @return Current stack pointer value
 */
uintptr_t platform_ctx_get_stack_pointer(platform_ctx_t* ctx);

/**
 * @brief Get the program counter (instruction pointer) of the context
 * @param ctx Pointer to the platform context
 * @return Program counter value
 */
uintptr_t platform_ctx_get_pc(platform_ctx_t* ctx);

/**
 * @brief Get the size of the context's stack
 * @param ctx Pointer to the platform context
 * @return Stack size in bytes
 */
int64_t platform_ctx_get_stack_size(platform_ctx_t* ctx);

/**
 * @brief Get the value of a specific register from the context
 * @param ctx Pointer to the platform context
 * @param i Register index
 * @return Value of the specified register
 */
uint64_t platform_ctx_get_reg(platform_ctx_t *ctx, int i) ;
#endif
