add_llvm_library(LLVMUnsoundTransforms
  Annotator.cpp
  Config.cpp
  Overloads.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Transforms/Unsound

  DEPENDS
  intrinsics_gen
  )
