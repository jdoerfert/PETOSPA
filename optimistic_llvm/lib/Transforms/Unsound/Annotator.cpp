//===- Unsound/Annotator.cpp - TODO -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Unsound/Annotator.h"

#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpander.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Regex.h"
#include "llvm/Transforms/Unsound/Config.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

using namespace llvm;

#define DEBUG_TYPE "optimistic-annotator"

static cl::list<std::string>
    OnlyFunctions("optimistic-annotator-only-functions", cl::ZeroOrMore,
                  cl::CommaSeparated, cl::Hidden);

static cl::opt<bool>
    OptimisticAnnotationsDefault("optimistic-annotations-default-optimistic",
                                 cl::init(false), cl::Hidden);

static cl::opt<int> OptimisticAnnotationRuns("optimistic-annotation-runs",
                                             cl::init(-1), cl::Hidden);

static cl::opt<std::string>
    OptimisticAnnotationsControlStr("optimistic-annotations-control",
                                    cl::ZeroOrMore, cl::init(""), cl::Hidden);

static cl::opt<bool> PrintOptimisticChoices("print-optimistic-choices",
                                            cl::init(false), cl::Hidden);

static cl::opt<bool>
    PrintOptimisticOpportunities("print-optimistic-opportunities",
                                 cl::init(false), cl::Hidden);

STATISTIC(NumAlignedMemoryInst, "#Memory accesses annotated as aligned");
STATISTIC(NumDereferenceableMemoryInst,
          "#Memory accesses annotated as dereferenceable");
STATISTIC(NumInvariantLoads, "#Loads annotated as invariant");

STATISTIC(NumNoSignedWrapOps, "#Ops annotated as no-signed wrap");
STATISTIC(NumNoUnsignedWrapOps, "#Ops annotated as no-unsigned wrap");
STATISTIC(NumInboundGEPs, "#GEPs annotated as inbounds");

STATISTIC(NumNoAliasParameters, "#Parameters annotated as no-alias");
STATISTIC(NumAlignedParameters, "#Parameters annotated as aligned");
STATISTIC(NumReadNoneParameters, "#Parameters annotated as read-none");
STATISTIC(NumReadOnlyParameters, "#Parameters annotated as read-only");
STATISTIC(NumWriteOnlyParameters, "#Parameters annotated as write-only");
STATISTIC(NumNoCaptureParameters, "#Parameters annotated as no-capture");
STATISTIC(NumDereferenceableParameters,
          "#Parameters annotated as dereferenceable");
STATISTIC(NumReturnedParameters, "#Parameters annotated as returned");

STATISTIC(NumReadNoneFunctions, "#Functions annotated as read-none");
STATISTIC(NumReadOnlyFunctions, "#Functions annotated as read-only");
STATISTIC(NumWriteOnlyFunctions, "#Functions annotated as write-only");
STATISTIC(NumFunctionsInternalized,
          "#Functions annotated as internal (static)");
STATISTIC(NumFunctionsNoUnwind, "#Functions annotated as no-unwind");
STATISTIC(NumFunctionsArgMemOnly, "#Functions annotated as arg-mem only");
STATISTIC(NumFunctionsInaccessibleOnly,
          "#Functions annotated as inaccessible only");
STATISTIC(NumFunctionsArgMemOrInaccessibleOnly,
          "#Functions annotated as arg-mem or inaccessible only");
STATISTIC(NumFunctionsSpeculatable, "#Functions annotated as speculatable");
STATISTIC(NumFunctionsNoAliasReturn, "#Function returns annotated as no-alias");
STATISTIC(NumFunctionsDereferenceableReturn,
          "#Function returns annotated as dereferenceable");

STATISTIC(NumConditionalBranchesEliminated,
          "#Branches annotated as always taken");

STATISTIC(NumParallelInnermostLoops, "#Innermost loops annotated as parallel");
STATISTIC(NumParallelOuterLoops, "#Outer loops annotated as parallel");
STATISTIC(NumBackedgeTakenLoops, "#Loops annotated with minimum one iteration");

STATISTIC(NumChoicesCachedByName, "#Optimistic choices cached by name");

namespace {
template <typename T> using GetterType = std::function<T *(Function &)>;
}

enum OptimisticChoiceKind {

  OCK_FIRST = 0,

  OCK_PARAMETER_NO_ALIAS,
  OCK_FUNCTION_RETURN_NO_ALIAS,
  OCK_PARAMETER_NO_CAPTURE,
  OCK_OVERFLOW_NSW,
  OCK_OVERFLOW_NUW,
  OCK_INBOUNDS_GEP,
  OCK_FUNCTION_NO_UNWIND,
  OCK_LOOP_PARALLEL_INNERMOST,
  OCK_LOOP_PARALLEL_OUTER,
  OCK_FUNCTION_INTERNAL,
  OCK_FUNCTION_RETURN_DEREFERENCEABLE,
  OCK_PARAMETER_DEREFERENCEABLE,
  OCK_MEMORY_ACCESS_DEREFERENCEABLE,
  OCK_PARAMETER_ALIGNED,
  OCK_MEMORY_ACCESS_ALIGN,
  OCK_MEMORY_ACCESS_RES_ALIGN,
  OCK_CONTROL_FLOW_TRG,
  OCK_MEMORY_LOAD_INVARIANT,
  OCK_PARAMETER_RETURNED,
  OCK_PARAMETER_MEM_BEHAVIOR,
  OCK_FUNCTION_MEM_EFFECTS,
  OCK_LOOP_BACKEDGE_TAKEN,
  OCK_LAST
};

template <class T>
static bool shouldAnnotateFunction(Function &F, T &OnlyFunctionsSet) {
  if (OnlyFunctionsSet.empty())
    return true;

  for (const std::string &OnlyFunction : OnlyFunctionsSet) {
    Regex R(OnlyFunction);

    std::string Err;
    if (!R.isValid(Err)) {
      errs() << "invalid regex: " << Err << "\n";
      return false;
    }

    if (R.match(F.getName()))
      return true;
  }

  return false;
}

static bool isValidControlChar(int NumChoices, char ControlChar) {
  return ControlChar >= '0' && ControlChar < '0' + NumChoices;
}

static unsigned getControlCharChoice(char ControlChar) {
  return ControlChar - '0' + 1;
}

static OptimisticChoiceKind
getNextOptimisticChoiceKind(OptimisticChoiceKind LastKind) {
  return (OptimisticChoiceKind)(LastKind + 1);
}

static OptimisticChoiceKind
getOptimisticChoiceKindForControlChar(const char ControlChar) {
  assert(isValidControlChar(OCK_LAST, ControlChar));
  return (OptimisticChoiceKind)(getControlCharChoice(ControlChar) - 1);
}

static const Function *getFnForValue(const Value &V) {
  if (auto *I = dyn_cast<Instruction>(&V))
    return I->getFunction();
  if (auto *A = dyn_cast<Argument>(&V))
    return A->getParent();
  if (auto *B = dyn_cast<BasicBlock>(&V))
    return B->getParent();
  if (auto *F = dyn_cast<Function>(&V))
    return F;
  V.dump();
  assert(0);
}

static StringRef getFnNameForValue(const Value &V) {
  if (auto *I = dyn_cast<Instruction>(&V))
    return I->getFunction()->getName();
  if (auto *A = dyn_cast<Argument>(&V))
    return A->getParent()->getName();
  if (auto *B = dyn_cast<BasicBlock>(&V))
    return B->getParent()->getName();
  if (auto *F = dyn_cast<Function>(&V))
    return F->getName();
  return "<Unknown>";
}

inline raw_ostream &operator<<(raw_ostream &OS, OptimisticChoiceKind OCT) {
  switch (OCT) {
  case OCK_FIRST:
    return OS << "[N/A][FIRST    ]";
  case OCK_OVERFLOW_NSW:
    return OS << "[Op ][NoSgnWrap]";
  case OCK_OVERFLOW_NUW:
    return OS << "[Op ][NoUnsWrap]";
  case OCK_INBOUNDS_GEP:
    return OS << "[GEP][Inbounds ]";
  case OCK_PARAMETER_NO_ALIAS:
    return OS << "[Par][NoAlias  ]";
  case OCK_PARAMETER_ALIGNED:
    return OS << "[Par][Alignment]";
  case OCK_PARAMETER_NO_CAPTURE:
    return OS << "[Par][NoCapture]";
  case OCK_PARAMETER_DEREFERENCEABLE:
    return OS << "[Par][Dereferen]";
  case OCK_PARAMETER_MEM_BEHAVIOR:
    return OS << "[Par][MemAccess]";
  case OCK_FUNCTION_RETURN_NO_ALIAS:
    return OS << "[Fn][RetNoAlia ]";
  case OCK_FUNCTION_RETURN_DEREFERENCEABLE:
    return OS << "[Fn][RetDerefe ]";
  case OCK_FUNCTION_INTERNAL:
    return OS << "[Fn][Internaliz]";
  case OCK_FUNCTION_NO_UNWIND:
    return OS << "[Fn][NoUnwind  ]";
  case OCK_FUNCTION_MEM_EFFECTS:
    return OS << "[Fn][MemEffects]";
  case OCK_PARAMETER_RETURNED:
    return OS << "[Fn][Returned  ]";
  case OCK_CONTROL_FLOW_TRG:
    return OS << "[CFG][BranchTrg]";
  case OCK_MEMORY_ACCESS_ALIGN:
    return OS << "[Mem][Alignment]";
  case OCK_MEMORY_ACCESS_RES_ALIGN:
    return OS << "[Mem][ResAlign ]";
  case OCK_MEMORY_ACCESS_DEREFERENCEABLE:
    return OS << "[Mem][Dereferen]";
  case OCK_MEMORY_LOAD_INVARIANT:
    return OS << "[Ld ][Invariant]";
  case OCK_LOOP_PARALLEL_INNERMOST:
    return OS << "[Lop][ParallelI]";
  case OCK_LOOP_PARALLEL_OUTER:
    return OS << "[Lop][ParallelO]";
  case OCK_LOOP_BACKEDGE_TAKEN:
    return OS << "[Lop][BackEdgeT]";
  case OCK_LAST:
    return OS << "[N/A][LAST     ]";
  default:
    llvm_unreachable("unknown optimistic change type");
  }
  return OS;
}

static std::map<std::string, unsigned> NameCache[OCK_LAST];
static std::map<unsigned, SmallVector<unsigned, 32>>
    ChoiceMaps[OCK_LAST];
static DenseMap<const Function *, unsigned> ChoiceIndices[OCK_LAST];
static unsigned GlobalInstance = 0;
static DenseMap<const Function *, unsigned> FunctionNumerator;
static DenseMap<unsigned, const Function *> AllFunctions;
// static DenseSet<const Value *> SeenValues[OCK_LAST];

struct UnsoundAnnotator {

  // DenseSet<const Value *> NewValues[OCK_LAST];

  SmallPtrSet<const Function *, 16> FunctionDeclOnly;

  UnsoundAnnotator(GetterType<LoopInfo> &LIG, GetterType<ScalarEvolution> &SEG)
      : LIG(LIG), SEG(SEG), Instance(GlobalInstance++) {}

  void printOptimisticChoice(unsigned NumChoices, const Value &V,
                             Twine ChoiceStr, OptimisticChoiceKind Kind) {
    if (!PrintOptimisticChoices)
      return;

    dbgs() << "[OC][" << NumChoices << "][" << ChoiceStr << "]" << Kind << " @ "
           << V.getName() << " in " << getFnNameForValue(V) << "\n";
  }

  static void ensureName(Value &V) {
    if (V.hasName())
      return;

    if (Instruction *I = dyn_cast<Instruction>(&V))
      return I->setName("v");
    if (Argument *A = dyn_cast<Argument>(&V))
      return A->setName("a");

    V.dump();
    llvm_unreachable("TODO");
  }

  unsigned getOptimisticChoice(unsigned NumChoices, Value &V,
                               OptimisticChoiceKind Kind) {
    std::string VNameStr;
    if (!V.hasName()) {
      if (V.getType()->isVoidTy()) {
        if (auto *SI = dyn_cast<StoreInst>(&V)) {
          auto *Ptr = SI->getPointerOperand();
          ensureName(*Ptr);
          assert(Ptr->hasName());
          VNameStr = "__s." + Ptr->getName().str();
        } else {
          V.dump();
          assert(0 && "TODO add naming scheme!");
        }
      } else {
        ensureName(V);
        assert(V.hasName());
      }
    }
    if (VNameStr.empty())
      VNameStr = V.getName().str();

    auto &CachedResult = NameCache[Kind][VNameStr];
    const Function *F = getFnForValue(V);
    assert(F);
    if (!FunctionNumerator.count(F)) {
      unsigned Size = AllFunctions.size();
      AllFunctions[Size] = F;
      FunctionNumerator[F] = Size;
    }
    assert(FunctionNumerator.count(F));
    assert(AllFunctions[FunctionNumerator[F]] == F);

    std::string FNameStr = F->getName().str();
    assert(NumChoices > 0);
    unsigned Pos = ChoiceIndices[Kind][F]++;
    auto &Map = ChoiceMaps[Kind][FunctionNumerator[F]];
    // errs() << "Lookup " << F << " " << Kind << " Pos: "<< Pos <<" : " <<
    // Map.size() << "\n";
    if (Map.size() <= Pos)
      Map.resize(Pos + 1);

    auto &Choice = Map[Pos];
    // errs() << "Choice: " << Choice << " : CC: " << CachedResult << "\n";
    if (Choice > 0) {
      CachedResult = Choice;
    } else if (CachedResult > 0) {
      Choice = CachedResult;
    } else {
      // errs() << "Kind: " << Kind << " Default!\n";
      // assert(Instance + 1 == OptimisticAnnotationRuns &&
      //"Instance is not the last one but not completely specified!");
      Choice = OptimisticAnnotationsDefault ? NumChoices : 1;

      if ((OptimisticAnnotationRuns < 0 ||
           Instance == OptimisticAnnotationRuns - 1) &&
          PrintOptimisticOpportunities)
        dbgs() << "[OC][" << NumChoices << "][" << (Choice - 1) << "]["
               << (unsigned)Kind << "][" << FunctionNumerator[F] << "]" << Kind
               << " @ " << VNameStr << " in " << FNameStr << "\n";
    }

    return Choice - 1;
  }

  void searchAndReplace(std::string &str, const std::string &oldStr,
                        const std::string &newStr) {
    std::string::size_type pos = 0u;
    while ((pos = str.find(oldStr, pos)) != std::string::npos) {
      str.replace(pos, oldStr.length(), newStr);
      pos += newStr.length();
    }
  }

  template <typename FunctionsContainerTy>
  void initializeChoiceMaps(Module &M, FunctionsContainerTy &Functions) {
    for (Function &F : M) {
      if (!F.isIntrinsic() && (F.hasInternalLinkage() || F.isDeclaration()) &&
          F.getNumUses() == 0)
        continue;

      std::string FName = F.getName();
      searchAndReplace(FName, ".", "_");

      const std::string OCNameInC = (FName + "_OptimisticChoices");
      GlobalVariable *OptChoices = M.getGlobalVariable(OCNameInC, true);
      if (!OptChoices)
        OptChoices = M.getGlobalVariable(
            "_ZL" + std::to_string(OCNameInC.size()) + OCNameInC, true);
      if (!OptChoices && F.getName().equals("main"))
        OptChoices = M.getGlobalVariable("_ZZ4mainE18_OptimisticChoices", true);
      if (!OptChoices) {
        const std::string OCNameInCXX =
            ("_Z" + StringRef(FName).slice(1, -1) + "E18_OptimisticChoices")
                .str();
        OptChoices = M.getGlobalVariable(OCNameInCXX, true);
      }

      if (!OptChoices || OptChoices->getNumOperands() != 1)
        continue;

      if (auto *CE = dyn_cast<ConstantExpr>(OptChoices->getOperand(0)))
        OptChoices = dyn_cast<GlobalVariable>(CE->stripPointerCasts());
      if (!OptChoices)
        return;

      auto *DataArray =
          dyn_cast<ConstantDataArray>(OptChoices->getInitializer());
      if (!DataArray)
        return;

      Functions.insert(&F);
      if (!FunctionNumerator.count(&F)) {
        unsigned Size = AllFunctions.size();
        AllFunctions[Size] = &F;
        FunctionNumerator[&F] = Size;
      }

      OptimisticChoiceKind CurOCKind = OCK_FIRST;
      StringRef DataArrayString = DataArray->getAsString();
      if (DataArrayString.empty()) {
        continue;
      }

      if (Instance > 0)
        continue;

      for (unsigned ArrayPos = 0, e = DataArrayString.size(); ArrayPos + 1 < e;
           ArrayPos++) {
        char ControlChar = DataArrayString[ArrayPos];
        if (ControlChar == '#') {
          assert(ArrayPos + 2 < e);
          ControlChar = DataArrayString[++ArrayPos];
          if (ControlChar == 'c') {
            ControlChar = DataArrayString[++ArrayPos];
            assert(isValidControlChar(OCK_LAST, ControlChar));
            CurOCKind = getOptimisticChoiceKindForControlChar(ControlChar);
            continue;
          }
          llvm_unreachable("Unknown control char!");
        }
        assert(FunctionNumerator.count(&F));
        //errs() << "MAP: " << F.getName() << " [" << FunctionNumerator[&F]
               //<< "] " << CurOCKind << " : '" << ControlChar << "'\n";
        auto &Map = ChoiceMaps[CurOCKind][FunctionNumerator[&F]];
        Map.push_back(getControlCharChoice(ControlChar));
      }
    }

    if ((OptimisticAnnotationRuns <= 1) &&
        PrintOptimisticOpportunities && Instance == 0) {
      dbgs() << "Annotatable Functions [" << Instance << "][" << GlobalInstance
             << "]:\n";
      for (auto &It : AllFunctions)
        dbgs() << "no " << It.first << ": " << It.second->getName() << "\n";
    }

    if (Instance > 0)
      return;

    char Buffer[20];
    //Function *F = nullptr;
    int FnNo = -1;
    OptimisticChoiceKind CurOCKind = OCK_FIRST;
    for (unsigned ArrayPos = 0, e = OptimisticAnnotationsControlStr.size();
         ArrayPos < e; ArrayPos++) {
      char ControlChar = OptimisticAnnotationsControlStr[ArrayPos];
      //errs() << ArrayPos << " : " << ControlChar << " :\n";
      //<< (F ? F->getName() : "n/a") << " : " << CurOCKind << "\n";
      if (ControlChar == '#') {
        assert(ArrayPos + 2 < e);
        ControlChar = OptimisticAnnotationsControlStr[++ArrayPos];
        if (ControlChar == 'f') {
          int BufIdx = 0;
          ControlChar = OptimisticAnnotationsControlStr[++ArrayPos];
          assert(ControlChar >= '0' && ControlChar <= '9');
          do {
            assert(BufIdx != 20);
            Buffer[BufIdx++] = ControlChar;
            ControlChar = OptimisticAnnotationsControlStr[++ArrayPos];
          } while (ControlChar != 'f');
          Buffer[BufIdx] = '\0';
          //assert(isValidControlChar(AllFunctions.size(), ControlChar));
          FnNo = atoi(&Buffer[0]);//getControlCharChoice(ControlChar) - 1;
          assert(FnNo >= 0);
          //F = AllFunctions[FnNo - 1];
          continue;
        }
        if (ControlChar == 'c') {
          ControlChar = OptimisticAnnotationsControlStr[++ArrayPos];
          //errs() << "CC: " << ControlChar << " @ " << ArrayPos << "\n";
          assert(isValidControlChar(OCK_LAST, ControlChar));
          CurOCKind = getOptimisticChoiceKindForControlChar(ControlChar);
          continue;
        }
        llvm_unreachable("Unknown control char!");
      }
      assert(FnNo >= 0);
      //errs() << "MAP: " << FnNo << " "
             //<< (AllFunctions.count(FnNo) ? AllFunctions[FnNo]->getName()
                                          //: "n/a")
             //<< "[" << FnNo << "] " << CurOCKind << " : '" << ControlChar
             //<< "'\n";
      assert(CurOCKind > OCK_FIRST && CurOCKind < OCK_LAST);
      auto &Map = ChoiceMaps[CurOCKind][FnNo];
      Map.push_back(getControlCharChoice(ControlChar));
      // errs() << "Szie: " << Map.size() <<"\n";
    }
  }

  bool runOnModule(Module &M) {
    bool Changed = false;
    if ((unsigned)OptimisticAnnotationRuns <= Instance)
      return Changed;

    Intrinsic::getDeclaration(&M, Intrinsic::assume);
    //Intrinsic::getDeclaration(&M, Intrinsic::lifetime_start);
    //Intrinsic::getDeclaration(&M, Intrinsic::lifetime_end);

    SmallSetVector<Function *, 32> Functions;
    for (Function &F : M)
      if (shouldAnnotateFunction(F, OnlyFunctions)) {
        Functions.insert(&F);
        if (!FunctionNumerator.count(&F)) {
          unsigned Size = AllFunctions.size();
          AllFunctions[Size] = &F;
          FunctionNumerator[&F] = Size;
        }
        for (Instruction &I : instructions(F)) {
          CallSite CS(&I);
          if (CS)
            if (Function *Callee = CS.getCalledFunction())
              if (!shouldAnnotateFunction(*Callee, OnlyFunctions)) {
                FunctionDeclOnly.insert(Callee);
                Functions.insert(Callee);
              }
        }
      }

    initializeChoiceMaps(M, Functions);

    OptimisticChoiceKind Kind = OCK_FIRST;
    while ((Kind = getNextOptimisticChoiceKind(Kind)) != OCK_LAST) {
      for (Function *FPtr : Functions) {
        Function &F = *FPtr;

        DEBUG(errs() << "UnsoundAnnotator run on " << F.getName() << "\n");
        Changed |= annotateParameters(F, Kind);
        Changed |= annotateFunction(F, Kind);

        if (F.isDeclaration() || FunctionDeclOnly.count(&F))
          continue;

        LoopInfo &LI = *LIG(F);
        // ScalarEvolution &SE = *SEG(F);

        Changed |= annotateCalls(F, Kind);
        Changed |= annotateOverflowComputations(F, Kind);
        Changed |= annotateMemoryOperations(F, Kind, LI);
        Changed |= annotateBranchConditions(F, Kind, LI);
        Changed |= annotateLoops(F, Kind, LI);
      }
      Changed |= createSpeculativeAssumptions(M, Kind);
    }

    DEBUG(M.dump());
    if (verifyModule(M, &errs())) {
      M.dump();
      llvm_unreachable("Verification failed");
    }
    return Changed;
  }

private:
  bool annotateParameters(Function &F, OptimisticChoiceKind Kind) {
    if (Instance > 0)
      return false;
    const DataLayout &DL = F.getParent()->getDataLayout();
    LLVMContext &Ctx = F.getContext();
    for (Argument &Arg : F.args()) {
      if (!Arg.getType()->isPointerTy())
        continue;
      if (Arg.getNumUses() == 0)
        continue;
      if (Kind == OCK_PARAMETER_NO_CAPTURE &&
          !Arg.hasAttribute(Attribute::NoCapture)) {
        if (getOptimisticChoice(2, Arg, OCK_PARAMETER_NO_CAPTURE)) {
          printOptimisticChoice(2, Arg, "no-capture", OCK_PARAMETER_NO_CAPTURE);
          Arg.addAttr(Attribute::NoCapture);
          NumNoCaptureParameters++;
        }
      }
      if (Kind == OCK_PARAMETER_NO_ALIAS &&
          !Arg.hasAttribute(Attribute::NoAlias)) {
        if (getOptimisticChoice(2, Arg, OCK_PARAMETER_NO_ALIAS)) {
          printOptimisticChoice(2, Arg, "no-alias", OCK_PARAMETER_NO_ALIAS);
          Arg.addAttr(Attribute::NoAlias);
          NumNoAliasParameters++;
        }
      }
      unsigned TypeSize = DL.getPointerSize();
      if (Arg.getType()->getPointerElementType()->isSized())
        TypeSize = DL.getTypeAllocSize(Arg.getType()->getPointerElementType());
      if (Kind == OCK_PARAMETER_DEREFERENCEABLE &&
          !Arg.hasAttribute(Attribute::Dereferenceable)) {
        switch (getOptimisticChoice(3, Arg, OCK_PARAMETER_DEREFERENCEABLE)) {
        case 2:
          TypeSize *= 64;
          LLVM_FALLTHROUGH;
        case 1:
          printOptimisticChoice(
              3, Arg, "dereferenceable(" + std::to_string(TypeSize) + ")",
              OCK_PARAMETER_DEREFERENCEABLE);
          Arg.addAttr(
              Attribute::get(Ctx, Attribute::Dereferenceable, TypeSize));
          NumDereferenceableParameters++;
          break;
        case 0:
          break;
        }
      }
      if (Kind == OCK_PARAMETER_ALIGNED &&
          !Arg.hasAttribute(Attribute::Alignment)) {
        unsigned NewAlignment = 0;
        switch (getOptimisticChoice(3, Arg, OCK_PARAMETER_ALIGNED)) {
        case 0:
          NewAlignment = 0;
          break;
        case 1:
          NewAlignment = 8;
          break;
        case 2:
          NewAlignment = 64;
          break;
        default:
          break;
        }
        if (NewAlignment) {
          printOptimisticChoice(
              3, Arg, "param aligned(" + std::to_string(NewAlignment) + ")",
              OCK_PARAMETER_NO_CAPTURE);
          Arg.addAttr(Attribute::getWithAlignment(Ctx, NewAlignment));
          NumAlignedParameters++;
        }
      }
      if (Kind == OCK_PARAMETER_MEM_BEHAVIOR &&
          !Arg.hasAttribute(Attribute::ReadNone) &&
          !Arg.hasAttribute(Attribute::ReadOnly) &&
          !Arg.hasAttribute(Attribute::WriteOnly)) {
        switch (getOptimisticChoice(4, Arg, OCK_PARAMETER_MEM_BEHAVIOR)) {
        case 3:
          printOptimisticChoice(4, Arg, "param read-none",
                                OCK_PARAMETER_MEM_BEHAVIOR);
          Arg.addAttr(Attribute::get(Ctx, Attribute::ReadNone));
          NumReadNoneParameters++;
          break;
        case 2:
          printOptimisticChoice(4, Arg, "param read-only",
                                OCK_PARAMETER_MEM_BEHAVIOR);
          Arg.addAttr(Attribute::get(Ctx, Attribute::ReadOnly));
          NumReadOnlyParameters++;
          break;
        case 1:
          printOptimisticChoice(4, Arg, "param write-only",
                                OCK_PARAMETER_MEM_BEHAVIOR);
          Arg.addAttr(Attribute::get(Ctx, Attribute::WriteOnly));
          NumWriteOnlyParameters++;
          break;
        case 0:
          break;
        }
      }
    }

    return true;
  }

  bool annotateFunction(Function &F, OptimisticChoiceKind Kind) {
    if (Instance > 0)
      return false;
    if (F.getNumUses() == 0 && !F.isIntrinsic())
      return false;

    const DataLayout &DL = F.getParent()->getDataLayout();
    LLVMContext &Ctx = F.getContext();

    if (Kind == OCK_FUNCTION_INTERNAL && !F.isDeclaration() &&
        !F.hasInternalLinkage() && F.getNumUses()) {
      if (getOptimisticChoice(2, F, OCK_FUNCTION_INTERNAL)) {
        printOptimisticChoice(2, F, "internal", OCK_FUNCTION_INTERNAL);
        F.setLinkage(Function::InternalLinkage);
        NumFunctionsInternalized++;
      }
    }

    if (Kind == OCK_FUNCTION_NO_UNWIND && !F.doesNotThrow()) {
      if (getOptimisticChoice(2, F, OCK_FUNCTION_NO_UNWIND)) {
        printOptimisticChoice(2, F, "no-unwind", OCK_FUNCTION_NO_UNWIND);
        F.setDoesNotThrow();
        NumFunctionsNoUnwind++;
      }
    }

    bool IsSpeculatable = F.isSpeculatable();
    if (IsSpeculatable) {
      F.setDoesNotAccessMemory();
    }

    if (Kind == OCK_FUNCTION_MEM_EFFECTS) {
      bool HasAnnotatedRange = IsSpeculatable || F.doesNotAccessMemory() ||
                               F.onlyAccessesArgMemory() ||
                               F.onlyAccessesInaccessibleMemory() ||
                               F.onlyAccessesInaccessibleMemOrArgMem();
      unsigned NumChoices = 14;
      if (F.getName().equals("llvm.assume")) {
        NumChoices = 8;
        HasAnnotatedRange =
            F.onlyAccessesInaccessibleMemory() && F.doesNotReadMemory();
        // errs() << "ASSUME: " << F.onlyAccessesInaccessibleMemory() << " : "
        //<< F.doesNotReadMemory() << "\n";
        // if (!HasAnnotatedRange)
        // F.getParent()->dump();
      }

      if (!F.doesNotAccessMemory() && !HasAnnotatedRange) {
        unsigned Choice =
            getOptimisticChoice(NumChoices, F, OCK_FUNCTION_MEM_EFFECTS);
        if (Choice) {
          F.removeFnAttr(Attribute::WriteOnly);
          F.removeFnAttr(Attribute::ReadOnly);
        }
        switch (Choice) {
        case 13:
          printOptimisticChoice(NumChoices, F, "function speculative read-none",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setDoesNotAccessMemory();
          NumReadNoneFunctions++;
          F.setSpeculatable();
          NumFunctionsSpeculatable++;
          // LLVM_FALLTHROUGH;
          break;
        case 12:
          printOptimisticChoice(NumChoices, F, "function read-none",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setDoesNotAccessMemory();
          NumReadNoneFunctions++;
          break;
        case 11:
          printOptimisticChoice(NumChoices, F,
                                "function read-only inaccessible memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemory();
          F.setOnlyReadsMemory();
          NumFunctionsInaccessibleOnly++;
          NumReadOnlyFunctions++;
          break;
        case 10:
          printOptimisticChoice(NumChoices, F,
                                "function read-only argument memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesArgMemory();
          F.setOnlyReadsMemory();
          NumFunctionsArgMemOnly++;
          NumReadOnlyFunctions++;
          break;
        case 9:
          printOptimisticChoice(
              NumChoices, F,
              "function read-only inaccessible or argument memory",
              OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemOrArgMem();
          F.setOnlyReadsMemory();
          NumFunctionsArgMemOrInaccessibleOnly++;
          NumReadOnlyFunctions++;
          break;
        case 8:
          printOptimisticChoice(NumChoices, F, "function read-only",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyReadsMemory();
          NumReadOnlyFunctions++;
          break;
        case 7:
          printOptimisticChoice(NumChoices, F,
                                "function write-only inaccessible memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemory();
          F.setDoesNotReadMemory();
          NumFunctionsInaccessibleOnly++;
          NumWriteOnlyFunctions++;
          break;
        case 6:
          printOptimisticChoice(NumChoices, F,
                                "function write-only argument memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesArgMemory();
          F.setDoesNotReadMemory();
          NumFunctionsArgMemOnly++;
          NumWriteOnlyFunctions++;
          break;
        case 5:
          printOptimisticChoice(
              NumChoices, F,
              "function write-only inaccessible or argument memory",
              OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemOrArgMem();
          F.setDoesNotReadMemory();
          NumFunctionsArgMemOrInaccessibleOnly++;
          NumWriteOnlyFunctions++;
          break;
        case 4:
          printOptimisticChoice(NumChoices, F, "function write-only",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setDoesNotReadMemory();
          NumWriteOnlyFunctions++;
          break;
        case 3:
          printOptimisticChoice(NumChoices, F, "function inaccessible memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemory();
          NumFunctionsInaccessibleOnly++;
          break;
        case 2:
          printOptimisticChoice(NumChoices, F, "function argument memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesArgMemory();
          NumFunctionsArgMemOnly++;
          break;
        case 1:
          printOptimisticChoice(NumChoices, F,
                                "function inaccessible or argument memory",
                                OCK_FUNCTION_MEM_EFFECTS);
          F.setOnlyAccessesInaccessibleMemOrArgMem();
          NumFunctionsArgMemOrInaccessibleOnly++;
          break;
        case 0:
          break;
        default:
          llvm_unreachable("Unknown function memory effect!");
        }
      }
    }

    if (F.getReturnType()->isPointerTy()) {
      if (Kind == OCK_FUNCTION_RETURN_NO_ALIAS &&
          !F.hasAttribute(AttributeList::ReturnIndex, Attribute::NoAlias)) {
        if (getOptimisticChoice(2, F, OCK_FUNCTION_RETURN_NO_ALIAS)) {
          printOptimisticChoice(2, F, "function return no-alias",
                                OCK_FUNCTION_RETURN_NO_ALIAS);
          F.addAttribute(AttributeList::ReturnIndex, Attribute::NoAlias);
          NumFunctionsNoAliasReturn++;
        }
      }
      if (Kind == OCK_FUNCTION_RETURN_DEREFERENCEABLE &&
          !F.hasAttribute(AttributeList::ReturnIndex,
                          Attribute::Dereferenceable)) {
        unsigned TypeSize = DL.getPointerSize();
        if (F.getReturnType()->getPointerElementType()->isSized())
          TypeSize =
              DL.getTypeAllocSize(F.getReturnType()->getPointerElementType());
        switch (
            getOptimisticChoice(3, F, OCK_FUNCTION_RETURN_DEREFERENCEABLE)) {
        case 2:
          TypeSize *= 64;
          LLVM_FALLTHROUGH;
        case 1:
          printOptimisticChoice(3, F,
                                "function return dereferenceable(" +
                                    std::to_string(TypeSize) + ")",
                                OCK_FUNCTION_RETURN_DEREFERENCEABLE);
          F.addAttribute(
              AttributeList::ReturnIndex,
              Attribute::get(Ctx, Attribute::Dereferenceable, TypeSize));
          NumFunctionsDereferenceableReturn++;
          break;
        case 0:
          break;
        }
      }
    }

    Type *RetTy = F.getReturnType();
    SmallVector<Argument *, 4> PossibleReturnedArgs;
    for (Argument &Arg : F.args())
      if (Arg.getType() == RetTy)
        PossibleReturnedArgs.push_back(&Arg);
    if (Kind == OCK_PARAMETER_RETURNED && !PossibleReturnedArgs.empty() &&
        RetTy->isPointerTy()) {
      unsigned Decision = getOptimisticChoice(PossibleReturnedArgs.size() + 1,
                                              F, OCK_PARAMETER_RETURNED);
      if (Decision > 0) {
        assert(Decision <= PossibleReturnedArgs.size());
        Argument *ReturnedArg =
            PossibleReturnedArgs[PossibleReturnedArgs.size() - Decision];
        printOptimisticChoice(PossibleReturnedArgs.size() + 1, F,
                              "parameter returned: " + ReturnedArg->getName(),
                              OCK_PARAMETER_RETURNED);
        ReturnedArg->addAttr(Attribute::Returned);
        NumReturnedParameters++;
      }
    }

    return true;
  }

  bool annotateOverflowComputations(Function &F, OptimisticChoiceKind Kind) {

    for (Instruction &I : instructions(F)) {
      auto *GEP = dyn_cast<GetElementPtrInst>(&I);
      if (Kind == OCK_INBOUNDS_GEP && GEP && !GEP->isInBounds()) {
        if (getOptimisticChoice(2, I, OCK_INBOUNDS_GEP)) {
          printOptimisticChoice(2, I, "inbounds gep", OCK_INBOUNDS_GEP);
          GEP->setIsInBounds();
          NumInboundGEPs++;
        }
      }

      auto *OBO = dyn_cast<OverflowingBinaryOperator>(&I);
      if (!OBO)
        continue;
      if (Kind == OCK_OVERFLOW_NSW && !OBO->hasNoSignedWrap()) {
        if (getOptimisticChoice(2, I, OCK_OVERFLOW_NSW)) {
          printOptimisticChoice(2, I, "nsw binop", OCK_OVERFLOW_NSW);
          I.setHasNoSignedWrap();
          NumNoSignedWrapOps++;
        }
      }
      if (Kind == OCK_OVERFLOW_NUW && !OBO->hasNoUnsignedWrap()) {
        if (getOptimisticChoice(2, I, OCK_OVERFLOW_NUW)) {
          printOptimisticChoice(2, I, "nuw binop", OCK_OVERFLOW_NUW);
          I.setHasNoUnsignedWrap();
          NumNoUnsignedWrapOps++;
        }
      }
    }

    return true;
  }

  struct SpeculationInfo {
    SmallSetVector<Constant *, 4> Constants;
    SmallVector<std::pair<Value *, Instruction *>, 8> ValueInsertionPoints;
  };
  DenseMap<StringRef, SpeculationInfo> SpeculationMap;
  SmallVector<StringRef, 8> SpeculationMapOrder;

  bool annotateBranchConditions(Function &F, OptimisticChoiceKind Kind,
                                LoopInfo &LI) {
    if (Kind != OCK_CONTROL_FLOW_TRG)
      return false;

    const DataLayout &DL = F.getParent()->getDataLayout();

    for (BasicBlock &BB : F) {
      auto *TI = BB.getTerminator();
      if (Loop *L = LI.getLoopFor(&BB)) {
        if (L->isLoopLatch(&BB) || L->isLoopExiting(&BB))
          continue;
      }

      if (auto *BI = dyn_cast<BranchInst>(TI)) {
        if (BI->isUnconditional())
          continue;
        Value *BICond = BI->getCondition();
        auto *ICmpCond = dyn_cast<ICmpInst>(BICond);
        if (!ICmpCond || !ICmpCond->isEquality())
          continue;

        Value *Op0 = ICmpCond->getOperand(0);
        Value *Op1 = ICmpCond->getOperand(1);
        Constant *C0 = dyn_cast<Constant>(Op0);
        Constant *C1 = dyn_cast<Constant>(Op1);
        if ((C0 && C1) || (!C0 && !C1))
          continue;

        Constant *C = C1 ? C1 : C0;
        Type *T = C->getType();
        if (!T->isIntegerTy() && !T->isPointerTy() && !T->isFloatTy())
          continue;

        Value *V = C1 ? Op0->stripPointerCasts() : Op1->stripPointerCasts();
        if (!V || (!isa<LoadInst>(V) && !isa<Argument>(V) && !isa<CallInst>(V)))
          continue;

        StringRef IdString = V->getName();
        if (auto *Call = dyn_cast<CallInst>(V)) {
          if (auto *CF = Call->getCalledFunction())
            IdString = CF->getName();
        } else if (auto *Load = dyn_cast<LoadInst>(V)) {
          Value *Ptr = Load->getPointerOperand();
          Value *Obj = GetUnderlyingObject(Ptr, DL);
          if (Obj)
            IdString = Obj->getName();
        }

        Instruction *IP = isa<LoadInst>(V) || isa<CallInst>(V)
                              ? cast<Instruction>(V)->getNextNode()
                              : &*F.getEntryBlock().getFirstInsertionPt();

        SpeculationInfo &SI = SpeculationMap[IdString];
        if (SI.Constants.empty())
          SpeculationMapOrder.push_back(IdString);
        SI.Constants.insert(C);
        SI.ValueInsertionPoints.push_back({V, IP});

#if 0
      } else if (auto *SI = dyn_cast<SwitchInst>(TI)) {

        Value *SICond = SI->getCondition();
        Value *UnderlyingObj = GetUnderlyingObject(SICond, DL);
        if (!UnderlyingObj)
          continue;

        if (!isa<LoadInst>(UnderlyingObj) && !isa<Argument>(UnderlyingObj))
          continue;

        // TODO: Default case
        unsigned OptimisticChoice = getOptimisticChoice(
            SI->getNumSuccessors(), *SICond, OCK_CONTROL_FLOW_TRG);
        if (OptimisticChoice == 0)
          continue;

        Constant *CaseValue =
            (SI->case_begin() + (OptimisticChoice - 1))->getCaseValue();
        Instruction *CondResult =
            new ICmpInst(ICmpInst::ICMP_EQ, SICond, CaseValue);
        CondResult->insertBefore(SI);
        CallInst *Call = CallInst::Create(AssumeIntrinsic, {CondResult});
        Call->insertAfter(CondResult);

        NumConditionalBranchesEliminated++;
#endif
      }
    }

    return true;
  }

  bool createSpeculativeAssumptions(Module &M, OptimisticChoiceKind Kind) {
    if (Kind != OCK_CONTROL_FLOW_TRG)
      return false;

    LLVMContext &Ctx = M.getContext();
    Function *AssumeIntrinsic =
        Intrinsic::getDeclaration(&M, Intrinsic::assume);

    for (auto &IdString : SpeculationMapOrder) {
      SpeculationInfo &SI = SpeculationMap[IdString];
      assert(!SI.Constants.empty());
      assert(!SI.ValueInsertionPoints.empty());

      Value *Val = SI.ValueInsertionPoints.front().first;
      unsigned OptimisticChoice = getOptimisticChoice(
          SI.Constants.size() + 1, *Val, OCK_CONTROL_FLOW_TRG);
      if (OptimisticChoice == 0)
        continue;

      printOptimisticChoice(SI.Constants.size() + 1, *Val, "cfg speculation",
                            OCK_CONTROL_FLOW_TRG);

      for (auto &ValueInsertionPointPair : SI.ValueInsertionPoints) {
        Value *V = ValueInsertionPointPair.first;
        Instruction *IP = ValueInsertionPointPair.second;
        assert(OptimisticChoice > 0 && SI.Constants.size() > OptimisticChoice - 1);
        Constant *C = SI.Constants[OptimisticChoice - 1];
        if (C->getType() != V->getType()) {
          if (C->getType()->isIntegerTy()) {
            if (V->getType()->isIntegerTy())
              C = ConstantExpr::getIntegerCast(C, V->getType(), true);
            else if (V->getType()->isPointerTy())
              C = ConstantExpr::getIntToPtr(C, V->getType());
            else if (V->getType()->isFloatTy())
              C = ConstantExpr::getSIToFP(C, V->getType());
          } else if (C->getType()->isPointerTy()) {
            if (V->getType()->isIntegerTy())
              C = ConstantExpr::getPtrToInt(C, V->getType());
            else if (V->getType()->isPointerTy())
              C = ConstantExpr::getBitCast(C, V->getType());
            else if (V->getType()->isFloatTy())
              C = ConstantExpr::getSIToFP(
                  ConstantExpr::getPtrToInt(C, IntegerType::getInt64Ty(Ctx)),
                  V->getType());
          } else if (C->getType()->isFloatTy()) {
            if (V->getType()->isIntegerTy())
              C = ConstantExpr::getFPToSI(C, V->getType());
            else if (V->getType()->isPointerTy())
              C = ConstantExpr::getIntToPtr(
                  ConstantExpr::getFPToUI(C, IntegerType::getInt64Ty(Ctx)),
                  V->getType());
            else if (V->getType()->isFloatTy()) {
              if (V->getType()->getScalarSizeInBits() >
                  C->getType()->getScalarSizeInBits())
                C = ConstantExpr::getFPExtend(C, V->getType());
              else
                C = ConstantExpr::getFPTrunc(C, V->getType());
            }
          }
        }

        // errs() << It.getFirst() << " V: " << *V << " : " << *C << "\n";
        Instruction *ICmpCondResult = new ICmpInst(ICmpInst::ICMP_EQ, V, C);
        // ICmpCondResult->dump();

        ICmpCondResult->insertBefore(IP);
        CallInst *Call = CallInst::Create(AssumeIntrinsic, {ICmpCondResult});
        Call->insertBefore(IP);

        NumConditionalBranchesEliminated++;
      }
    }

    return !SpeculationMap.empty();
  }

  bool annotateMemoryOperations(Function &F, OptimisticChoiceKind Kind,
                                LoopInfo &LI) {
    const DataLayout &DL = F.getParent()->getDataLayout();
    LLVMContext &Ctx = F.getContext();
    Type *I64Ty = Type::getInt64Ty(Ctx);

    for (Instruction &I : instructions(F)) {
      Loop *L = LI.getLoopFor(I.getParent());
      if (auto *Load = dyn_cast<LoadInst>(&I)) {
        if (!Load->isSimple())
          continue;
        bool InvariantPtr = true;
        if (L && isa<Instruction>(Load->getPointerOperand()))
          InvariantPtr = L->isLoopInvariant(Load->getPointerOperand());

        if (Kind == OCK_MEMORY_LOAD_INVARIANT && InvariantPtr &&
            !Load->getMetadata("invariant.load")) {
          if (getOptimisticChoice(2, I, OCK_MEMORY_LOAD_INVARIANT)) {
            printOptimisticChoice(2, I, "invariant load",
                                  OCK_MEMORY_LOAD_INVARIANT);
            MDNode *MD = MDNode::get(Ctx, {});
            Load->setMetadata("invariant.load", MD);
            NumInvariantLoads++;
          }
        }
        if (Kind == OCK_MEMORY_ACCESS_ALIGN && L && Load->getAlignment() < 64) {
          unsigned NewAlignment = 0;
          switch (getOptimisticChoice(3, I, OCK_MEMORY_ACCESS_ALIGN)) {
          case 0:
            NewAlignment = 0;
            break;
          case 1:
            NewAlignment = 8;
            break;
          case 2:
            NewAlignment = 64;
            break;
          default:
            break;
          }
          if (Load->getAlignment() < NewAlignment) {
            printOptimisticChoice(
                3, I, "load aligned(" + std::to_string(NewAlignment) + ")",
                OCK_MEMORY_ACCESS_ALIGN);
            Load->setAlignment(NewAlignment);
            NumAlignedMemoryInst++;
          }
        }

        if (!Load->getType()->isPointerTy())
          continue;

        unsigned AccessSize = DL.getPointerSize();
        if (Load->getPointerOperand()->getType()->isSized())
          AccessSize =
              DL.getTypeAllocSize(Load->getPointerOperand()->getType());

        if (Kind == OCK_MEMORY_ACCESS_DEREFERENCEABLE &&
            !Load->getMetadata("dereferenceable")) {
          switch (
              getOptimisticChoice(3, I, OCK_MEMORY_ACCESS_DEREFERENCEABLE)) {
          case 2:
            AccessSize *= 64;
            LLVM_FALLTHROUGH;
          case 1: {
            printOptimisticChoice(3, I,
                                  "load dereferenceable(" +
                                      std::to_string(AccessSize) + ")",
                                  OCK_MEMORY_ACCESS_DEREFERENCEABLE);
            MDNode *MD = MDNode::get(
                Ctx, {ConstantAsMetadata::get(
                         ConstantInt::getSigned(I64Ty, AccessSize))});
            Load->setMetadata("dereferenceable", MD);
            NumDereferenceableMemoryInst++;
            break;
          }
          case 0:
            break;
          }
        }
        if (Kind == OCK_MEMORY_ACCESS_RES_ALIGN &&
            !Load->getMetadata("align")) {
          unsigned NewAlignment = 0;
          switch (getOptimisticChoice(3, I, OCK_MEMORY_ACCESS_RES_ALIGN)) {
          case 0:
            NewAlignment = 0;
            break;
          case 1:
            NewAlignment = 8;
            break;
          case 2:
            NewAlignment = 64;
            break;
          default:
            break;
          }
          if (NewAlignment) {
            printOptimisticChoice(3, I,
                                  "loaded ptr aligned(" +
                                      std::to_string(NewAlignment) + ")",
                                  OCK_MEMORY_ACCESS_RES_ALIGN);
            MDNode *MD = MDNode::get(
                Ctx, {ConstantAsMetadata::get(
                         ConstantInt::getSigned(I64Ty, NewAlignment))});
            Load->setMetadata("align", MD);
            NumAlignedMemoryInst++;
          }
        }
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        if (!SI->isSimple())
          continue;
        if (Kind == OCK_MEMORY_ACCESS_ALIGN && L && SI->getAlignment() < 64) {
          unsigned NewAlignment = 0;
          switch (getOptimisticChoice(3, I, OCK_MEMORY_ACCESS_ALIGN)) {
          case 0:
            NewAlignment = 0;
            break;
          case 1:
            NewAlignment = 8;
            break;
          case 2:
            NewAlignment = 64;
            break;
          default:
            break;
          }
          if (SI->getAlignment() < NewAlignment) {
            printOptimisticChoice(
                3, I, "store aligned(" + std::to_string(NewAlignment) + ")",
                OCK_MEMORY_ACCESS_ALIGN);
            SI->setAlignment(NewAlignment);
            NumAlignedMemoryInst++;
          }
        }
      }
    }

    return true;
  }

  bool annotateCalls(Function &F, OptimisticChoiceKind Kind) {
    bool Changed = false;
    for (Instruction &I : instructions(F)) {
      CallSite CS(&I);
      if (!CS)
        continue;
    }

    return Changed;
  }

  bool annotateLoops(Function &F, OptimisticChoiceKind Kind, LoopInfo &LI) {
    bool Changed = false;

    if (LI.empty())
      return Changed;

#if 0
    const DataLayout &DL = F.getParent()->getDataLayout();
    Function *AssumeIntrinsic =
        Intrinsic::getDeclaration(F.getParent(), Intrinsic::assume);
#endif

    LLVMContext &Ctx = F.getContext();
    for (Loop *OuterMostL : LI) {
      SmallPtrSet<Loop *, 4> ParallelLoops;

      for (Loop *L : depth_first(OuterMostL)) {

#if 0
        if (L->getLoopPredecessor() && SE.hasLoopInvariantBackedgeTakenCount(L)) {
          const SCEV *TripCount = SE.getBackedgeTakenCount(L);
          if (!SE.isKnownPositive(TripCount)) {
            if (const SCEVSMaxExpr *SMax = dyn_cast<SCEVSMaxExpr>(TripCount)) {
              if (SMax->getNumOperands() == 2 &&
                  (SMax->getOperand(0)->isZero() ||
                   SMax->getOperand(1)->isZero())) {
                TripCount = SMax->getOperand(0)->isZero() ? SMax->getOperand(1) : SMax->getOperand(0);
              }
            }
            if (getOptimisticChoice(2, *L->getHeader(), OCK_LOOP_BACKEDGE_TAKEN)) {
              Instruction *IP = L->getLoopPredecessor()->getTerminator();
              SCEVExpander Exp(SE, DL, "opt.ann.trip_count");
              Value *TripCountValue =
                  Exp.expandCodeFor(TripCount, TripCount->getType(), IP);
              Instruction *ICmpCondResult =
                  new ICmpInst(ICmpInst::ICMP_SGT, TripCountValue,
                               ConstantInt::getNullValue(TripCount->getType()));
              ICmpCondResult->insertBefore(IP);
              CallInst *Call = CallInst::Create(AssumeIntrinsic, {ICmpCondResult});
              Call->insertBefore(IP);

              Changed = true;
              NumBackedgeTakenLoops++;
            }
          }
        }
#endif

        if (Instance == 7) {
          OptimisticChoiceKind OCK = L->empty() ? OCK_LOOP_PARALLEL_INNERMOST
                                                : OCK_LOOP_PARALLEL_OUTER;
          if (Kind != OCK)
            continue;
          if (!getOptimisticChoice(2, *L->getHeader(), OCK))
            continue;

          printOptimisticChoice(2, *L->getHeader(), "parallel loop", OCK);

          Changed = true;
          if (OCK == OCK_LOOP_PARALLEL_INNERMOST)
            NumParallelInnermostLoops++;
          else
            NumParallelOuterLoops++;

          ParallelLoops.insert(L);
          if (L->getLoopID())
            continue;

          SmallVector<Metadata *, 4> MDs;
          // Reserve first location for self reference to the LoopID metadata
          // node.
          MDs.push_back(nullptr);

          MDNode *LoopID = MDNode::get(Ctx, MDs);
          // Set operand 0 to refer to the loop id itself.
          LoopID->replaceOperandWith(0, LoopID);
          L->setLoopID(LoopID);
        }
      }

      for (Loop *L : ParallelLoops) {
        SmallVector<Metadata *, 4> LoopIDs;
        Loop *PL = L;
        do {
          LoopIDs.push_back(PL->getLoopID());
          PL = PL->getParentLoop();
        } while (PL);

        for (BasicBlock *BB : L->blocks()) {
          Loop *BBL = LI.getLoopFor(BB);
          if (BBL != L && ParallelLoops.count(BBL))
            continue;
          for (Instruction &I : *BB) {
            if (!I.mayReadOrWriteMemory())
              continue;

            MDNode *ParMemMD = MDNode::get(Ctx, LoopIDs);
            MDNode *LoopIdMD =
                I.getMetadata(LLVMContext::MD_mem_parallel_loop_access);
            if (LoopIdMD)
              ParMemMD = ParMemMD->concatenate(ParMemMD, LoopIdMD);
            I.setMetadata(LLVMContext::MD_mem_parallel_loop_access, ParMemMD);
          }
        }
      }

      for (Loop *L : ParallelLoops) {
        if (!L->isAnnotatedParallel()) {
          F.getParent()->dump();
          errs() << "F: " << F.getName() << "\n";
          L->dump();
        }
        assert(L->isAnnotatedParallel());
      }
    }

    return Changed;
  }

  GetterType<LoopInfo> &LIG;
  GetterType<ScalarEvolution> &SEG;
  unsigned Instance;
};

#if 0
unsigned getOptimisticChoice(unsigned Idx, unsigned NumChoices,
                                   const Value &V,
                                   OptimisticChoiceKind OCKind) {
  assert(NumChoices > 0);


  if (OptimisticAnnotationsControlStr.size() <= OptimisticControlIdx) {
    if (!OptimisticAnnotationsDefaultOptimistic)
      return 0;

    return NumChoices -1;
  }

  char ControlChar = OptimisticAnnotationsControlStr[OptimisticControlIdx++];

  if (!isValidControlChar(NumChoices, ControlChar)) {
    errs() << OCKind << " @Pos: " << (OptimisticControlIdx - 1)
           << " CC: " << ControlChar << " NC: " << NumChoices << " V: " << V
           << " in " << getFnNameForValue(V)
           << " #OACS: " << OptimisticAnnotationsControlStr.size() << "\n";
    assert(false && "Control string for optimistic annotations is invalid!");
  }

  unsigned Choice = getControlCharChoice(ControlChar);
  return Choice;
}
#endif

//===----------------------------------------------------------------------===//
//
// Pass Manager integration code
//
//===----------------------------------------------------------------------===//
PreservedAnalyses UnsoundAnnotatorPass::run(Module &M,
                                            ModuleAnalysisManager &MAM) {
  auto &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  GetterType<LoopInfo> LoopInfoGetter = [&FAM](Function &F) {
    return &FAM.getResult<LoopAnalysis>(F);
  };
  GetterType<ScalarEvolution> ScalarEvolutionGetter = [&FAM](Function &F) {
    return &FAM.getResult<ScalarEvolutionAnalysis>(F);
  };
  UnsoundAnnotator UA(LoopInfoGetter, ScalarEvolutionGetter);
  UA.runOnModule(M);
  return PreservedAnalyses::none();
}

namespace {

struct UnsoundAnnotatorLegacyPass : public ModulePass {
  static char ID; // Pass identification, replacement for typeid

  UnsoundAnnotatorLegacyPass() : ModulePass(ID) {}

  bool doInitialization(Module &M) override { return false; }

  bool runOnModule(Module &M) override {
    if (skipModule(M))
      return false;

    GetterType<LoopInfo> LoopInfoGetter = [this](Function &F) {
      return &this->getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
    };
    GetterType<ScalarEvolution> ScalarEvolutionGetter = [this](Function &F) {
      return &this->getAnalysis<ScalarEvolutionWrapperPass>(F).getSE();
    };
    UnsoundAnnotator UA(LoopInfoGetter, ScalarEvolutionGetter);
    return UA.runOnModule(M);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequiredTransitive<ScalarEvolutionWrapperPass>();
    AU.addRequiredTransitive<LoopInfoWrapperPass>();
  }
};

} // end anonymous namespace

char UnsoundAnnotatorLegacyPass::ID = 0;

INITIALIZE_PASS_BEGIN(UnsoundAnnotatorLegacyPass, "unsound-annotator",
                      "Unsound annotator pass", false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolutionWrapperPass)
INITIALIZE_PASS_END(UnsoundAnnotatorLegacyPass, "unsound-annotator",
                    "Unsound annotator pass", false, false)

Pass *llvm::createUnsoundAnnotatorPass() {
  return new UnsoundAnnotatorLegacyPass();
}
