//===-- Unsound/Overloads.cpp - Unsound/Optimistic Function Overloads -----===//
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

#include "llvm/Analysis/Unsound/Overloads.h"

#include <cassert>

using namespace llvm;

bool llvm::isGuaranteedToTransferExecutionToSuccessor(const CallInst *I) {
  assert(0);
}
