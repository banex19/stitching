#include <stdio.h>
#include <stdint.h>

/*
1	90                              NOP
2	6690                            66 NOP
3	0f1f00                          NOP DWORD ptr [EAX]
4	0f1f4000                        NOP DWORD ptr [EAX + 00H]
5	0f1f440000                      NOP DWORD ptr [EAX + EAX*1 + 00H]
6	660f1f440000                    66 NOP DWORD ptr [EAX + EAX*1 + 00H]
7	0f1f8000000000                  NOP DWORD ptr [EAX + 00000000H]
8	0f1f840000000000                NOP DWORD ptr [EAX + EAX*1 + 00000000H]
9	660f1f840000000000              66 NOP DWORD ptr [EAX + EAX*1 + 00000000H]
*/

static const int CALL_OP_OFF_SIZE = 5;
static const int CALL_OP_SIZE = 1;
static const int CALL_OFF_SIZE = 4;

static const int JUMP_OP_OFF_SIZE = 6;
static const int JUMP_OP_SIZE = 2;
static const int JUMP_OFF_SIZE = 4;

void* __getAddressFromCallInstr(void* callInstr)
{
	unsigned char* code = (unsigned char*) callInstr;

	code += CALL_OP_SIZE;
	int offset = *(int*)(code);
	code += CALL_OFF_SIZE;

	return (void*)(code + offset);
}

void* __getAddressFromJumpInstr(void* jumpInstr)
{
	unsigned char* code = (unsigned char*) jumpInstr;

	code += JUMP_OP_SIZE;
	int offset = *(int*)(code);
	code += JUMP_OFF_SIZE;

	return (void*)(code + offset);
}

//#define ENABLE_DEBUG

int __isJumpInstr(void* instr)
{
	unsigned char* code = (unsigned char*) instr;

	return (*code == 0xFF 
		#ifdef ENABLE_DEBUG
		||*code == 0xCC
		#endif
		) && *(code + 1) == 0x25;
}

int __isCallInstr(void* instr)
{
	unsigned char* code = (unsigned char*) instr;

	return *code == 0xE8;
}

void __write8ByteNOP(unsigned char* code)
{
	*code++ = 0x0F;
	*code++ = 0x1F;
	*code++ = 0x84;
	*code++ = 0x00;
	*code++ = 0x00;
	*code++ = 0x00;
	*code++ = 0x00;
	*code++ = 0x00;
}

void __write5ByteNOP(unsigned char* code)
{
	*code++ = 0x0F;
	*code++ = 0x1F;
	*code++ = 0x44;
	*code++ = 0x00;
	*code++ = 0x00; 

	/**code++ = 0xEB;
	*code++ = 0x03;
	*code++ = 0x00;
	*code++ = 0x00;
	*code++ = 0x00;  */

	/**code++ = 0x43;
	*code++ = 0x0F;
	*code++ = 0x1F;
	*code++ = 0x04;
	*code++ = 0x1B;  */

	/*for (int i = 0; i < 5; ++i)
	{
		*code++ = 0x90;
	}*/
}

void __attribute__((noinline)) __stitch_logic(void* addr)
{
	//printf("Calling address: %p\n", addr);

	unsigned char* code = (unsigned char*) addr;
//	code -= CALL_OP_OFF_SIZE* 2 + 3;

	code -= CALL_OP_OFF_SIZE;
	__write5ByteNOP(code);

	
	code -= CALL_OP_OFF_SIZE ;

	void* pltAddr = __getAddressFromCallInstr(code);

	if (!__isJumpInstr(pltAddr))
	{
		//code += CALL_OP_OFF_SIZE;

		//__write8ByteNOP(code);
	//	printf("Call is not external\n");
		return;
	}

	void* gotAddr = __getAddressFromJumpInstr(pltAddr);

	void* fnAddr = (void*)(*(uintptr_t*)gotAddr);

	int reachable = (fnAddr - addr) < (intptr_t)INT32_MAX
	&& (fnAddr - addr) > -(intptr_t)INT32_MAX;

	//printf("PLT address: %p\n", pltAddr);
	//printf("GOT address: %p\n", gotAddr);
/*	printf("Function address: %p (32 bit reach from %p: %s)\n", fnAddr,
	 code + CALL_OP_OFF_SIZE, 
	 reachable ? "true" : "false"); */

	if (reachable)
	{
		int offset = fnAddr - (void*)(code + CALL_OP_OFF_SIZE);
		code += CALL_OP_SIZE;

		int* offsetCode = (int*)code;
		*offsetCode = offset;

	//	code += CALL_OFF_SIZE;

		//__write8ByteNOP(code);
	//	printf("---- STITCHER ---- Call stitched\n");
	}
	else 
	{ 
	/*	*code++ = 0x49;
		*code++ = 0xBB;

		uintptr_t* fnCode = (uintptr_t*) code;
		*fnCode = (uintptr_t)fnAddr;

		code += 8;

		*code++ = 0x41;
		*code++ = 0xFF;
		*code++ = 0xD3;  */
	//	printf("---- STITCHER ---- Call is not within 32bit\n");
	} 
}

void __replaceCallWithNewAddress(unsigned char* instrAfterCall, void* newAddress)
{
	int offset = newAddress - (void*)(instrAfterCall);
	instrAfterCall -= CALL_OFF_SIZE;

	int* offsetCode = (int*)instrAfterCall;
	*offsetCode = offset;
}

void __attribute__((noinline)) __stitch_logic_indirect(void* stubAddr, void* originalAddr)
{

	//printf("Stub address: %p\n", stubAddr);
	//printf("Original address: %p\n", originalAddr);
	//return;

	unsigned char* code = (unsigned char*) stubAddr;

	code -= CALL_OP_OFF_SIZE;
	//__write5ByteNOP(code);

	
	code -= CALL_OP_OFF_SIZE ;

	if (!__isCallInstr(code))
	{
		//printf ("Too many stack operations %p\n", stubAddr);
		code += CALL_OP_OFF_SIZE;
		__write5ByteNOP(code);
		return ;
	}

	void* pltAddr = __getAddressFromCallInstr(code);

//	printf("Tentative PLT address: %p\n", pltAddr);

	if (!__isJumpInstr(pltAddr))
	{
	//	code += CALL_OP_OFF_SIZE;

	//	__write5ByteNOP(code);

		__replaceCallWithNewAddress(originalAddr, pltAddr);

		code = (unsigned char*) pltAddr;

		//printf("Call is not external (first  opcodes %02x %02x %02x %02x %02x)\n", *code, *(code+1), *(code+2), *(code+3), *(code+4));
		return;
	}

	void* gotAddr = __getAddressFromJumpInstr(pltAddr);

	void* fnAddr = (void*)(*(uintptr_t*)gotAddr);

	int reachable = (fnAddr - originalAddr) < (intptr_t)INT32_MAX
	&& (fnAddr - originalAddr) > -(intptr_t)INT32_MAX;

	//printf("PLT address: %p\n", pltAddr);
	//printf("GOT address: %p\n", gotAddr);
/*	printf("Function address: %p (32 bit reach from %p: %s)\n", fnAddr,
	 code + CALL_OP_OFF_SIZE, 
	 reachable ? "true" : "false"); */

	if (reachable)
	{
		__replaceCallWithNewAddress(originalAddr, fnAddr);

	//	code += CALL_OFF_SIZE;

		//__write8ByteNOP(code);
	//	printf("---- STITCHER ---- Call to %p stitched with %p (offset %d)\n", originalAddr, fnAddr, (int)(fnAddr - originalAddr));
	}
	else 
	{ 
	/*	*code++ = 0x49;
		*code++ = 0xBB;

		uintptr_t* fnCode = (uintptr_t*) code;
		*fnCode = (uintptr_t)fnAddr;

		code += 8;

		*code++ = 0x41;
		*code++ = 0xFF;
		*code++ = 0xD3;  */
	//	printf("---- STITCHER ---- Call to %p is not within 32bit from %p\n", fnAddr, originalAddr);
	} 
}

//#define SAVE_RDX 
//#define SAVE_XMM

#define STACK_ALIGNED

//#define DISABLE_STITCHER
//#define DISABLE_ALL

void __attribute__((noinline)) __attribute__((naked)) __stitch_relocation()
{
#ifndef DISABLE_ALL
	#ifndef DISABLE_STITCHER		
	__asm__("mov (%rsp), %rdi");	
	#endif

		// Save RAX and RDX.
	__asm__("pushq %rax");

	#ifdef SAVE_RDX
	__asm__("pushq %rdx");
	#endif

	    // Save XMM0 and XMM1.
	#ifdef SAVE_XMM
	__asm__("subq $16, %rsp");
	__asm__("movdqu  %xmm0, (%rsp) ");
	__asm__("subq $16, %rsp");
	__asm__("movdqu  %xmm1, (%rsp)");
	#endif

	#ifndef DISABLE_STITCHER
	__asm__("callq __stitch_logic");
	#endif

	    // Restore XMM0 and XMM1.
	#ifdef SAVE_XMM
	__asm__("movdqu  (%rsp), %xmm1");
	__asm__("addq $16, %rsp");
	__asm__("movdqu  (%rsp), %xmm0");
	__asm__("addq $16, %rsp"); 
	#endif

		// Restore RAX and RDX.
	#ifdef SAVE_RDX
	__asm__("popq %rdx");
	#endif

	__asm__("popq %rax");
 #endif // DISABLE_ALL

    // Return.
	__asm__("retq");
}

void __attribute__((noinline)) __attribute__((naked)) __stitch_relocation_indirect()
{
#ifndef DISABLE_ALL
	#ifndef DISABLE_STITCHER
		#ifdef STACK_ALIGNED
	__asm__("mov 16(%rsp), %rsi");
		#else
	__asm__("mov 8(%rsp), %rsi");
		#endif		
	__asm__("mov (%rsp), %rdi");	
	#endif

		// Save RAX and RDX.
	__asm__("pushq %rax");

	#ifdef SAVE_RDX
	__asm__("pushq %rdx");
	#endif

	    // Save XMM0 and XMM1.
	#ifdef SAVE_XMM
	__asm__("subq $16, %rsp");
	__asm__("movdqu  %xmm0, (%rsp) ");
	__asm__("subq $16, %rsp");
	__asm__("movdqu  %xmm1, (%rsp)");
	#endif

	#ifndef DISABLE_STITCHER
	__asm__("callq __stitch_logic_indirect");
	#endif

	    // Restore XMM0 and XMM1.
	#ifdef SAVE_XMM
	__asm__("movdqu  (%rsp), %xmm1");
	__asm__("addq $16, %rsp");
	__asm__("movdqu  (%rsp), %xmm0");
	__asm__("addq $16, %rsp"); 
	#endif

		// Restore RAX and RDX.
	#ifdef SAVE_RDX
	__asm__("popq %rdx");
	#endif

	__asm__("popq %rax");
 #endif // DISABLE_ALL

    // Return.
	__asm__("retq");
}