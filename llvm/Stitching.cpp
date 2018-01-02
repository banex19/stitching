#define DEBUG_TYPE "stitching"

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
#include <string>
#include <vector>
using namespace llvm;


static std::set<std::string> functions  {"log","cosf","sinf", "sqrt", "strtod",
 //"memset", "calloc", "memcpy"
};

namespace {
	class StitchingPass : public MachineFunctionPass {

	private:
		std::set<std::string> internalFunctions;

		void ReadInternalFunctionsList(const std::string& internalFnListPath)
		{
			if (internalFnListPath == "")
			{  
				return;
			}

			auto MBOrErr = MemoryBuffer::getFile(internalFnListPath);
			if (auto EC = MBOrErr.getError()) 
			{
				return;
			}

			std::unique_ptr<MemoryBuffer> &MB = *MBOrErr;
			std::string buffer = MB->getMemBufferRef().getBuffer().str();

			size_t curPos = 0;

			while (curPos < buffer.size())
			{
				std::string name = buffer.substr(curPos, buffer.find('\n', curPos) - curPos);
				curPos += name.size() + 1;
				internalFunctions.insert(name);
			}
		}

	public:
		static char ID;
		StitchingPass(const std::string& internalFnListPath) : MachineFunctionPass(ID) {

			ReadInternalFunctionsList(internalFnListPath); 
		}

		StitchingPass() : MachineFunctionPass(ID) {
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

			GlobalValue* stitchFunction = (GlobalValue*) M->getNamedValue("__stitch_relocation");

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

						BuildMI(block, instr, DebugLoc(), TII->get(X86::CALL64pcrel32)).addGlobalAddress(stitchFunction);

						insertCall = false;
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
							if (global->isDeclarationForLinker() && global->getName() != "__stitch_relocation"
							//	&& functions.count(global->getName())
								&& MF.getName().str().substr(0,9) != "__stitch_"
								)
							{
								if (global->getName() != "expf")
								{
								//	continue;
								}

								if (internalFunctions.find(global->getName()) == internalFunctions.end())
								{
									originalCall = &instr;
									insertCall = true; 
								}
							}
						}
					}

					++num_instr;
				}
			}

			for (MachineBasicBlock& block : MF) {
				for (MachineInstr& instr : block) 
				{
					if (instr.getDesc().isCall())
					{		
						const GlobalValue* global = instr.getOperand(0).getGlobal();
						if (global->isDeclarationForLinker() && global->getName() == "__stitch_relocation")
						{

							instr.copyImplicitOps(MF, *originalCall);
							instr.getOperand(0).setTargetFlags(originalCall->getOperand(0).getTargetFlags());

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


		//	errs() << "Count: --- " << MF.getName() << " has "
		//	<< num_instr << " calls.\n";
			return false;
		} 
	};
}


FunctionPass *llvm::createX86StitchingPass(const std::string& internalFnListPath) {
	return new StitchingPass(internalFnListPath);
}

char StitchingPass::ID = 0;
static RegisterPass<StitchingPass> X("stitching", "Stitching Pass");