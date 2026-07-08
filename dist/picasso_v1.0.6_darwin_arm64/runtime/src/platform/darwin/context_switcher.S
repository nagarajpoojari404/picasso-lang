.section __TEXT,__text,regular,pure_instructions
.align 4
.global _platform_ctx_switch
.global _platform_ctx_entry
.global _task_completed_callback

_platform_ctx_switch:
    ; x0: *from_ctx, x1: *to_ctx
    
    ; Save all callee-saved registers to 'from' context
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x16, sp
    str x16, [x0, #96]
    
    ; Restore all callee-saved registers from 'to' context
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldp x29, x30, [x1, #80]
    ldr x16, [x1, #96]
    mov sp, x16
    
    ; Jump to restored context
    br x30

_platform_ctx_entry:
    ; When we arrive here via context switch:
    ; x19: entry_fn pointer
    ; x20: arg_a
    ; x21: arg_b
    ; SP points to: [back_link] [current_ctx] [rest of stack...]
    
    ; Pop the context pointers from stack and save them
    ldr x22, [sp], #16          ; x22 = back_link (parent context)
    ldr x23, [sp], #16          ; x23 = current_ctx
    
    ; Align stack to 16 bytes
    mov x16, sp
    and x16, x16, #0xFFFFFFFFFFFFFFF0
    mov sp, x16
    
    ; Set up a proper C stack frame
    ; We need to save x22, x23 for later use
    stp x22, x23, [sp, #-16]!   ; Save context pointers on stack
    stp x29, x30, [sp, #-16]!   ; Save frame pointer and link register
    mov x29, sp                  ; Set up frame pointer
    
    ; Prepare arguments for entry function
    mov x0, x20                  ; First arg: a
    mov x1, x21                  ; Second arg: b
    
    ; Call the entry function (task_trampoline)
    blr x19
    
    ; Entry function returned!
    ; Now call the completion callback
    ; Restore context pointers from stack
    ldp x29, x30, [sp], #16     ; Restore FP, LR (though we don't need them)
    ldp x0, x1, [sp], #16       ; x0 = back_link, x1 = current_ctx
    
    ; Swap them to correct order for callback
    mov x16, x0
    mov x0, x1                   ; x0 = current_ctx (first arg)
    mov x1, x16                  ; x1 = back_link (second arg)
    
    ; Call completion callback
    bl _task_completed_callback
    
    ; Should never return, but just in case
    b .
