//===- Unsound/Config.h - Unsound/Optimistic configuration ------*- C++ -*-===//
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

#ifndef LLVM_UNSOUND_CONFIG_H
#define LLVM_UNSOUND_CONFIG_H

#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Unsound/Annotator.h"

namespace llvm {

void createUnsoundPasses(PassManagerBuilder::ExtensionPointTy Ty,
                         legacy::PassManagerBase &MPM);

inline void initializeUnsoundTransforms(PassRegistry &Registry) {
  initializeUnsoundAnnotatorLegacyPassPass(Registry);
}

} // namespace llvm

#endif
