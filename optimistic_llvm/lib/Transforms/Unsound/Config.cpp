//===- Unsound/Config.cpp - TODO -------------------===//
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

#include "llvm/Transforms/Unsound/Config.h"

using namespace llvm;

void llvm::createUnsoundPasses(PassManagerBuilder::ExtensionPointTy Ty,
                               legacy::PassManagerBase &MPM) {
  MPM.add(createUnsoundAnnotatorPass());
}
