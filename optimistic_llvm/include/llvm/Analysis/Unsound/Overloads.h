//===- Analysis/Unsound/Overloads.h - Unsound Function Overloads -*- C++ -*-==//
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

#ifndef LLVM_ANALYSIS_UNSOUND_ANNOTATOR_H
#define LLVM_ANALYSIS_UNSOUND_ANNOTATOR_H

namespace llvm {
class CallInst;

bool isGuaranteedToTransferExecutionToSuccessor(const CallInst *I);

}

#endif
