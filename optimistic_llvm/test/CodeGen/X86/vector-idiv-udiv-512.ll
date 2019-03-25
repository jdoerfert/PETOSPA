; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx512f | FileCheck %s --check-prefix=AVX --check-prefix=AVX512 --check-prefix=AVX512F
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx512bw | FileCheck %s --check-prefix=AVX --check-prefix=AVX512 --check-prefix=AVX512BW

;
; udiv by 7
;

define <8 x i64> @test_div7_8i64(<8 x i64> %a) nounwind {
; AVX-LABEL: test_div7_8i64:
; AVX:       # %bb.0:
; AVX-NEXT:    vextracti32x4 $3, %zmm0, %xmm1
; AVX-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX-NEXT:    movabsq $2635249153387078803, %rsi # imm = 0x2492492492492493
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vmovq %xmm1, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm1
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; AVX-NEXT:    vextracti32x4 $2, %zmm0, %xmm2
; AVX-NEXT:    vpextrq $1, %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm3[0]
; AVX-NEXT:    vinserti128 $1, %xmm1, %ymm2, %ymm1
; AVX-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX-NEXT:    vpextrq $1, %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm3[0]
; AVX-NEXT:    vpextrq $1, %xmm0, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm0, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    shrq %rcx
; AVX-NEXT:    addq %rdx, %rcx
; AVX-NEXT:    shrq $2, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm0
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm3[0]
; AVX-NEXT:    vinserti128 $1, %xmm2, %ymm0, %ymm0
; AVX-NEXT:    vinserti64x4 $1, %ymm1, %zmm0, %zmm0
; AVX-NEXT:    retq
  %res = udiv <8 x i64> %a, <i64 7, i64 7, i64 7, i64 7, i64 7, i64 7, i64 7, i64 7>
  ret <8 x i64> %res
}

define <16 x i32> @test_div7_16i32(<16 x i32> %a) nounwind {
; AVX-LABEL: test_div7_16i32:
; AVX:       # %bb.0:
; AVX-NEXT:    vpbroadcastd {{.*#+}} zmm1 = [613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757]
; AVX-NEXT:    vpmuludq %zmm1, %zmm0, %zmm2
; AVX-NEXT:    vpshufd {{.*#+}} zmm1 = zmm1[1,1,3,3,5,5,7,7,9,9,11,11,13,13,15,15]
; AVX-NEXT:    vpshufd {{.*#+}} zmm3 = zmm0[1,1,3,3,5,5,7,7,9,9,11,11,13,13,15,15]
; AVX-NEXT:    vpmuludq %zmm1, %zmm3, %zmm1
; AVX-NEXT:    vmovdqa32 {{.*#+}} zmm3 = [1,17,3,19,5,21,7,23,9,25,11,27,13,29,15,31]
; AVX-NEXT:    vpermi2d %zmm1, %zmm2, %zmm3
; AVX-NEXT:    vpsubd %zmm3, %zmm0, %zmm0
; AVX-NEXT:    vpsrld $1, %zmm0, %zmm0
; AVX-NEXT:    vpaddd %zmm3, %zmm0, %zmm0
; AVX-NEXT:    vpsrld $2, %zmm0, %zmm0
; AVX-NEXT:    retq
  %res = udiv <16 x i32> %a, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  ret <16 x i32> %res
}

define <32 x i16> @test_div7_32i16(<32 x i16> %a) nounwind {
; AVX512F-LABEL: test_div7_32i16:
; AVX512F:       # %bb.0:
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm2 = [9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363]
; AVX512F-NEXT:    vpmulhuw %ymm2, %ymm0, %ymm3
; AVX512F-NEXT:    vpsubw %ymm3, %ymm0, %ymm0
; AVX512F-NEXT:    vpsrlw $1, %ymm0, %ymm0
; AVX512F-NEXT:    vpaddw %ymm3, %ymm0, %ymm0
; AVX512F-NEXT:    vpsrlw $2, %ymm0, %ymm0
; AVX512F-NEXT:    vpmulhuw %ymm2, %ymm1, %ymm2
; AVX512F-NEXT:    vpsubw %ymm2, %ymm1, %ymm1
; AVX512F-NEXT:    vpsrlw $1, %ymm1, %ymm1
; AVX512F-NEXT:    vpaddw %ymm2, %ymm1, %ymm1
; AVX512F-NEXT:    vpsrlw $2, %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_div7_32i16:
; AVX512BW:       # %bb.0:
; AVX512BW-NEXT:    vpmulhuw {{.*}}(%rip), %zmm0, %zmm1
; AVX512BW-NEXT:    vpsubw %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpsrlw $1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpaddw %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpsrlw $2, %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %res = udiv <32 x i16> %a, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  ret <32 x i16> %res
}

define <64 x i8> @test_div7_64i8(<64 x i8> %a) nounwind {
; AVX512F-LABEL: test_div7_64i8:
; AVX512F:       # %bb.0:
; AVX512F-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm2 = xmm2[0],zero,xmm2[1],zero,xmm2[2],zero,xmm2[3],zero,xmm2[4],zero,xmm2[5],zero,xmm2[6],zero,xmm2[7],zero,xmm2[8],zero,xmm2[9],zero,xmm2[10],zero,xmm2[11],zero,xmm2[12],zero,xmm2[13],zero,xmm2[14],zero,xmm2[15],zero
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm3 = [37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37]
; AVX512F-NEXT:    vpmullw %ymm3, %ymm2, %ymm2
; AVX512F-NEXT:    vpsrlw $8, %ymm2, %ymm2
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm4 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero,xmm0[4],zero,xmm0[5],zero,xmm0[6],zero,xmm0[7],zero,xmm0[8],zero,xmm0[9],zero,xmm0[10],zero,xmm0[11],zero,xmm0[12],zero,xmm0[13],zero,xmm0[14],zero,xmm0[15],zero
; AVX512F-NEXT:    vpmullw %ymm3, %ymm4, %ymm4
; AVX512F-NEXT:    vpsrlw $8, %ymm4, %ymm4
; AVX512F-NEXT:    vperm2i128 {{.*#+}} ymm5 = ymm4[2,3],ymm2[2,3]
; AVX512F-NEXT:    vinserti128 $1, %xmm2, %ymm4, %ymm2
; AVX512F-NEXT:    vpackuswb %ymm5, %ymm2, %ymm2
; AVX512F-NEXT:    vpsubb %ymm2, %ymm0, %ymm0
; AVX512F-NEXT:    vpsrlw $1, %ymm0, %ymm0
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm4 = [127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127]
; AVX512F-NEXT:    vpand %ymm4, %ymm0, %ymm0
; AVX512F-NEXT:    vpaddb %ymm2, %ymm0, %ymm0
; AVX512F-NEXT:    vpsrlw $2, %ymm0, %ymm0
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm2 = [63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63]
; AVX512F-NEXT:    vpand %ymm2, %ymm0, %ymm0
; AVX512F-NEXT:    vextracti128 $1, %ymm1, %xmm5
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm5 = xmm5[0],zero,xmm5[1],zero,xmm5[2],zero,xmm5[3],zero,xmm5[4],zero,xmm5[5],zero,xmm5[6],zero,xmm5[7],zero,xmm5[8],zero,xmm5[9],zero,xmm5[10],zero,xmm5[11],zero,xmm5[12],zero,xmm5[13],zero,xmm5[14],zero,xmm5[15],zero
; AVX512F-NEXT:    vpmullw %ymm3, %ymm5, %ymm5
; AVX512F-NEXT:    vpsrlw $8, %ymm5, %ymm5
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm6 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero,xmm1[4],zero,xmm1[5],zero,xmm1[6],zero,xmm1[7],zero,xmm1[8],zero,xmm1[9],zero,xmm1[10],zero,xmm1[11],zero,xmm1[12],zero,xmm1[13],zero,xmm1[14],zero,xmm1[15],zero
; AVX512F-NEXT:    vpmullw %ymm3, %ymm6, %ymm3
; AVX512F-NEXT:    vpsrlw $8, %ymm3, %ymm3
; AVX512F-NEXT:    vperm2i128 {{.*#+}} ymm6 = ymm3[2,3],ymm5[2,3]
; AVX512F-NEXT:    vinserti128 $1, %xmm5, %ymm3, %ymm3
; AVX512F-NEXT:    vpackuswb %ymm6, %ymm3, %ymm3
; AVX512F-NEXT:    vpsubb %ymm3, %ymm1, %ymm1
; AVX512F-NEXT:    vpsrlw $1, %ymm1, %ymm1
; AVX512F-NEXT:    vpand %ymm4, %ymm1, %ymm1
; AVX512F-NEXT:    vpaddb %ymm3, %ymm1, %ymm1
; AVX512F-NEXT:    vpsrlw $2, %ymm1, %ymm1
; AVX512F-NEXT:    vpand %ymm2, %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_div7_64i8:
; AVX512BW:       # %bb.0:
; AVX512BW-NEXT:    vpmovzxbw {{.*#+}} zmm1 = ymm0[0],zero,ymm0[1],zero,ymm0[2],zero,ymm0[3],zero,ymm0[4],zero,ymm0[5],zero,ymm0[6],zero,ymm0[7],zero,ymm0[8],zero,ymm0[9],zero,ymm0[10],zero,ymm0[11],zero,ymm0[12],zero,ymm0[13],zero,ymm0[14],zero,ymm0[15],zero,ymm0[16],zero,ymm0[17],zero,ymm0[18],zero,ymm0[19],zero,ymm0[20],zero,ymm0[21],zero,ymm0[22],zero,ymm0[23],zero,ymm0[24],zero,ymm0[25],zero,ymm0[26],zero,ymm0[27],zero,ymm0[28],zero,ymm0[29],zero,ymm0[30],zero,ymm0[31],zero
; AVX512BW-NEXT:    vmovdqa64 {{.*#+}} zmm2 = [37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37]
; AVX512BW-NEXT:    vpmullw %zmm2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpsrlw $8, %zmm1, %zmm1
; AVX512BW-NEXT:    vpmovwb %zmm1, %ymm1
; AVX512BW-NEXT:    vextracti64x4 $1, %zmm0, %ymm3
; AVX512BW-NEXT:    vpmovzxbw {{.*#+}} zmm3 = ymm3[0],zero,ymm3[1],zero,ymm3[2],zero,ymm3[3],zero,ymm3[4],zero,ymm3[5],zero,ymm3[6],zero,ymm3[7],zero,ymm3[8],zero,ymm3[9],zero,ymm3[10],zero,ymm3[11],zero,ymm3[12],zero,ymm3[13],zero,ymm3[14],zero,ymm3[15],zero,ymm3[16],zero,ymm3[17],zero,ymm3[18],zero,ymm3[19],zero,ymm3[20],zero,ymm3[21],zero,ymm3[22],zero,ymm3[23],zero,ymm3[24],zero,ymm3[25],zero,ymm3[26],zero,ymm3[27],zero,ymm3[28],zero,ymm3[29],zero,ymm3[30],zero,ymm3[31],zero
; AVX512BW-NEXT:    vpmullw %zmm2, %zmm3, %zmm2
; AVX512BW-NEXT:    vpsrlw $8, %zmm2, %zmm2
; AVX512BW-NEXT:    vpmovwb %zmm2, %ymm2
; AVX512BW-NEXT:    vinserti64x4 $1, %ymm2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpsubb %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpsrlw $1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpandq {{.*}}(%rip), %zmm0, %zmm0
; AVX512BW-NEXT:    vpaddb %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    vpsrlw $2, %zmm0, %zmm0
; AVX512BW-NEXT:    vpandq {{.*}}(%rip), %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %res = udiv <64 x i8> %a, <i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7>
  ret <64 x i8> %res
}

;
; urem by 7
;

define <8 x i64> @test_rem7_8i64(<8 x i64> %a) nounwind {
; AVX-LABEL: test_rem7_8i64:
; AVX:       # %bb.0:
; AVX-NEXT:    vextracti32x4 $3, %zmm0, %xmm1
; AVX-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX-NEXT:    movabsq $2635249153387078803, %rsi # imm = 0x2492492492492493
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vmovq %xmm1, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm1
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; AVX-NEXT:    vextracti32x4 $2, %zmm0, %xmm2
; AVX-NEXT:    vpextrq $1, %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm3[0]
; AVX-NEXT:    vinserti128 $1, %xmm1, %ymm2, %ymm1
; AVX-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX-NEXT:    vpextrq $1, %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm2, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm2
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm3[0]
; AVX-NEXT:    vpextrq $1, %xmm0, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm3
; AVX-NEXT:    vmovq %xmm0, %rcx
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    mulq %rsi
; AVX-NEXT:    movq %rcx, %rax
; AVX-NEXT:    subq %rdx, %rax
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    addq %rdx, %rax
; AVX-NEXT:    shrq $2, %rax
; AVX-NEXT:    leaq (,%rax,8), %rdx
; AVX-NEXT:    subq %rax, %rdx
; AVX-NEXT:    subq %rdx, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm0
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm3[0]
; AVX-NEXT:    vinserti128 $1, %xmm2, %ymm0, %ymm0
; AVX-NEXT:    vinserti64x4 $1, %ymm1, %zmm0, %zmm0
; AVX-NEXT:    retq
  %res = urem <8 x i64> %a, <i64 7, i64 7, i64 7, i64 7, i64 7, i64 7, i64 7, i64 7>
  ret <8 x i64> %res
}

define <16 x i32> @test_rem7_16i32(<16 x i32> %a) nounwind {
; AVX-LABEL: test_rem7_16i32:
; AVX:       # %bb.0:
; AVX-NEXT:    vpbroadcastd {{.*#+}} zmm1 = [613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757,613566757]
; AVX-NEXT:    vpmuludq %zmm1, %zmm0, %zmm2
; AVX-NEXT:    vpshufd {{.*#+}} zmm1 = zmm1[1,1,3,3,5,5,7,7,9,9,11,11,13,13,15,15]
; AVX-NEXT:    vpshufd {{.*#+}} zmm3 = zmm0[1,1,3,3,5,5,7,7,9,9,11,11,13,13,15,15]
; AVX-NEXT:    vpmuludq %zmm1, %zmm3, %zmm1
; AVX-NEXT:    vmovdqa32 {{.*#+}} zmm3 = [1,17,3,19,5,21,7,23,9,25,11,27,13,29,15,31]
; AVX-NEXT:    vpermi2d %zmm1, %zmm2, %zmm3
; AVX-NEXT:    vpsubd %zmm3, %zmm0, %zmm1
; AVX-NEXT:    vpsrld $1, %zmm1, %zmm1
; AVX-NEXT:    vpaddd %zmm3, %zmm1, %zmm1
; AVX-NEXT:    vpsrld $2, %zmm1, %zmm1
; AVX-NEXT:    vpmulld {{.*}}(%rip){1to16}, %zmm1, %zmm1
; AVX-NEXT:    vpsubd %zmm1, %zmm0, %zmm0
; AVX-NEXT:    retq
  %res = urem <16 x i32> %a, <i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7, i32 7>
  ret <16 x i32> %res
}

define <32 x i16> @test_rem7_32i16(<32 x i16> %a) nounwind {
; AVX512F-LABEL: test_rem7_32i16:
; AVX512F:       # %bb.0:
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm2 = [9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363,9363]
; AVX512F-NEXT:    vpmulhuw %ymm2, %ymm0, %ymm3
; AVX512F-NEXT:    vpsubw %ymm3, %ymm0, %ymm4
; AVX512F-NEXT:    vpsrlw $1, %ymm4, %ymm4
; AVX512F-NEXT:    vpaddw %ymm3, %ymm4, %ymm3
; AVX512F-NEXT:    vpsrlw $2, %ymm3, %ymm3
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm4 = [7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7]
; AVX512F-NEXT:    vpmullw %ymm4, %ymm3, %ymm3
; AVX512F-NEXT:    vpsubw %ymm3, %ymm0, %ymm0
; AVX512F-NEXT:    vpmulhuw %ymm2, %ymm1, %ymm2
; AVX512F-NEXT:    vpsubw %ymm2, %ymm1, %ymm3
; AVX512F-NEXT:    vpsrlw $1, %ymm3, %ymm3
; AVX512F-NEXT:    vpaddw %ymm2, %ymm3, %ymm2
; AVX512F-NEXT:    vpsrlw $2, %ymm2, %ymm2
; AVX512F-NEXT:    vpmullw %ymm4, %ymm2, %ymm2
; AVX512F-NEXT:    vpsubw %ymm2, %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_rem7_32i16:
; AVX512BW:       # %bb.0:
; AVX512BW-NEXT:    vpmulhuw {{.*}}(%rip), %zmm0, %zmm1
; AVX512BW-NEXT:    vpsubw %zmm1, %zmm0, %zmm2
; AVX512BW-NEXT:    vpsrlw $1, %zmm2, %zmm2
; AVX512BW-NEXT:    vpaddw %zmm1, %zmm2, %zmm1
; AVX512BW-NEXT:    vpsrlw $2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpmullw {{.*}}(%rip), %zmm1, %zmm1
; AVX512BW-NEXT:    vpsubw %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %res = urem <32 x i16> %a, <i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7, i16 7>
  ret <32 x i16> %res
}

define <64 x i8> @test_rem7_64i8(<64 x i8> %a) nounwind {
; AVX512F-LABEL: test_rem7_64i8:
; AVX512F:       # %bb.0:
; AVX512F-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm3 = xmm2[0],zero,xmm2[1],zero,xmm2[2],zero,xmm2[3],zero,xmm2[4],zero,xmm2[5],zero,xmm2[6],zero,xmm2[7],zero,xmm2[8],zero,xmm2[9],zero,xmm2[10],zero,xmm2[11],zero,xmm2[12],zero,xmm2[13],zero,xmm2[14],zero,xmm2[15],zero
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm2 = [37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37]
; AVX512F-NEXT:    vpmullw %ymm2, %ymm3, %ymm3
; AVX512F-NEXT:    vpsrlw $8, %ymm3, %ymm3
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm4 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero,xmm0[4],zero,xmm0[5],zero,xmm0[6],zero,xmm0[7],zero,xmm0[8],zero,xmm0[9],zero,xmm0[10],zero,xmm0[11],zero,xmm0[12],zero,xmm0[13],zero,xmm0[14],zero,xmm0[15],zero
; AVX512F-NEXT:    vpmullw %ymm2, %ymm4, %ymm4
; AVX512F-NEXT:    vpsrlw $8, %ymm4, %ymm4
; AVX512F-NEXT:    vperm2i128 {{.*#+}} ymm5 = ymm4[2,3],ymm3[2,3]
; AVX512F-NEXT:    vinserti128 $1, %xmm3, %ymm4, %ymm3
; AVX512F-NEXT:    vpackuswb %ymm5, %ymm3, %ymm3
; AVX512F-NEXT:    vpsubb %ymm3, %ymm0, %ymm4
; AVX512F-NEXT:    vpsrlw $1, %ymm4, %ymm5
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm4 = [127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127]
; AVX512F-NEXT:    vpand %ymm4, %ymm5, %ymm5
; AVX512F-NEXT:    vpaddb %ymm3, %ymm5, %ymm3
; AVX512F-NEXT:    vpsrlw $2, %ymm3, %ymm3
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm5 = [63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63,63]
; AVX512F-NEXT:    vpand %ymm5, %ymm3, %ymm6
; AVX512F-NEXT:    vpmovsxbw %xmm6, %ymm7
; AVX512F-NEXT:    vmovdqa {{.*#+}} ymm3 = [7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7]
; AVX512F-NEXT:    vpmullw %ymm3, %ymm7, %ymm7
; AVX512F-NEXT:    vpmovsxwd %ymm7, %zmm7
; AVX512F-NEXT:    vpmovdb %zmm7, %xmm7
; AVX512F-NEXT:    vextracti128 $1, %ymm6, %xmm6
; AVX512F-NEXT:    vpmovsxbw %xmm6, %ymm6
; AVX512F-NEXT:    vpmullw %ymm3, %ymm6, %ymm6
; AVX512F-NEXT:    vpmovsxwd %ymm6, %zmm6
; AVX512F-NEXT:    vpmovdb %zmm6, %xmm6
; AVX512F-NEXT:    vinserti128 $1, %xmm6, %ymm7, %ymm6
; AVX512F-NEXT:    vpsubb %ymm6, %ymm0, %ymm0
; AVX512F-NEXT:    vextracti128 $1, %ymm1, %xmm6
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm6 = xmm6[0],zero,xmm6[1],zero,xmm6[2],zero,xmm6[3],zero,xmm6[4],zero,xmm6[5],zero,xmm6[6],zero,xmm6[7],zero,xmm6[8],zero,xmm6[9],zero,xmm6[10],zero,xmm6[11],zero,xmm6[12],zero,xmm6[13],zero,xmm6[14],zero,xmm6[15],zero
; AVX512F-NEXT:    vpmullw %ymm2, %ymm6, %ymm6
; AVX512F-NEXT:    vpsrlw $8, %ymm6, %ymm6
; AVX512F-NEXT:    vpmovzxbw {{.*#+}} ymm7 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero,xmm1[4],zero,xmm1[5],zero,xmm1[6],zero,xmm1[7],zero,xmm1[8],zero,xmm1[9],zero,xmm1[10],zero,xmm1[11],zero,xmm1[12],zero,xmm1[13],zero,xmm1[14],zero,xmm1[15],zero
; AVX512F-NEXT:    vpmullw %ymm2, %ymm7, %ymm2
; AVX512F-NEXT:    vpsrlw $8, %ymm2, %ymm2
; AVX512F-NEXT:    vperm2i128 {{.*#+}} ymm7 = ymm2[2,3],ymm6[2,3]
; AVX512F-NEXT:    vinserti128 $1, %xmm6, %ymm2, %ymm2
; AVX512F-NEXT:    vpackuswb %ymm7, %ymm2, %ymm2
; AVX512F-NEXT:    vpsubb %ymm2, %ymm1, %ymm6
; AVX512F-NEXT:    vpsrlw $1, %ymm6, %ymm6
; AVX512F-NEXT:    vpand %ymm4, %ymm6, %ymm4
; AVX512F-NEXT:    vpaddb %ymm2, %ymm4, %ymm2
; AVX512F-NEXT:    vpsrlw $2, %ymm2, %ymm2
; AVX512F-NEXT:    vpand %ymm5, %ymm2, %ymm2
; AVX512F-NEXT:    vpmovsxbw %xmm2, %ymm4
; AVX512F-NEXT:    vpmullw %ymm3, %ymm4, %ymm4
; AVX512F-NEXT:    vpmovsxwd %ymm4, %zmm4
; AVX512F-NEXT:    vpmovdb %zmm4, %xmm4
; AVX512F-NEXT:    vextracti128 $1, %ymm2, %xmm2
; AVX512F-NEXT:    vpmovsxbw %xmm2, %ymm2
; AVX512F-NEXT:    vpmullw %ymm3, %ymm2, %ymm2
; AVX512F-NEXT:    vpmovsxwd %ymm2, %zmm2
; AVX512F-NEXT:    vpmovdb %zmm2, %xmm2
; AVX512F-NEXT:    vinserti128 $1, %xmm2, %ymm4, %ymm2
; AVX512F-NEXT:    vpsubb %ymm2, %ymm1, %ymm1
; AVX512F-NEXT:    retq
;
; AVX512BW-LABEL: test_rem7_64i8:
; AVX512BW:       # %bb.0:
; AVX512BW-NEXT:    vpmovzxbw {{.*#+}} zmm1 = ymm0[0],zero,ymm0[1],zero,ymm0[2],zero,ymm0[3],zero,ymm0[4],zero,ymm0[5],zero,ymm0[6],zero,ymm0[7],zero,ymm0[8],zero,ymm0[9],zero,ymm0[10],zero,ymm0[11],zero,ymm0[12],zero,ymm0[13],zero,ymm0[14],zero,ymm0[15],zero,ymm0[16],zero,ymm0[17],zero,ymm0[18],zero,ymm0[19],zero,ymm0[20],zero,ymm0[21],zero,ymm0[22],zero,ymm0[23],zero,ymm0[24],zero,ymm0[25],zero,ymm0[26],zero,ymm0[27],zero,ymm0[28],zero,ymm0[29],zero,ymm0[30],zero,ymm0[31],zero
; AVX512BW-NEXT:    vmovdqa64 {{.*#+}} zmm2 = [37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37]
; AVX512BW-NEXT:    vpmullw %zmm2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpsrlw $8, %zmm1, %zmm1
; AVX512BW-NEXT:    vpmovwb %zmm1, %ymm1
; AVX512BW-NEXT:    vextracti64x4 $1, %zmm0, %ymm3
; AVX512BW-NEXT:    vpmovzxbw {{.*#+}} zmm3 = ymm3[0],zero,ymm3[1],zero,ymm3[2],zero,ymm3[3],zero,ymm3[4],zero,ymm3[5],zero,ymm3[6],zero,ymm3[7],zero,ymm3[8],zero,ymm3[9],zero,ymm3[10],zero,ymm3[11],zero,ymm3[12],zero,ymm3[13],zero,ymm3[14],zero,ymm3[15],zero,ymm3[16],zero,ymm3[17],zero,ymm3[18],zero,ymm3[19],zero,ymm3[20],zero,ymm3[21],zero,ymm3[22],zero,ymm3[23],zero,ymm3[24],zero,ymm3[25],zero,ymm3[26],zero,ymm3[27],zero,ymm3[28],zero,ymm3[29],zero,ymm3[30],zero,ymm3[31],zero
; AVX512BW-NEXT:    vpmullw %zmm2, %zmm3, %zmm2
; AVX512BW-NEXT:    vpsrlw $8, %zmm2, %zmm2
; AVX512BW-NEXT:    vpmovwb %zmm2, %ymm2
; AVX512BW-NEXT:    vinserti64x4 $1, %ymm2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpsubb %zmm1, %zmm0, %zmm2
; AVX512BW-NEXT:    vpsrlw $1, %zmm2, %zmm2
; AVX512BW-NEXT:    vpandq {{.*}}(%rip), %zmm2, %zmm2
; AVX512BW-NEXT:    vpaddb %zmm1, %zmm2, %zmm1
; AVX512BW-NEXT:    vpsrlw $2, %zmm1, %zmm1
; AVX512BW-NEXT:    vpandq {{.*}}(%rip), %zmm1, %zmm1
; AVX512BW-NEXT:    vpmovsxbw %ymm1, %zmm2
; AVX512BW-NEXT:    vmovdqa64 {{.*#+}} zmm3 = [7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7]
; AVX512BW-NEXT:    vpmullw %zmm3, %zmm2, %zmm2
; AVX512BW-NEXT:    vpmovwb %zmm2, %ymm2
; AVX512BW-NEXT:    vextracti64x4 $1, %zmm1, %ymm1
; AVX512BW-NEXT:    vpmovsxbw %ymm1, %zmm1
; AVX512BW-NEXT:    vpmullw %zmm3, %zmm1, %zmm1
; AVX512BW-NEXT:    vpmovwb %zmm1, %ymm1
; AVX512BW-NEXT:    vinserti64x4 $1, %ymm1, %zmm2, %zmm1
; AVX512BW-NEXT:    vpsubb %zmm1, %zmm0, %zmm0
; AVX512BW-NEXT:    retq
  %res = urem <64 x i8> %a, <i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7,i8 7, i8 7, i8 7, i8 7>
  ret <64 x i8> %res
}
