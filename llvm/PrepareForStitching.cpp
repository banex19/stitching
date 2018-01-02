#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Support/MemoryBuffer.h"
#include <vector>
#include <map>
#include <set>

using namespace llvm;


namespace {
    struct PrepareForStitching : public ModulePass {

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
        PrepareForStitching(const std::string& internalFnListPath) : ModulePass(ID) 
        {
            ReadInternalFunctionsList(internalFnListPath);  
        }

        bool runOnModule(Module &M) override
        {
            FunctionType* stitchFunctionType = FunctionType::get(Type::getVoidTy(M.getContext()), false);

            auto* indirectStitchFunction = M.getOrInsertFunction("__stitch_relocation_indirect",
                stitchFunctionType);

            auto* stitchFunction = M.getOrInsertFunction("__stitch_relocation",
                stitchFunctionType);

            IRBuilder<> builder(M.getContext());

            std::map<Function*, Function*> remap;

            for (auto& F : M)
            {
                if (F.isDeclaration() && F.getName() != "__stitch_relocation_indirect" && F.getName() != "__stitch_relocation" && 
                    !F.isIntrinsic() && !F.isVarArg())
                {

                    if (internalFunctions.find(F.getName()) != internalFunctions.end())
                    {
                        continue;
                    }
                    if (F.getName() != "expf" 
                        && false
                      //  && F.getName() != "expf"
                        )
                    {
                        continue;
                    }

                    std::string fnName = F.getName().str();
                    auto* newFunction = (Function*)M.getOrInsertFunction("__stitch_" + fnName,
                        F.getFunctionType());

                    remap[&F] = newFunction;

                    newFunction->setAttributes(F.getAttributes());
                    newFunction->addFnAttr(Attribute::NoInline);
                    //newFunction->addFnAttr(Attribute::Naked);
                    newFunction->addFnAttr(Attribute::MinSize);

                    newFunction->setCallingConv(CallingConv::C);
                    newFunction->setLinkage(GlobalValue::LinkOnceODRLinkage);

                //  F.replaceAllUsesWith(newFunction);



                    BasicBlock* block = BasicBlock::Create(M.getContext(), "entry", newFunction);
                    builder.SetInsertPoint(block);

                    std::vector< Value*> args;
                    for(auto& arg : newFunction->args())
                    {
                        args.push_back(&arg);
                    }

                    auto* returnValue = builder.CreateCall(&F, ArrayRef< Value*>(args));
                    builder.CreateCall(indirectStitchFunction);
                    if (F.getFunctionType()->getReturnType() == Type::getVoidTy(M.getContext()))
                        builder.CreateRetVoid();
                    else
                        builder.CreateRet(returnValue);



                    //errs() << "Old function: " << *F.getFunctionType() << "\n" << F << "\n";
                    //errs() << "New function: " << *newFunction->getFunctionType() << "\n" << *newFunction << "\n";
                //  errs() << "Old function args: " << F.getFunctionType()->getNumParams() << "\nNew function args: " << newFunction->getFunctionType()->getNumParams() << "\n"; 
                }
            } 

         /*   for (auto& F : M)
            {
                if (F.getName().str().substr(0,9) == "__stitch_")
                    continue;

                for (auto& block : F)
                {
                    for (auto& inst : block)
                    {
                        CallInst* call = dyn_cast<CallInst>(&inst);

                        if (call != nullptr)
                        {
                            Function* callee = call->getCalledFunction();


                            if (callee && callee->isDeclaration() 
                                && !call->isTailCall() 
                                && remap.find(callee) != remap.end())
                            {
                            //  errs() << "Before: " << inst << "\n";

                                call->setCalledFunction(remap[callee]);

                               // if (call->isTailCall())
                               //     call->setTailCall(false);

                            //  errs() << "After: " << inst << "\n";
                            }
                        }
                    }
                }
            }  */

            if (verifyModule(M, &errs()) == true)
            {
                assert(false);
            } 

            //M.setPICLevel(PICLevel::BigPIC);


            //IRBuilder<> builder(M.getContext());

    //      errs() << "Function name: ";
    //      errs().write_escaped(F.getName()) << '\n';
    //      errs() << "Number of blocks: " << F.getBasicBlockList().size() << "\n";

        /*  for (auto& F : M)
            {
                for (auto& block : F)
                {
                    for (auto& inst : block)
                    {
                        CallInst* call = dyn_cast<CallInst>(&inst);

                        if (call != nullptr)
                        {
                            Function* callee = call->getCalledFunction();

                            if (callee->getName() != "__stitch_relocation")
                            {
                                bool external = callee->isDeclaration();

                            //errs() << "Found call to " << ((callee != nullptr) ? callee->getName() : "(indirect)") << "\n";
                            //errs() << "Library call: " << (external ? "true" : "false") << "\n";

                                if (external)
                                {
                                //builder.SetInsertPoint(inst.getNextNode());
                            //  builder.CreateCall(stitchFunction);


                                    return false;
                                }
                            }
                        }
                    }
                }
            } */

        //  errs() << "\n";

        //  errs() << "Function:\n" << F; */
            return false;
        }
    }; 
}  // end of anonymous namespace

ModulePass * llvm::createPrepareForStitchingPass(const std::string& internalFnListPath) {
    return new PrepareForStitching(internalFnListPath);
}


char PrepareForStitching::ID = 0;
//INITIALIZE_PASS(PrepareForStitching, "preparestitchingpass", "Prepare For Stitching Pass",
//  false, false)


/*
/// Register the pass with the pass manager builder.  This instructs the
/// builder to call the `addSimplePass` function at the end of adding other
/// optimisations, so that we can insert the pass.  See the
/// `PassManagerBuilder` documentation for other extension points.
    RegisterStandardPasses SOpt(PassManagerBuilder::EP_OptimizerLast,
        addSimplePass);
/// Register the pass to run at -O0.  This is useful for debugging the pass,
/// though modifications to this pass will typically want to disable this, as
/// most passes don't make sense to run at -O0.
    RegisterStandardPasses S(PassManagerBuilder::EP_EnabledOnOptLevel0,
        addSimplePass);

        */