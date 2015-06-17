@**: EMULib Emulation Library *********************************
@**:                                                         **
@**:                        LibARM.asm                       **
@**:                                                         **
@**: This file contains optimized ARM assembler functions    **
@**: used to copy and process images on ARM-based platforms  **
@**: such as Symbian/S60, Symbian/UIQ, and Maemo.            **
@**:                                                         **
@**: Copyright (C) Marat Fayzullin 2005-2008                 **
@**:     You are not allowed to distribute this software     **
@**:     commercially. Please, notify me, if you make any    **
@**:     changes to this file.                               **
@*************************************************************:
	.text

	.global	TELEVIZE0
	.global	TELEVIZE1
	.global	C256T120
	.global	C256T160
	.global	C256T176
	.global	C256T208
	.global	C256T240
	.global	C256T256
	.global	C256T320
	.global	C256T352
	.global	C256T416
	.global	C256T512
	.global	C256T768


	@** M_LOADCONSTS *************************************************
	@** Loads two constants used for pixel merging.                 **
	@** For 16BPP:       r12=0x7BEF7BEF, r14=0x42084208             **
	@** For 24BPP/32BPP: r12=0x007F7F7F, r14=0x00808080             **
	@*****************************************************************
	.macro M_LOADCONSTS
	.ifdef BPP16
	mov r12,#0x00EF
	orr r12,r12,#0x7B00
	orr r12,r12,r12,lsl #16
	mov r14,#0x0008
	orr r14,r14,#0x4200
	orr r14,r14,r14,lsl #16
	.else
	mov r12,#0x00007F
	orr r12,r12,#0x007F00
	orr r12,r12,#0x7F0000
	mov r14,#0x000080
	orr r14,r14,#0x008000
	orr r14,r14,#0x800000
	.endif
	.endm

	@** M_MERGE16 ****************************************************
	@** Merge two 16bpp pixels into one. Trashes \src.              **
	@*****************************************************************
	.macro M_MERGE16 dst,src
	and \dst,\src,r12
	add \dst,\dst,\dst,lsr #16
	and \src,r14,\src,lsr #1
	add \src,\src,\src,lsr #16
	and \dst,r12,\dst,lsl #15
	add \dst,\src,\dst,lsr #16
	.endm

	@** M_MERGE32 ****************************************************
	@** Merge two 24/32bpp pixels into one. Trashes \src1 and \tmp. **
	@*****************************************************************
	.macro M_MERGE32 dst,src1,src2,tmp
	and \tmp,\src1,r12
	and \dst,\src2,r12
	add \tmp,\tmp,\dst
	and \tmp,r12,\tmp,lsr #1
	and \src1,\src1,r14
	and \dst,\src2,r14
	add \dst,\dst,\src1
	add \dst,\tmp,\dst,lsr #1
	.endm

C256T120:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L120:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	M_MERGE16 r11,r3
	mov r3,r11,lsl #16
	mov r3,r3,lsr #16
	M_MERGE16 r11,r4
	orr r3,r3,r11,lsl #16
	M_MERGE16 r11,r5
	mov r4,r11,lsl #16
	mov r4,r4,lsr #16
	M_MERGE16 r11,r6
	orr r4,r4,r11,lsl #16
	M_MERGE16 r11,r7
	mov r5,r11,lsl #16
	mov r5,r5,lsr #16
	M_MERGE16 r11,r8
	orr r5,r5,r11,lsl #16
	M_MERGE16 r11,r9
	mov r6,r11,lsl #16
	mov r6,r6,lsr #16
	M_MERGE16 r11,r10
	orr r6,r6,r11,lsl #16
	stmia r0!,{r3,r4,r5,r6}
	subs r2,r2,#16
	.else
	ldmia r1!,{r4,r5}
	M_MERGE32 r3,r4,r5,r11
	ldmia r1!,{r5,r6,r7,r8,r9,r10}
	M_MERGE32 r4,r5,r6,r11
	M_MERGE32 r5,r7,r8,r11
	M_MERGE32 r6,r9,r10,r11
	stmia r0!,{r3,r4,r5,r6}
	subs r2,r2,#8
	.endif
	bhi L120
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T160:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L160:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[0]  = MERGE(SRC[0],SRC[1])
	strh r4,[r0],#2		@ DST[1]  = SRC[2]
	mov r3,r4,lsr #16
	orr r3,r3,r5,lsl #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[2]  = MERGE(SRC[3],SRC[4])
	mov r11,r5,lsr #16
	strh r11,[r0],#2		@ DST[3]  = SRC[5]
	M_MERGE16 r11,r6
	strh r11,[r0],#2		@ DST[4]  = MERGE(SRC[6],SRC[7])
	M_MERGE16 r11,r7
	strh r11,[r0],#2		@ DST[5]  = MERGE(SRC[8],SRC[9])
	strh r8,[r0],#2		@ DST[6]  = SRC[10]
	mov r3,r8,lsr #16
	orr r3,r3,r9,lsl #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[7]  = MERGE(SRC[11],SRC[12])
	mov r11,r9,lsr #16
	strh r11,[r0],#2		@ DST[8]  = SRC[13]
	M_MERGE16 r11,r10
	strh r11,[r0],#2		@ DST[9]  = MERGE(SRC[14],SRC[15])
	.else
	ldmia r1!,{r3,r4,r6,r7,r8,r10}
	M_MERGE32 r5,r3,r4,r11
	M_MERGE32 r9,r7,r8,r11
	stmia r0!,{r5,r6,r9,r10}	@ DST[0,1,2,3] = MERGE(0,1),SRC[2],MERGE(3,4),SRC[5]
	ldmia r1!,{r3,r4,r6,r7,r9}
	M_MERGE32 r5,r3,r4,r11
	M_MERGE32 r8,r6,r7,r11
	stmia r0!,{r5,r8,r9}		@ DST[4,5,6] = MERGE(6,7),MERGE(8,9),SRC[10]
	ldmia r1!,{r3,r4,r6,r7,r8}
	M_MERGE32 r5,r3,r4,r11
	M_MERGE32 r9,r7,r8,r11
	stmia r0!,{r5,r6,r9}		@ DST[7,8,9] = MERGE(11,12),SRC[13],MERGE(14,15)
	.endif
	subs r2,r2,#16
	bhi L160
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T176:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L176:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[0]  = MERGE(SRC[0],SRC[1])
	mov r11,r4,lsr #16
	strh r4,[r0],#2		@ DST[1]  = SRC[2]
	strh r11,[r0],#2		@ DST[2]  = SRC[3]
	M_MERGE16 r11,r5
	strh r11,[r0],#2		@ DST[3]  = MERGE(SRC[4],SRC[5])
	mov r11,r6,lsr #16
	strh r6,[r0],#2		@ DST[4]  = SRC[6]
	strh r11,[r0],#2		@ DST[5]  = SRC[7]
	M_MERGE16 r11,r7
	strh r11,[r0],#2		@ DST[6]  = MERGE(SRC[8],SRC[9])
	strh r8,[r0],#2		@ DST[7]  = SRC[10]
	mov r3,r9,lsl #16
	orr r3,r8,lsr #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[8]  = MERGE(SRC[11],SRC[12])
	mov r11,r9,lsr #16
	strh r11,[r0],#2		@ DST[9]  = SRC[13]
	M_MERGE16 r11,r10
	strh r11,[r0],#2		@ DST[10] = MERGE(SRC[14],SRC[15])
	.else
	ldmia r1!,{r3,r4,r6,r7,r8,r9}
	M_MERGE32 r5,r3,r4,r11
	M_MERGE32 r10,r8,r9,r11
	stmia r0!,{r5,r6,r7,r10}	@ DST[0,1,2,3] = MERGE(0,1),SRC[2],SRC[3],MERGE(4,5)
	ldmia r1!,{r3,r4,r5,r6,r8,r9,r10}
	M_MERGE32 r7,r5,r6,r11
	stmia r0!,{r3,r4,r7,r8}	@ DST[4,5,6,7] = SRC[6],SRC[7],MERGE(8,9),SRC[10]
	M_MERGE32 r3,r9,r10,r11
	ldmia r1!,{r4,r5,r6}
	M_MERGE32 r7,r5,r6,r11
	stmia r0!,{r3,r4,r7}		@ DST[8,9,10] = MERGE(11,12),SRC[13],MERGE(14,15)
	.endif
	subs r2,r2,#16
	bhi L176
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T208:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L208:	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	mov r11,r3,lsr #16
	strh r3,[r0],#2		@ DST[0]  = SRC[0]
	strh r11,[r0],#2		@ DST[1]  = SRC[1]
	mov r11,r4,lsr #16
	strh r4,[r0],#2		@ DST[2]  = SRC[2]
	strh r11,[r0],#2		@ DST[3]  = SRC[3]
	M_MERGE16 r11,r5
	strh r11,[r0],#2		@ DST[4]  = MERGE(SRC[4],SRC[5])
	mov r11,r6,lsr #16
	strh r6,[r0],#2		@ DST[5]  = SRC[6]
	strh r11,[r0],#2		@ DST[6]  = SRC[7]
	strh r7,[r0],#2		@ DST[7]  = SRC[8]
	mov r3,r8,lsl #16
	orr r3,r3,r7,lsr #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[8]  = MERGE(SRC[9],SRC[10])
	mov r11,r8,lsr #16
	strh r11,[r0],#2		@ DST[9]  = SRC[11]
	mov r11,r9,lsr #16
	strh r9,[r0],#2		@ DST[10] = SRC[12]
	strh r11,[r0],#2		@ DST[11] = SRC[13]
	M_MERGE16 r11,r10
	strh r11,[r0],#2		@ DST[12] = MERGE(SRC[14],SRC[15])
	.else
	stmia r0!,{r3,r4,r5,r6}
	M_MERGE32 r3,r7,r8,r11
	stmia r0!,{r3,r9,r10}
	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	str r3,[r0],#4
	M_MERGE32 r3,r4,r5,r11
	stmia r0!,{r3,r6,r7,r8}
	M_MERGE32 r3,r9,r10,r11
	str r3,[r0],#4
	.endif
	subs r2,r2,#16
	bhi L208
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T240:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L240:	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	mov r11,r3,lsr #16
	strh r3,[r0],#2
	strh r11,[r0],#2
	mov r11,r4,lsr #16
	strh r4,[r0],#2
	strh r11,[r0],#2
	mov r11,r5,lsr #16
	strh r5,[r0],#2
	strh r11,[r0],#2
	mov r11,r6,lsr #16
	strh r6,[r0],#2
	strh r11,[r0],#2
	M_MERGE16 r11,r7
	strh r11,[r0],#2
	mov r11,r8,lsr #16
	strh r8,[r0],#2
	strh r11,[r0],#2
	mov r11,r9,lsr #16
	strh r9,[r0],#2
	strh r11,[r0],#2
	mov r11,r10,lsr #16
	strh r10,[r0],#2
	strh r11,[r0],#2
	.else
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10}
	ldmia r1!,{r5,r6}
	M_MERGE32 r4,r5,r6,r11
	ldmia r1!,{r5,r6,r7,r8,r9,r10}
	stmia r0!,{r4,r5,r6,r7,r8,r9,r10}
	.endif
	subs r2,r2,#16
	bhi L240
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T256:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10}
L256:	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	subs r2,r2,#16
	.else
	subs r2,r2,#8
	.endif
	bhi L256
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10}
	mov r15,r14

C256T320:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L320:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6}
	strh r3,[r0],#2		@ DST[0]  = SRC[0]
	mov r7,r3,lsr #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[1]  = MERGE(SRC[0],SRC[1])
	strh r7,[r0],#2		@ DST[2]  = SRC[1]
	mov r11,r4,lsr #16
	strh r4,[r0],#2		@ DST[3]  = SRC[2]
	strh r11,[r0],#2		@ DST[4]  = SRC[3]
	strh r5,[r0],#2		@ DST[5]  = SRC[4]
	mov r7,r5,lsr #16
	M_MERGE16 r11,r5
	strh r11,[r0],#2		@ DST[6]  = MERGE(SRC[4],SRC[5])
	strh r7,[r0],#2		@ DST[7]  = SRC[5]
	mov r11,r6,lsr #16
	strh r6,[r0],#2		@ DST[8]  = SRC[6]
	strh r11,[r0],#2		@ DST[9]  = SRC[7]
	.else
	ldmia r1!,{r3,r5,r6,r7,r8,r10}
	M_MERGE32 r4,r3,r5,r11
	M_MERGE32 r9,r8,r10,r11
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10}	@ DST[0-7] = SRC[0],MERGE(0,1),SRC[1-4],MERGE(4,5),SRC[5]
	ldmia r1!,{r3,r4}
	stmia r0!,{r3,r4}
	.endif
	subs r2,r2,#8
	bhi L320
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T352:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L352:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6}
	strh r3,[r0],#2		@ DST[0]  = SRC[0]
	mov r7,r3,lsr #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[1]  = MERGE(SRC[0],SRC[1])
	strh r7,[r0],#2		@ DST[2]  = SRC[1]
	mov r11,r4,lsr #16
	strh r4,[r0],#2		@ DST[3]  = SRC[2]
	strh r11,[r0],#2		@ DST[4]  = SRC[3]
	orr r11,r11,r5,lsl #16
	M_MERGE16 r3,r11
	strh r3,[r0],#2		@ DST[5]  = MERGE(SRC[3],SRC[4])
	mov r11,r5,lsr #16
	strh r5,[r0],#2		@ DST[6]  = SRC[4]
	strh r11,[r0],#2		@ DST[7]  = SRC[5]
	orr r11,r11,r6,lsl #16
	M_MERGE16 r3,r11
	strh r3,[r0],#2		@ DST[8]  = MERGE(SRC[5],SRC[6])
	mov r11,r6,lsr #16
	strh r6,[r0],#2		@ DST[9]  = SRC[6]
	strh r11,[r0],#2		@ DST[10] = SRC[7]
	.else
	ldmia r1!,{r3,r5,r6,r7,r8,r9,r10}
	str r3,[r0],#4		@ DST[0]  = SRC[0]
	M_MERGE32 r4,r3,r5,r11
	stmia r0!,{r4,r5,r6,r7}	@ DST[1,2,3,4] = MERGE(0,1),SRC[1],SRC[2],SRC[3]
	M_MERGE32 r6,r7,r8,r11
	stmia r0!,{r6,r8,r9}		@ DST[5,6,7] = MERGE(3,4),SRC[4],SRC[5]
	M_MERGE32 r6,r9,r10,r11
	ldr r11,[r1],#4
	stmia r0!,{r6,r10,r11}	@ DST[8,9,10] = MERGE(5,6),SRC[6],SRC[7]
	.endif
	subs r2,r2,#8
	bhi L352
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T416:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
	M_LOADCONSTS
L416:	.ifdef BPP16
	ldmia r1!,{r3,r4,r5,r6}
	strh r3,[r0],#2		@ DST[0]  = SRC[0]
	mov r7,r3,lsr #16
	M_MERGE16 r11,r3
	strh r11,[r0],#2		@ DST[1]  = MERGE(SRC[0],SRC[1])
	strh r7,[r0],#2		@ DST[2]  = SRC[1]
	orr r11,r7,r4,lsl #16
	M_MERGE16 r3,r11
	strh r3,[r0],#2		@ DST[3]  = MERGE(SRC[1],SRC[2])
	strh r4,[r0],#2		@ DST[4]  = SRC[2]
	mov r11,r4,lsr #16
	strh r11,[r0],#2		@ DST[5]  = SRC[3]
	orr r11,r11,r5,lsl #16
	M_MERGE16 r3,r11
	strh r3,[r0],#2		@ DST[6]  = MERGE(SRC[3],SRC[4])
	strh r5,[r0],#2		@ DST[7]  = SRC[4]
	mov r11,r5
	M_MERGE16 r3,r11
	strh r3,[r0],#2		@ DST[8]  = MERGE(SRC[4],SRC[5])
	mov r11,r5,lsr #16
	strh r11,[r0],#2		@ DST[9]  = SRC[5]
	strh r6,[r0],#2		@ DST[10] = SRC[6]
	mov r11,r6,lsr #16
	M_MERGE16 r3,r6
	strh r3,[r0],#2		@ DST[11] = MERGE(SRC[6],SRC[7])
	strh r11,[r0],#2		@ DST[12] = SRC[7]
	.else
	ldmia r1!,{r3,r5,r6,r7,r8,r9,r10}
	str r3,[r0],#4		@ DST[0]      = SRC[0]
	M_MERGE32 r4,r3,r5,r11
	stmia r0!,{r4,r5}		@ DST[1,2]    = MERGE(0,1),SRC[1]
	M_MERGE32 r3,r5,r6,r11
	stmia r0!,{r3,r6,r7}		@ DST[3,4,5]  = MERGE(1,2),SRC[2],SRC[3]
	M_MERGE32 r3,r7,r8,r11
	stmia r0!,{r3,r8}		@ DST[6,7]    = MERGE(3,4),SRC[4]
	M_MERGE32 r3,r8,r9,r11
	stmia r0!,{r3,r9,r10}		@ DST[8,9,10] = MERGE(4,5),SRC[5],SRC[6]
	ldr r4,[r1],#4
	M_MERGE32 r3,r10,r4,r11
	stmia r0!,{r3,r4}		@ DST[11,12]  = MERGE(6,7),SRC[7]
	.endif
	subs r2,r2,#8
	bhi L416
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T512:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
L512:	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	mov r14,r5,lsr #16
	orr r14,r14,r14,lsl #16
	mov r12,r5,lsl #16
	orr r12,r12,r12,lsr #16
	mov r11,r4,lsr #16
	orr r11,r11,r11,lsl #16
	mov r5,r4,lsl #16
	orr r5,r5,r5,lsr #16
	mov r4,r3,lsr #16
	orr r4,r4,r4,lsl #16
	mov r3,r3,lsl #16
	orr r3,r3,r3,lsr #16
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	mov r3,r6,lsl #16
	orr r3,r3,r3,lsr #16
	mov r4,r6,lsr #16
	orr r4,r4,r4,lsl #16
	mov r5,r7,lsl #16
	orr r5,r5,r5,lsr #16
	mov r6,r7,lsr #16
	orr r6,r6,r6,lsl #16
	mov r7,r8,lsl #16
	orr r7,r7,r7,lsr #16
	mov r8,r8,lsr #16
	orr r8,r8,r8,lsl #16
	mov r12,r10,lsr #16
	orr r12,r12,r12,lsl #16
	mov r11,r10,lsl #16
	orr r11,r11,r11,lsr #16
	mov r10,r9,lsr #16
	orr r10,r10,r10,lsl #16
	mov r9,r9,lsl #16
	orr r9,r9,r9,lsr #16
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
	subs r2,r2,#16
	.else
	mov r14,r5
	mov r12,r5
	mov r11,r4
	mov r5,r4
	mov r4,r3
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	mov r3,r6
	mov r4,r6
	mov r5,r7
	mov r6,r7
	mov r7,r8
	mov r12,r10
	mov r11,r10
	mov r10,r9
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}
	subs r2,r2,#8
	.endif
	bhi L512
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

C256T768:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
L768:	ldmia r1!,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	mov r11,r3,lsl #16
	mov r14,r3,lsr #16
	mov r12,r14,lsl #16
	orr r12,r12,r11,lsr #16
	orr r11,r11,r11,lsr #16
	orr r14,r14,r14,lsl #16
	stmia r0!,{r11,r12,r14}
	mov r11,r4,lsl #16
	mov r14,r4,lsr #16
	mov r12,r14,lsl #16
	orr r12,r12,r11,lsr #16
	orr r11,r11,r11,lsr #16
	orr r14,r14,r14,lsl #16
	stmia r0!,{r11,r12,r14}
	mov r3,r5,lsl #16
	mov r5,r5,lsr #16
	mov r4,r5,lsl #16
	orr r4,r4,r3,lsr #16
	orr r3,r3,r3,lsr #16
	orr r5,r5,r5,lsl #16
	mov r11,r6,lsl #16
	mov r14,r6,lsr #16
	mov r12,r14,lsl #16
	orr r12,r12,r11,lsr #16
	orr r11,r11,r11,lsr #16
	orr r14,r14,r14,lsl #16
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	mov r3,r7,lsl #16
	mov r5,r7,lsr #16
	mov r4,r5,lsl #16
	orr r4,r4,r3,lsr #16
	orr r3,r3,r3,lsr #16
	orr r5,r5,r5,lsl #16
	mov r11,r8,lsl #16
	mov r14,r8,lsr #16
	mov r12,r14,lsl #16
	orr r12,r12,r11,lsr #16
	orr r11,r11,r11,lsr #16
	orr r14,r14,r14,lsl #16
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	mov r3,r9,lsl #16
	mov r5,r9,lsr #16
	mov r4,r5,lsl #16
	orr r4,r4,r3,lsr #16
	orr r3,r3,r3,lsr #16
	orr r5,r5,r5,lsl #16
	mov r11,r10,lsl #16
	mov r14,r10,lsr #16
	mov r12,r14,lsl #16
	orr r12,r12,r11,lsr #16
	orr r11,r11,r11,lsr #16
	orr r14,r14,r14,lsl #16
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	subs r2,r2,#16
	.else
	mov r14,r3
	mov r12,r3
	stmia r0!,{r3,r12,r14}
	mov r14,r5
	mov r12,r5
	mov r11,r5
	mov r3,r4
	mov r5,r4
	stmia r0!,{r3,r4,r5,r11,r12,r14}
	mov r14,r7
	mov r12,r7
	mov r3,r6
	mov r4,r6
	stmia r0!,{r3,r4,r6,r7,r12,r14}
	mov r14,r10
	mov r12,r10
	mov r3,r8
	mov r4,r8
	mov r5,r8
	mov r6,r9
	mov r7,r9
	stmia r0!,{r3,r4,r5,r6,r7,r9,r10,r12,r14}
	subs r2,r2,#8
	.endif
	bhi L768
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r12,r15}

TELEVIZE0:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r14}
	.ifdef BPP16
	mov r14,#0x00E3
	orr r14,r14,#0x1800
	orr r14,r14,r14,lsl #16
	.else
	mov r14,#0x000F
	orr r14,r14,#0x0F00
	orr r14,r14,r14,lsl #8
	.endif
LTV0:	ldmia r0,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	and r2,r14,r3,lsr #3
	sub r3,r3,r2
	and r2,r14,r4,lsr #3
	sub r4,r4,r2
	and r2,r14,r5,lsr #3
	sub r5,r5,r2
	and r2,r14,r6,lsr #3
	sub r6,r6,r2
	and r2,r14,r7,lsr #3
	sub r7,r7,r2
	and r2,r14,r8,lsr #3
	sub r8,r8,r2
	and r2,r14,r9,lsr #3
	sub r9,r9,r2
	and r2,r14,r10,lsr #3
	sub r10,r10,r2
	subs r1,r1,#16
	.else
	and r2,r14,r3,lsr #4
	sub r3,r3,r2
	and r2,r14,r4,lsr #4
	sub r4,r4,r2
	and r2,r14,r5,lsr #4
	sub r5,r5,r2
	and r2,r14,r6,lsr #4
	sub r6,r6,r2
	and r2,r14,r7,lsr #4
	sub r7,r7,r2
	and r2,r14,r8,lsr #4
	sub r8,r8,r2
	and r2,r14,r9,lsr #4
	sub r9,r9,r2
	and r2,r14,r10,lsr #4
	sub r10,r10,r2
	subs r1,r1,#8
	.endif
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10}
	bhi LTV0
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r15}

TELEVIZE1:	stmdb r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r14}
	.ifdef BPP16
	mov r14,#0x00E3
	orr r14,r14,#0x1800
	orr r14,r14,r14,lsl #16
	.else
	mov r14,#0x000F
	orr r14,r14,#0x0F00
	orr r14,r14,r14,lsl #8
	.endif
	mvn r11,#1
LTV1:	ldmia r0,{r3,r4,r5,r6,r7,r8,r9,r10}
	.ifdef BPP16
	eor r2,r3,r11
	and r2,r14,r2,lsr #3
	add r3,r3,r2
	eor r2,r4,r11
	and r2,r14,r2,lsr #3
	add r4,r4,r2
	eor r2,r5,r11
	and r2,r14,r2,lsr #3
	add r5,r5,r2
	eor r2,r6,r11
	and r2,r14,r2,lsr #3
	add r6,r6,r2
	eor r2,r7,r11
	and r2,r14,r2,lsr #3
	add r7,r7,r2
	eor r2,r8,r11
	and r2,r14,r2,lsr #3
	add r8,r8,r2
	eor r2,r9,r11
	and r2,r14,r2,lsr #3
	add r9,r9,r2
	eor r2,r10,r11
	and r2,r14,r2,lsr #3
	add r10,r10,r2
	subs r1,r1,#16
	.else
	eor r2,r3,r11
	and r2,r14,r2,lsr #4
	add r3,r3,r2
	eor r2,r4,r11
	and r2,r14,r2,lsr #4
	add r4,r4,r2
	eor r2,r5,r11
	and r2,r14,r2,lsr #4
	add r5,r5,r2
	eor r2,r6,r11
	and r2,r14,r2,lsr #4
	add r6,r6,r2
	eor r2,r7,r11
	and r2,r14,r2,lsr #4
	add r7,r7,r2
	eor r2,r8,r11
	and r2,r14,r2,lsr #4
	add r8,r8,r2
	eor r2,r9,r11
	and r2,r14,r2,lsr #4
	add r9,r9,r2
	eor r2,r10,r11
	and r2,r14,r2,lsr #4
	add r10,r10,r2
	subs r1,r1,#8
	.endif
	stmia r0!,{r3,r4,r5,r6,r7,r8,r9,r10}
	bhi LTV1
	ldmia r13!,{r4,r5,r6,r7,r8,r9,r10,r11,r15}

	
