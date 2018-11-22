	.cpu arm7tdmi
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 1
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"kvasir.cpp"
	.text
	.align	2
	.global	main
	.syntax unified
	.arm
	.fpu softvfp
	.type	main, %function
main:
	.fnstart
.LFB1581:
	@ Function supports interworking.
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}
	.save {r4, lr}
	.syntax divided
@ 28 "kvasir.cpp" 1
	@ zero REG_WIN0V start
@ 0 "" 2
	.arm
	.syntax unified
	mov	r2, #67108864
	mov	r3, #0
	strh	r3, [r2, #68]	@ movhi
	.syntax divided
@ 30 "kvasir.cpp" 1
	@ zero REG_WIN0V end
@ 0 "" 2
@ 33 "kvasir.cpp" 1
	@ set REG_WIN0V start
@ 0 "" 2
	.arm
	.syntax unified
	ldrh	r3, [r2, #68]
	and	r3, r3, #65280
	orr	r3, r3, #160
	strh	r3, [r2, #68]	@ movhi
	.syntax divided
@ 36 "kvasir.cpp" 1
	@ set REG_WIN0V end
@ 0 "" 2
	.arm
	.syntax unified
.L2:
	bl	VBlankIntrWait
	b	.L2
	.fnend
	.size	main, .-main
	.ident	"GCC: (devkitARM release 47) 7.1.0"
