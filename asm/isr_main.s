    .extern _G_ISR_TABLE
    .extern _G_ISR_HEAP_LAST_IDX

    .section .bss
    .align 2
    .type HEAP_CPY %object
HEAP_CPY:
    .space 112
    .size HEAP_CPY, .-HEAP_CPY
    

    .arm
    .section .ewram, "ax", %progbits
    .align 2
    .global ISR_MAIN_HANDLER
    .type ISR_MAIN_HANDLER %function
ISR_MAIN_HANDLER:
    MOV r0, #0x04000000
    LDR r12, [r0, #0x200]!
    AND r2, r12, r12, LSR #16  // r2 = REG_IE & REG_IF
    
    // ACK
    STRH r2, [r0, #2]
    LDR r3, [r0, #-0x208]
    ORR r3, r3, r2  // r3 = REG_IE & REG_IF | REG_IFBIOS
    STR r3, [r0, #-0x208]

    // SEARCH P1: Copy heap
    LDR r3, =_G_ISR_HEAP_LAST_IDX  // r3 = &heap_size
    LDR r3, [r3]  // r3 = *(&heap_size)
    CMP r3, #0
    BXEQ lr

    PUSH { r0, r4-r10}  // r0: 0x04000200 r4-r10: typical store for clobber guard
    PUSH { r2 }  // r2: REG_IE&REG_IF (separate so it's at top)
    LDR r1, =_G_ISR_TABLE
    LDR r0, =HEAP_CPY
    MOV r2, #3
.L_ISR_MAIN_HANDLER_heap_cpy:
        LDMIA r1!, { r3-r10 }
        STMIA r0!, { r3-r10 }
        SUBS r2, #3
        BNE .L_ISR_MAIN_HANDLER_heap_cpy
    LDMIA r1!, { r3-r6 }
    STMIA r0!, { r3-r6 }
    
    // SEARCH P2: clean out heap til match found
    LDR r0, =HEAP_CPY  // r0 = heap
    LDR r1, =_G_ISR_HEAP_LAST_IDX  // r1 = &(heap size)
    LDR r1, [r1]  // r1 = *(&heap_size)
    POP { r2 }  // r2 = (REG_IE & REG_IF) 
.L_ISR_MAIN_HANDLER_heap_search:
        LDRH r3, [r0]  // r3 = heap[0].flag
        TST r3, r2     // ANDS r3, r3, r2 except result is discarded
        LDRNE r3, [r0, #4]  // r3 = isr callback
        BNE .L_ISR_MAIN_HANDLER_post_heap_search
        SUBS r1, r1, #1  // --heap_size
        MOVEQ r3, #0  // if heap size is now 0, r3 = 0 ; isr callback = NULL
        BEQ .L_ISR_MAIN_HANDLER_post_heap_search  // if heap size is now zero, then search exhausted
        ADD r3, r0, r1, LSL #3  // r3 = heap_addr + heap_len*sizeof(heap_entry)
        // SEARCH P2.A: heap removal op
        LDR r4, [r3], #4
        STR r4, [r0]
        LDR r4, [r3]
        STR r4, [r0, #4]
        // former last index of heap is now at head. Bubble data down as needed
        MOV r3, #0  // r3 = bubble node idx
.L_ISR_MAIN_HANDLER_heap_bubble_down:
            MOV r4, r3, LSL #1  // r4 = idx*2
            ADD r4, r4, #1  // r4 = idx*2+1 = idx left child
            CMP r4, r1
            BGE .L_ISR_MAIN_HANDLER_heap_search
            ADD r5, r4, #1  // r5 = idx*2+2 = idx right child
            CMP r5, r1
            BGE .L_ISR_MAIN_HANDLER_heap_bubble_do_bubble
            LDR r6, [r0, r4]
            LSR r7, r7, #16
            LDR r7, [r0, r5]
            LSR r7, r7, #16
            CMP r7, r6
            BLT .L_ISR_MAIN_HANDLER_heap_bubble_do_bubble
            MOV r4, r5  // child idx is now right child idx
.L_ISR_MAIN_HANDLER_heap_bubble_do_bubble:
            LDR r5, [r0, r3]  // r5 = parent.priority
            LSR r5, r5, #16
            LDR r6, [r0, r4]  // r6 = child.priority
            LSR r6, r6, #16
            // if parent.priority >= child.priority, bubble done. continue
            CMP r5, r6
            BGE .L_ISR_MAIN_HANDLER_heap_search
            ADD r3, r0, r3, LSL #3  // r3 = &heap_addr[parent]
            ADD r5, r0, r4, LSL #3  // r5 = &heap_addr[child]
            LDMIA r3, { r9, r10 }   // load heap_addr[parent] in r9-r10
            LDMIA r5, { r7, r8 }    // load heap_addr[child] in r7-r8 
            STMIA r3, { r7, r8 }    // store (r7-r8 := heap_addr[child]) at &heap_addr[parent]
            STMIA r5, { r9, r10 }   // store (r9-r10 := heap_addr[parent]) at &heap_addr[child]
            MOV r3, r4  // r3 = child idx = new bubble idx
            B .L_ISR_MAIN_HANDLER_heap_bubble_down
.L_ISR_MAIN_HANDLER_post_heap_search:
    POP { r0, r4-r10 }
    // POP r0 = &REG_IE (0x04000200) r4-r10: declobber
    CMP r3, #0
    BXEQ lr  // if (r3 = isr_cb) == NULL return
    // Prep to switch out of irq mode back to sys mode:
    // CLEAR IME and curr IRQ in IE
    // Relevant regs: r0:&REG_IE r2:REG_IE&REG_IF r3:callback r12: REG_IF<<16|REG_IE
    MOV r1, r3  // r1 = cb
    LDR r3, [r0, #8]  // r3 = REG_IME
    STRB r0, [r0, #8]
    BIC r2, r12, r2
    STRH r2, [r0] 
    MRS r2, spsr  // r2 = spsr
    // Save SPSR, IME, IEIF, lr in stack
    STMFD sp!, { r2-r3, r12, lr }  // spsr, IME, (IF<<16|IE), irq lr
    
    MRS r3, cpsr
    BIC r3, r3, #0xDF
    ORR r3, r3, #0x1F
    MSR cpsr, r3
    // CALL CB
    STMFD sp!, { r0, lr }  // &REG_IE, sys lr
    MOV lr, pc
    BX r1
    LDMFD sp!, { r0, lr }  // &REG_IE, sys lr
    
    // UNDO CPU STATE CHANGES (RESET CPU MODE TO IRQ MODE)
    STRB r0, [r0, #8]  // clear IME again J.I.C.
    MRS r3, cpsr
    BIC r3, r3, #0xDF
    ORR r3, r3, #0x92
    MSR cpsr, r3

    // RESTORE SPSR, IME, IE, irq lr
    LDMFD sp!, { r2-r3, r12, lr }
    MSR spsr, r2
    STRH r12, [r0]
    STR r3, [r0, #8]
    // RETURN
    BX lr
    .size ISR_MAIN_HANDLER, .-ISR_MAIN_HANDLER
