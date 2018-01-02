#define DEBUG_TYPE "trampolinedstitching"

#include "X86InstrInfo.h"
#include "X86.h"
#include "X86InstrBuilder.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include "llvm/Target/TargetOpcodes.h"
#include "llvm/CodeGen/MachineOperand.h"
#include <set>
#include <vector>
using namespace llvm;


static std::set<std::string> functions  {"log","cosf","sinf", "sqrt", "strtod",
 //"memset", "calloc", "memcpy"
};

namespace {
	class TrampolinedStitchingPass : public MachineFunctionPass {
	public:
		static char ID;
		TrampolinedStitchingPass() : MachineFunctionPass(ID) {


		}

		/*virtual bool runOnMachineFunction(MachineFunction &MF) {	
			const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();

			const Function* F = MF.getFunction();
			Module* M = (Module*)F->getParent();

			GlobalValue* stitchFunction = (GlobalValue*) M->getNamedValue("__stitch_relocation");

			auto newOp = MachineOperand::CreateGA(stitchFunction, 6);

			size_t numRets = 0;
			bool lastBlock = false;

			MachineInstr* retInstr = nullptr;
			MachineBasicBlock* retBlock = nullptr;

			for (MachineBasicBlock& block : MF) 
			{
				lastBlock = false;
				for (MachineInstr& instr : block) 
				{
					if (instr.isReturn())
					{
						numRets++;
						lastBlock = true;

						if (numRets > 1)
							return false;

						retInstr = &instr;
						retBlock = &block;

					//	errs() << "STITCHER ---- " << instr << "\n";
					}
				}
			}

			if (numRets > 1)
			{
			//	errs() << "STITCHER ---- Function with more than one RET\n";
			//	MF.print(errs());
			}
			else if (numRets == 1)
			{
				BuildMI(*retBlock, *retInstr, DebugLoc(), TII->get(X86::CALL64pcrel32)).addGlobalAddress(stitchFunction);

				if (lastBlock == true)
				{
					
				//	errs() << "STITCHER ---- Function with only one RET\n";
				}
				else
				{
					//	errs() << "STITCHER ---- Function with one RET but not at the end\n";
					//	MF.print(errs());
				}
			}

			return false;
		} */


		virtual bool runOnMachineFunction(MachineFunction &MF) {	

		//	errs() << "STITCHING ---- Running stitching pass\n";

			unsigned num_instr = 0;

			const TargetInstrInfo *TII = MF.getSubtarget().getInstrInfo();
			const TargetRegisterInfo *TRI = MF.getSubtarget().getRegisterInfo();

			const Function* F = MF.getFunction();
			Module* M = (Module*)F->getParent();

			GlobalValue* stitchFunction = (GlobalValue*) M->getNamedValue("__stitch_relocation_indirect");

			auto newOp = MachineOperand::CreateGA(stitchFunction, 0);

			bool insertedAtLeastOnce = false;

			bool insertCall = false;
			//bool stitchInstr = false;

			MachineInstr* originalCall = nullptr;

		//	MF.print(errs());

			for (MachineBasicBlock& block : MF) {
				for (MachineInstr& instr : block) 
				{
					if (insertCall)
					{
						for (int i = 0; i < 3; ++i)
						{
					//		BuildMI(block,instr, DebugLoc(), TII->get(X86::NOOP));
						}

						//BuildMI(block, instr, DebugLoc(), TII->get(X86::CALL64pcrel32)).addGlobalAddress(stitchFunction);

						insertCall = false;

						insertedAtLeastOnce = true;
					}

					//errs() << "STITCHING --- Instr: " << instr << "\n";

					if (instr.getDesc().isCall() )
					{
						//BuildMI(block,instr, DebugLoc(), TII->get(X86::NOOP));
					//	BuildMI(block,instr, DebugLoc(), TII->get(X86::MOV64ri32), X86::RAX).addImm(1);

						//errs() << "Num operands: " << instr.getNumExplicitOperands() << "\n";
						//errs() << "STITCHING --- Found call\n";
						if (instr.getOpcode() != X86::CALL64pcrel32)
						{
							//	errs() << "Invalid call: " << instr << "\n";
								continue;
						}
						
						
						if ( instr.getOperand(0).isGlobal() )
						{
							const GlobalValue* global = instr.getOperand(0).getGlobal();
							if (global->isDeclarationForLinker() && global->getName() != "__stitch_relocation_indirect"
							//	&& functions.count(global->getName())
								&& MF.getName().str().substr(0,9) != "__stitch_"
								)
							{
								auto name = global->getName();

							//	errs() << "Call: " << instr << "\n";

								auto* stitchHelper = M->getNamedValue("__stitch_" + name.str());						
								if (stitchHelper != nullptr)
								{
								//	errs() << "Helper: " << *stitchHelper << "\n";
									auto flags = instr.getOperand(0).getTargetFlags();
									instr.RemoveOperand(0);
									instr.addOperand(MachineOperand::CreateGA( stitchHelper, flags)); 
								}

								originalCall = &instr;
								insertCall = true; 
							//	errs() << "STITCHING --- Found call\n";
							}
							//else 
							if (global->isDeclarationForLinker() && global->getName() == "__stitch_relocation_indirect")
							{
								for (auto& op : instr.operands())
								{
									if (op.isRegMask())
									{
										const uint32_t* regMask =  op.getRegMask();

										int maskSize = (TRI->getNumRegs()+31)/32;

										uint32_t* newMask = new uint32_t[maskSize];
										for (int i = 0; i < maskSize; ++i)
										{
											newMask[i] = regMask[i];
										}

										unsigned savedRegs[] = {X86::EAX, X86::RAX, X86::XMM0, X86::XMM1, X86::EDX, X86::RDX};

										for (auto& reg : savedRegs)
											newMask[reg / 32] |= ((uint32_t)1 << (reg % 32));

										op.setRegMask(newMask);
									}
								} 
							}
							//	errs() << "Instruction: " << instr << "\n";

							//	MF.print(errs());
							//	stitchInstr = true;
						}



							//const GlobalValue* global = instr.getOperand(0).getGlobal();
							//errs() << "Name: " << global->getName() << "\n";
							//errs() << "Declaration for linker: " << global->isDeclarationForLinker() << "\n";
							//instr.RemoveOperand(0);
						//	instr.addOperand(MF, newOp);
					}

					++num_instr;
				}
			}


			//if (!insertedAtLeastOnce)
			{
				for (MachineBasicBlock& block : MF) {
					for (MachineInstr& instr : block) 
					{
						if (instr.getDesc().isCall())
						{		
							const GlobalValue* global = instr.getOperand(0).getGlobal();
							if (global->isDeclarationForLinker() && global->getName() == "__stitch_relocation")
							{

								//instr.copyImplicitOps(MF, *originalCall);
								//instr.getOperand(0).setTargetFlags(originalCall->getOperand(0).getTargetFlags());

							//	errs() << "Call to stitch: " << instr << "\n";
							//	errs() << "STITCHING --- Found call\n";
							}
							else
							{
							//	errs() << "Call: " << instr << "\n";
								originalCall = &instr;
							}
						} 
					}
				}
			} 

		//	errs() << "Count: --- " << MF.getName() << " has "
		//	<< num_instr << " calls.\n";
			return false;
		} 
	};
}


FunctionPass *llvm::createX86TrampolinedStitchingPass() {
	return new TrampolinedStitchingPass();
}

char TrampolinedStitchingPass::ID = 0;
static RegisterPass<TrampolinedStitchingPass> X("trampolinedstitching", "Trampolined Stitching Pass");