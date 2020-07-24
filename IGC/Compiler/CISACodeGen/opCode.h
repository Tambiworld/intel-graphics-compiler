/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//opcodes definitions
//                      LLVM name                llvm types      name                     modifiers   sat     pred    condMod  mathIntrinsic  atomicIntrinsic  regioning
DECLARE_OPCODE(GenISA_sampleKillPix, GenISAIntrinsic, llvm_sample_killpix, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldptr, GenISAIntrinsic, llvm_ld_ptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_lodptr, GenISAIntrinsic, llvm_lodptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleptr, GenISAIntrinsic, llvm_sampleptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleBptr, GenISAIntrinsic, llvm_sample_bptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleCptr, GenISAIntrinsic, llvm_sample_cptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleDptr, GenISAIntrinsic, llvm_sample_dptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleDCptr, GenISAIntrinsic, llvm_sample_dcptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleLptr, GenISAIntrinsic, llvm_sample_lptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleLCptr, GenISAIntrinsic, llvm_sample_lcptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleBCptr, GenISAIntrinsic, llvm_sample_bcptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_gather4ptr, GenISAIntrinsic, llvm_gather4ptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_gather4Cptr, GenISAIntrinsic, llvm_gather4Cptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_gather4POptr, GenISAIntrinsic, llvm_gather4POptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_gather4POCptr, GenISAIntrinsic, llvm_gather4POCptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldmsptr, GenISAIntrinsic, llvm_ldmsptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldmsptr16bit, GenISAIntrinsic, llvm_ldmsptr16bit, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldmcsptr, GenISAIntrinsic, llvm_ldmcsptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_OUTPUT, GenISAIntrinsic, llvm_output, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_PatchConstantOutput, GenISAIntrinsic, llvm_PatchConstatntOutput, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_inputVec, GenISAIntrinsic, llvm_input, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_ShaderInputVec, GenISAIntrinsic, llvm_shaderinputvec, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_SystemValue, GenISAIntrinsic, llvm_sgv, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_RuntimeValue, GenISAIntrinsic, llvm_runtimeValue, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_discard, GenISAIntrinsic, llvm_discard, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_InitDiscardMask, GenISAIntrinsic, llvm_initdiscardmask, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_UpdateDiscardMask, GenISAIntrinsic, llvm_updatediscardmask, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_GetPixelMask, GenISAIntrinsic, llvm_getpixelmask, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_readsurfaceinfoptr, GenISAIntrinsic, llvm_surfaceinfo, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_resinfoptr, GenISAIntrinsic, llvm_resinfoptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sampleinfoptr, GenISAIntrinsic, llvm_sampleinfoptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_PullSampleIndexBarys, GenISAIntrinsic, llvm_pullSampleIndexBarys, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_PullSnappedBarys, GenISAIntrinsic, llvm_pullSnappedBarys, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_PullCentroidBarys,          GenISAIntrinsic,      llvm_pullCentroidBarys, false, false, false, false, false, false,  false  )
DECLARE_OPCODE(sqrt, Intrinsic, llvm_sqrt, true, true, true, false, true, false, false)
DECLARE_OPCODE(GenISA_rsq, GenISAIntrinsic, llvm_rsq, true, true, true, false, true, false, false)
DECLARE_OPCODE(FMul, Instruction, llvm_fmul, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_mul_rtz, GenISAIntrinsic, llvm_fmul_rtz, true, true, true, true, false, false, false)
DECLARE_OPCODE(FAdd, Instruction, llvm_fadd, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_add_rtz, GenISAIntrinsic, llvm_fadd_rtz, true, true, true, true, false, false, false)
DECLARE_OPCODE(FSub, Instruction, llvm_fsub, true, true, true, true, false, false, false)
DECLARE_OPCODE(FDiv, Instruction, llvm_fdiv, true, true, true, false, false, false, false)
DECLARE_OPCODE(FRem, Instruction, llvm_frem, false, false, false, false, false, false, false)
DECLARE_OPCODE(cos, Intrinsic, llvm_cos, true, true, true, false, true, false, false)
DECLARE_OPCODE(sin, Intrinsic, llvm_sin, true, true, true, false, true, false, false)
DECLARE_OPCODE(log2, Intrinsic, llvm_log, true, true, true, false, true, false, false)
DECLARE_OPCODE(exp2, Intrinsic, llvm_exp, true, true, true, false, true, false, false)
DECLARE_OPCODE(pow, Intrinsic, llvm_pow, true, true, true, false, true, false, false)
DECLARE_OPCODE(fma, Intrinsic, llvm_fma, true, true, true, false, false, false, false)
DECLARE_OPCODE(floor, Intrinsic, llvm_floor, true, true, true, true, true, false, false)
DECLARE_OPCODE(ceil, Intrinsic, llvm_ceil, true, true, true, true, true, false, false)
DECLARE_OPCODE(trunc, Intrinsic, llvm_round_z, true, true, true, true, true, false, false)
DECLARE_OPCODE(ctlz, Intrinsic, llvm_ctlz, false, false, true, false, true, false, false)
DECLARE_OPCODE(GenISA_ROUNDNE, GenISAIntrinsic, llvm_roundne, true, true, true, true, true, false, false)
DECLARE_OPCODE(InsertElement, Instruction, llvm_insert, true, false, false, false, false, false, false)
DECLARE_OPCODE(ExtractElement, Instruction, llvm_extract, false, false, false, false, false, false, false)
DECLARE_OPCODE(Select, Instruction, llvm_select, true, true, false, false, false, false, false)
DECLARE_OPCODE(Ret, Instruction, llvm_return, false, false, false, false, false, false, false)
DECLARE_OPCODE(FCmp, Instruction, llvm_fcmp, true, false, false, false, false, false, false)
DECLARE_OPCODE(Br, Instruction, llvm_branch, false, false, false, false, false, false, false)
DECLARE_OPCODE(PHI, Instruction, llvm_phi, false, false, false, false, false, false, false)
DECLARE_OPCODE(ICmp, Instruction, llvm_icmp, true, false, false, false, false, false, false)
DECLARE_OPCODE(Add, Instruction, llvm_add, true, true, true, true, false, false, true)
DECLARE_OPCODE(Sub, Instruction, llvm_sub, true, false, true, true, false, false, false)
DECLARE_OPCODE(UIToFP, Instruction, llvm_uitofp, false, true, true, true, false, false, false)
DECLARE_OPCODE(SIToFP, Instruction, llvm_sitofp, true, true, true, true, false, false, false)
DECLARE_OPCODE(FPToSI, Instruction, llvm_fptosi, true, false, true, true, false, false, false)
DECLARE_OPCODE(FPToUI, Instruction, llvm_fptoui, true, false, true, true, false, false, false)
DECLARE_OPCODE(Mul, Instruction, llvm_mul, true, false, true, true, false, false, false)
DECLARE_OPCODE(Xor, Instruction, llvm_xor, false, false, true, true, false, false, false)
DECLARE_OPCODE(Or, Instruction, llvm_or, false, false, true, true, false, false, false)
DECLARE_OPCODE(And, Instruction, llvm_and, false, false, true, true, false, false, false)
DECLARE_OPCODE(Shl, Instruction, llvm_shl, true, false, true, true, false, false, true)
DECLARE_OPCODE(AShr, Instruction, llvm_ishr, true, false, true, true, false, false, false)
DECLARE_OPCODE(LShr, Instruction, llvm_ushr, true, false, true, true, false, false, false)
DECLARE_OPCODE(GenISA_imulH, GenISAIntrinsic, llvm_imulh, false, false, true, true, true, false, false)
DECLARE_OPCODE(GenISA_umulH, GenISAIntrinsic, llvm_umulh, false, false, true, true, true, false, false)
DECLARE_OPCODE(minnum, Intrinsic, llvm_min, true, true, false, false, true, false, false)
DECLARE_OPCODE(maxnum, Intrinsic, llvm_max, true, true, false, false, true, false, false)
DECLARE_OPCODE(GenISA_GradientX, GenISAIntrinsic, llvm_gradientX, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_GradientY, GenISAIntrinsic, llvm_gradientY, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_GradientXfine, GenISAIntrinsic, llvm_gradientXfine, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_GradientYfine, GenISAIntrinsic, llvm_gradientYfine, true, true, true, true, false, false, false)
DECLARE_OPCODE(Load, Instruction, llvm_load, false, false, false, false, false, false, false)
DECLARE_OPCODE(Store, Instruction, llvm_store, false, false, false, false, false, false, false)
DECLARE_OPCODE(GetElementPtr, Instruction, llvm_getelementptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_GetBufferPtr, GenISAIntrinsic, llvm_getbufferptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(Call, Instruction, llvm_call, false, false, false, false, false, false, false)
DECLARE_OPCODE(SRem, Instruction, llvm_srem, true, false, false, false, false, false, false)
DECLARE_OPCODE(URem, Instruction, llvm_urem, true, false, false, false, false, false, false)
DECLARE_OPCODE(SExt, Instruction, llvm_sext, true, true, false, false, false, false, false)
DECLARE_OPCODE(ZExt, Instruction, llvm_zext, false, true, false, false, false, false, false)
DECLARE_OPCODE(Trunc, Instruction, llvm_trunc, true, false, false, false, false, false, false)
DECLARE_OPCODE(FPTrunc, Instruction, llvm_fptrunc, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ftof_rte, GenISAIntrinsic, llvm_fptrunc_rte, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ftof_rtz, GenISAIntrinsic, llvm_fptrunc_rtz, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ftof_rtp, GenISAIntrinsic, llvm_fptrunc_rtp, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ftof_rtn, GenISAIntrinsic, llvm_fptrunc_rtn, true, false, false, false, false, false, false)
DECLARE_OPCODE(BitCast, Instruction, llvm_bitcast, false, false, false, false, false, false, false)
DECLARE_OPCODE(PtrToInt, Instruction, llvm_ptrtoint, false, false, false, false, false, false, false)
DECLARE_OPCODE(IntToPtr, Instruction, llvm_inttoptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(SDiv, Instruction, llvm_sdiv, true, false, false, false, false, false, false)
DECLARE_OPCODE(UDiv, Instruction, llvm_udiv, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_uaddc, GenISAIntrinsic, llvm_uaddc, false, false, true, true, true, false, false)
DECLARE_OPCODE(GenISA_usubb, GenISAIntrinsic, llvm_usubb, false, false, true, true, true, false, false)
DECLARE_OPCODE(Alloca, Instruction, llvm_alloca, false, false, false, false, false, false, false)
DECLARE_OPCODE(FPExt, Instruction, llvm_fpext, true, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_f32tof16_rtz, GenISAIntrinsic, llvm_f32tof16_rtz, true, false, false, false, true, false, false)
DECLARE_OPCODE(fabs, Intrinsic, llvm_fabs, false, false, true, false, true, false, false)
DECLARE_OPCODE(GenISA_fsat, GenISAIntrinsic, llvm_fsat, true, false, true, true, true, false, false)
DECLARE_OPCODE(canonicalize, Intrinsic, llvm_canonicalize, true, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_dp4a_ss, GenISAIntrinsic, llvm_dp4a_ss, false, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_dp4a_uu, GenISAIntrinsic, llvm_dp4a_uu, false, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_dp4a_su, GenISAIntrinsic, llvm_dp4a_su, false, true, true, true, false, false, false)
DECLARE_OPCODE(GenISA_dp4a_us, GenISAIntrinsic, llvm_dp4a_us, false, true, true, true, false, false, false)

// GS Intrinsics
DECLARE_OPCODE(GenISA_OUTPUTGS, GenISAIntrinsic, llvm_output_gs, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_GsCutControlHeader, GenISAIntrinsic, llvm_gs_cut_control_header, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_GsStreamHeader, GenISAIntrinsic, llvm_gs_stream_header, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_GSinputVec, GenISAIntrinsic, llvm_gs_input, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_GSsystemValue, GenISAIntrinsic, llvm_gs_sgv, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_EndPrimitive, GenISAIntrinsic, llvm_gs_end_primitive, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_SetStream, GenISAIntrinsic, llvm_gs_set_stream, false, false, false, false, false, false, false)

// CS Intrinsics
DECLARE_OPCODE(GenISA_ldstructured, GenISAIntrinsic, llvm_ld_structured, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storestructured1, GenISAIntrinsic, llvm_store_structured1, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storestructured2, GenISAIntrinsic, llvm_store_structured2, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storestructured3, GenISAIntrinsic, llvm_store_structured3, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storestructured4, GenISAIntrinsic, llvm_store_structured4, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_typedread, GenISAIntrinsic, llvm_typed_read, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_typedwrite, GenISAIntrinsic, llvm_typed_write, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_threadgroupbarrier, GenISAIntrinsic, llvm_thread_group_barrier, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_memoryfence, GenISAIntrinsic, llvm_memory_fence, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_flushsampler, GenISAIntrinsic, llvm_flush_sampler, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_typedmemoryfence, GenISAIntrinsic, llvm_typed_memory_fence, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_cycleCounter, GenISAIntrinsic, llvm_cycleCounter, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_bfi, GenISAIntrinsic, llvm_bfi, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_ibfe, GenISAIntrinsic, llvm_ibfe, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_ubfe, GenISAIntrinsic, llvm_ubfe, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_bfrev, GenISAIntrinsic, llvm_bfrev, false, false, false, false, true, false, false)
DECLARE_OPCODE(ctpop, Intrinsic, llvm_cbit, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_firstbitHi, GenISAIntrinsic, llvm_fbh, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_firstbitLo, GenISAIntrinsic, llvm_fbl, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_firstbitShi, GenISAIntrinsic, llvm_fbh_shi, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_intatomicraw, GenISAIntrinsic, llvm_int_atomic_raw, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_dwordatomicstructured, GenISAIntrinsic, llvm_dword_atomic_structured, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_intatomictyped, GenISAIntrinsic, llvm_int_atomic_typed, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_icmpxchgatomicraw, GenISAIntrinsic, llvm_icmpxchg_atomic_raw, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_cmpxchgatomicstructured, GenISAIntrinsic, llvm_cmpxchg_atomic_structured, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_icmpxchgatomictyped, GenISAIntrinsic, llvm_icmpxchg_atomic_typed, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_atomiccounterinc, GenISAIntrinsic, llvm_atomic_counter_inc, false, false, false, false, false, true, false)
DECLARE_OPCODE(GenISA_atomiccounterpredec, GenISAIntrinsic, llvm_atomic_counter_predec, false, false, false, false, false, true, false)

DECLARE_OPCODE(GenISA_OWordPtr, GenISAIntrinsic, llvm_owordptr, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldraw_indexed, GenISAIntrinsic, llvm_ldraw_indexed, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ldrawvector_indexed, GenISAIntrinsic, llvm_ldrawvector_indexed, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storeraw_indexed, GenISAIntrinsic, llvm_storeraw_indexed, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_storerawvector_indexed, GenISAIntrinsic, llvm_storerawvector_indexed, false, false, false, false, false, false, false)

// Gen IR intrinsics
DECLARE_OPCODE(GenISA_URBWrite, GenISAIntrinsic, llvm_URBWrite, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_URBRead, GenISAIntrinsic, llvm_URBRead, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_RTWrite, GenISAIntrinsic, llvm_RTWrite, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_RTDualBlendSource, GenISAIntrinsic, llvm_dualRTWrite, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdLaneId, GenISAIntrinsic, llvm_simdLaneId, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdSize, GenISAIntrinsic, llvm_simdSize, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdShuffleDown, GenISAIntrinsic, llvm_simdShuffleDown, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdBlockRead, GenISAIntrinsic, llvm_simdBlockRead, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdBlockReadBindless, GenISAIntrinsic, llvm_simdBlockReadBindless, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdBlockWrite, GenISAIntrinsic, llvm_simdBlockWrite, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdBlockWriteBindless, GenISAIntrinsic, llvm_simdBlockWriteBindless, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdMediaBlockRead, GenISAIntrinsic, llvm_simdMediaBlockRead, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_simdMediaBlockWrite, GenISAIntrinsic, llvm_simdMediaBlockWrite, false, false, false, false, false, false, false)

// HS intrinsics
DECLARE_OPCODE(GenISA_OutputTessControlPoint, GenISAIntrinsic, llvm_output_hs, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_HSinputVec, GenISAIntrinsic, llvm_hs_input, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_HSPatchConstInputVec, GenISAIntrinsic, llvm_hsPatchConstinputvec, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_HSOutputCntrlPtInputVec, GenISAIntrinsic, llvm_hsOutputCntrlPtinputvec, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_HSURBPatchHeaderRead, GenISAIntrinsic, llvm_hsURBPatchHeaderRead, false, false, false, false, false, false, false)

// DS intrinsics
DECLARE_OPCODE(GenISA_DCL_DSPatchConstInputVec, GenISAIntrinsic, llvm_dsPatchConstinputvec, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_DCL_DSInputTessFactor, GenISAIntrinsic, llvm_dsInputTessFactor, false, false, false, false, false, false, false)

//PS intrinsics
DECLARE_OPCODE(GenISA_uavSerializeOnResID, GenISAIntrinsic, llvm_uavSerializeOnResID, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_uavSerializeAll, GenISAIntrinsic, llvm_uavSerializeAll, false, false, false, false, false, false, false)

// Message Phases manipulation
DECLARE_OPCODE(GenISA_createMessagePhases, GenISAIntrinsic, llvm_createMessagePhases, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_setMessagePhaseX, GenISAIntrinsic, llvm_setMessagePhaseX, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_setMessagePhaseXV, GenISAIntrinsic, llvm_setMessagePhaseXV, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_getMessagePhaseX, GenISAIntrinsic, llvm_getMessagePhaseX, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_getMessagePhaseXV, GenISAIntrinsic, llvm_getMessagePhaseXV, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_getMessagePhase, GenISAIntrinsic, llvm_getMessagePhase, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_setMessagePhase, GenISAIntrinsic, llvm_setMessagePhase, false, false, false, false, false, false, false)

// VME intrinsics
DECLARE_OPCODE(GenISA_vmeSendIME, GenISAIntrinsic, llvm_vmeSendIME, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vmeSendFBR, GenISAIntrinsic, llvm_vmeSendFBR, false, false, false, false, false, false, false)

// Correctly rounded intrinsics
DECLARE_OPCODE(GenISA_IEEE_Sqrt, GenISAIntrinsic, llvm_ieee_sqrt, false, false, false, false, true, false, false)
DECLARE_OPCODE(GenISA_IEEE_Divide, GenISAIntrinsic, llvm_ieee_divide, false, false, false, false, true, false, false)

// VA intrinsics
DECLARE_OPCODE(GenISA_vaErode, GenISAIntrinsic, llvm_vaErode, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaDilate, GenISAIntrinsic, llvm_vaDilate, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaMinMaxFilter, GenISAIntrinsic, llvm_vaMinMaxFilter, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaConvolve, GenISAIntrinsic, llvm_vaConvolve, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaConvolveGRF_16x1, GenISAIntrinsic, llvm_vaConvolveGRF_16x1, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaConvolveGRF_16x4, GenISAIntrinsic, llvm_vaConvolveGRF_16x4, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaMinMax, GenISAIntrinsic, llvm_vaMinMax, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaCentroid, GenISAIntrinsic, llvm_vaCentroid, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaBoolCentroid, GenISAIntrinsic, llvm_vaBoolCentroid, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_vaBoolSum, GenISAIntrinsic, llvm_vaBoolSum, false, false, false, false, false, false, false)

// 64-bit emulation
DECLARE_OPCODE(GenISA_add_pair, GenISAIntrinsic, llvm_add_pair, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_sub_pair, GenISAIntrinsic, llvm_sub_pair, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_mul_pair, GenISAIntrinsic, llvm_mul_pair, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_ptr_to_pair, GenISAIntrinsic, llvm_ptr_to_pair, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_pair_to_ptr, GenISAIntrinsic, llvm_pair_to_ptr, false, false, false, false, false, false, false)

// Wave intrinsics
DECLARE_OPCODE(GenISA_WaveBallot, GenISAIntrinsic, llvm_waveBallot, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_WaveAll, GenISAIntrinsic, llvm_waveAll, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_WaveClustered, GenISAIntrinsic, llvm_waveClustered, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_WavePrefix, GenISAIntrinsic, llvm_wavePrefix, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_QuadPrefix, GenISAIntrinsic, llvm_quadPrefix, false, false, false, false, false, false, false)
DECLARE_OPCODE(GenISA_WaveShuffleIndex, GenISAIntrinsic, llvm_waveShuffleIndex, false, false, false, false, false, false, false)

