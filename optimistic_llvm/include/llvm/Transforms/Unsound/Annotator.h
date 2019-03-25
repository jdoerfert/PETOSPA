//===- Unsound/Annotator.h - Unsound/Optimistic IR annotator ----*- C++ -*-===//
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

#ifndef LLVM_TRANSFORMS_UNSOUND_ANNOTATOR_H
#define LLVM_TRANSFORMS_UNSOUND_ANNOTATOR_H

#include "llvm/IR/PassManager.h"

#include "llvm/IR/LegacyPassManager.h"


namespace llvm {

class Function;

/// createUnsoundAnnotatorPass - TODO.
///
Pass *createUnsoundAnnotatorPass();

struct UnsoundAnnotatorPass : PassInfoMixin<UnsoundAnnotatorPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &);
};

void initializeUnsoundAnnotatorLegacyPassPass(PassRegistry &);

} // end namespace llvm

#endif
