	.file	"memset.s"
	.text
	.align	2
	.global	memset
	.type	memset,function
	@ very slow, but compact and simple memset (no stack needed)...
	@ void *s <- r0
	@ int c <- r1
	@ int n <- r2
memset:
	add	r3, r2, r0
.Loop:
	cmp	r0, r3
	beq	.EndLoop
	strb	r1, [r0]
	add	r0, r0, #1
	b	.Loop
.EndLoop:	
	mov	pc, lr
ememset:
	.size	memset,.ememset-memset
