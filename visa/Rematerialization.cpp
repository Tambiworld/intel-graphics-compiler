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

#include "Rematerialization.h"

namespace vISA
{
    void Rematerialization::populateRefs()
    {
        unsigned int id = 0;
        for (auto bb : kernel.fg)
        {
            for (auto inst : *bb)
            {
                inst->setLexicalId(id++);

                if (inst->isPseudoKill())
                    continue;

                auto dst = inst->getDst();

                if (dst && !dst->isNullReg())
                {
                    auto topdcl = dst->getTopDcl();

                    if (topdcl)
                    {
                        operations[topdcl].def.push_back(std::make_pair(inst, bb));
                    }
                }

                for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
                {
                    auto srcOpnd = inst->getSrc(i);
                    if (srcOpnd &&
                        srcOpnd->isSrcRegRegion())
                    {
                        auto topdcl = srcOpnd->asSrcRegRegion()->getTopDcl();
                        unsigned int startRow = srcOpnd->getLeftBound() / G4_GRF_REG_NBYTES;
                        unsigned int endRow = srcOpnd->getRightBound() / G4_GRF_REG_NBYTES;
                        if (topdcl)
                        {
                            auto dclIt = operations.find(topdcl);
                            if (dclIt == operations.end())
                            {
                                References r;
                                r.numUses = 1;
                                for (unsigned int k = startRow; k <= endRow; k++)
                                {
                                    r.rowsUsed.insert(k);
                                }
                                //r.uses.push_back(std::make_pair(inst, bb));
                                operations.insert(std::make_pair(topdcl, r));
                            }
                            else
                            {
                                (*dclIt).second.numUses++;
                                for (unsigned int k = startRow; k <= endRow; k++)
                                {
                                    (*dclIt).second.rowsUsed.insert(k);
                                }
                                (*dclIt).second.lastUseLexId = inst->getLexicalId();
                                //(*dclIt).second.uses.push_back(std::make_pair(inst, bb));
                            }
                        }
                    }
                }
            }

            // Update lastUseLexId based on BB live-out set
            for (unsigned int i = 0; i < liveness.getNumSelectedVar(); i++)
            {
                if (bb->size() > 0 && liveness.isLiveAtExit(bb, i))
                {
                    auto lr = coloring.getLiveRanges()[i];
                    auto dclIt = operations.find(lr->getDcl()->getRootDeclare());
                    if (dclIt != operations.end())
                    {
                        (*dclIt).second.lastUseLexId = bb->back()->getLexicalId();
                    }
                }
            }
        }
    }

    void Rematerialization::populateSamplerHeaderMap()
    {
        samplerHeaderMapPopulated = true;

        if (!samplerHeader)
            return;

        for (auto bb : kernel.fg)
        {
            G4_INST* samplerHeaderMov = nullptr;
            for (auto inst : *bb)
            {
                if (inst->getDst() &&
                    inst->getDst()->getTopDcl() == samplerHeader)
                {
                    samplerHeaderMov = inst;
                    continue;
                }

                if (samplerHeaderMov &&
                    inst->isSplitSend() &&
                    inst->getMsgDesc()->getFuncId() == SFID::SAMPLER &&
                    inst->getMsgDesc()->isHeaderPresent())
                {
                    MUST_BE_TRUE(samplerHeaderMov->getExecSize() == 1, "Unexpected sampler header");
                    samplerHeaderMap.insert(std::make_pair(inst, samplerHeaderMov));
                }
            }
        }
    }

    void Rematerialization::deLVNSamplers(G4_BB* bb)
    {
        // LVN pass removes redundant samplerHeader movs. This way
        // several consecutive samplers can use same samplerHeader
        // instruction. However, when remat is done, extra care
        // needs to be taken so that all samplers still use same
        // header as before. Consider this snippet:
        //
        // samplerHeader(0,2) = a
        // send (16) ... samplerHeader ...
        // = V1
        // send (16) ... samplerHeader ...
        //
        // After remating V1:
        //
        // samplerHeader(0,2) = a
        // send (16) ... samplerHeader ...
        // samplerHeader(0,2) = b
        // send (16) REMAT_V1 samplerHeader ...
        // send (16) ... samplerHeader ... <-- Uses incorrect samplerHeader!
        //
        // This function deLVNs all samplerHeaders in the program and later
        // we LVN them back after remating is done. This ensures correctness.
        if (!samplerHeader)
            return;

        for (auto instIt = bb->begin();
            instIt != bb->end();
            )
        {
            auto inst = (*instIt);

            if (inst->isSplitSend() &&
                inst->getMsgDesc()->getFuncId() == SFID::SAMPLER)
            {
                auto samplerHeaderInstIt = samplerHeaderMap.find(inst);

                if (samplerHeaderInstIt != samplerHeaderMap.end())
                {
                    auto samplerHeaderMov = (*samplerHeaderInstIt).second;

                    auto dupOp = samplerHeaderMov->cloneInst();

                    bb->insert(instIt, dupOp);
                }
            }

            instIt++;
        }
    }

    bool Rematerialization::inSameSubroutine(G4_BB* use, G4_BB* def)
    {
        // Return true if both BBs belong to same sub
        auto defBBIt = BBPerSubroutine.find(def);
        auto useBBIt = BBPerSubroutine.find(use);

        // Neither BBs found in map means both are part of main kernel
        if (defBBIt == BBPerSubroutine.end() &&
            useBBIt == BBPerSubroutine.end())
            return true;

        if (defBBIt != BBPerSubroutine.end() &&
            useBBIt != BBPerSubroutine.end())
        {
            // Both BBs part of same subroutine
            if ((*defBBIt).second == (*useBBIt).second)
                return true;
        }

        // BBs not part of same subroutine
        return false;
    }

    // bb1 should block defining original computation and
    // bb2 should be the block where remat is expected.
    bool Rematerialization::areInSameLoop(G4_BB* bb1, G4_BB* bb2, bool& bb1OutsideLoop)
    {
        bool bb1InAnyLoop = false;
        bb1OutsideLoop = false;

        // Check whether bb1 is in any loop at all. If not,
        // then we can allow remat even if bb2 is in a loop.
        // The case that is disallowed is where bb1 and bb2
        // are both in loops, but in different ones.
        for (auto&& be : kernel.fg.backEdges)
        {
            auto loopIt = kernel.fg.naturalLoops.find(be);

            if (loopIt != kernel.fg.naturalLoops.end())
            {
                auto&& bbsInLoop = (*loopIt).second;

                auto bb1InLoop = bbsInLoop.find(bb1);
                if (bb1InLoop != bbsInLoop.end())
                {
                    bb1InAnyLoop = true;
                    break;
                }
            }
        }

        if (!bb1InAnyLoop)
            bb1OutsideLoop = true;

        for (auto&& be : kernel.fg.backEdges)
        {
            auto loopIt = kernel.fg.naturalLoops.find(be);

            if (loopIt != kernel.fg.naturalLoops.end())
            {
                auto&& bbsInLoop = (*loopIt).second;

                auto bb1InLoop = bbsInLoop.find(bb1);
                auto bb2InLoop = bbsInLoop.find(bb2);

                // Both BBs must be present in all nested loops
                if ((bb1InLoop == bbsInLoop.end() && bb2InLoop != bbsInLoop.end()) ||
                    (bb1InLoop != bbsInLoop.end() && bb2InLoop == bbsInLoop.end()))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool Rematerialization::isRangeSpilled(G4_Declare* dcl)
    {
        if (dcl)
            return dcl->isSpilled();

        return false;
    }

    bool Rematerialization::areAllDefsInBB(G4_Declare* dcl, G4_BB* bb, unsigned int lexId)
    {
        auto defsIt = operations.find(dcl);
        if (defsIt == operations.end())
            return false;

        auto&& refs = (*defsIt).second;
        // Each def must be in same BB as sampler header must appear lexically before sampler
        for (auto&& d : refs.def)
        {
            if (d.second != bb)
                return false;

            if (d.first->getLexicalId() > lexId)
                return false;
        }


        return true;
    }

    unsigned int Rematerialization::getLastUseLexId(G4_Declare* dcl)
    {
        unsigned int lastLexId = 0;
        auto it = operations.find(dcl);
        if (it != operations.end())
            lastLexId = (*it).second.lastUseLexId;

        return lastLexId;
    }

    void Rematerialization::cleanRedundantSamplerHeaders()
    {
        if (!samplerHeader)
            return;

        for (auto bb : kernel.fg)
        {
            std::list<G4_INST*> lastMov;

            INST_LIST_ITER toErase = bb->end();

            if (deLVNedBBs.find(bb) == deLVNedBBs.end())
                continue;

            for (auto instIt = bb->begin(), instItEnd = bb->end();
                instIt != instItEnd;
                )
            {
                auto inst = (*instIt);

                if (inst->isSplitSend() &&
                    inst->getMsgDesc()->getFuncId() == SFID::SAMPLER &&
                    inst->getMsgDesc()->isHeaderPresent())
                {
                    toErase = bb->end();
                }

                if (inst->isMov() && inst->getDst() && inst->getExecSize() == 1)
                {
                    // mov (1|NM) samplerHeader(0,2)<1>:ud   imm
                    auto dstTopDcl = inst->getDst()->getTopDcl();

                    if (dstTopDcl == samplerHeader)
                    {
                        if (toErase != bb->end())
                        {
                            lastMov.remove(*toErase);
                            bb->erase(toErase);
                            toErase = instIt;
                        }

                        if (lastMov.size() > 0)
                        {
                            auto lastMovSrc0 = lastMov.back()->getSrc(0);
                            auto instSrc0 = inst->getSrc(0);

                            if (inst->getDst()->getSubRegOff() == 2 &&
                                lastMovSrc0->isImm() == instSrc0->isImm() &&
                                lastMovSrc0->asImm()->getImm() == instSrc0->asImm()->getImm() &&
                                lastMovSrc0->getType() == instSrc0->getType())
                            {
                                // Remove current instruction
#if 0
                                printf("Removing sampler header mov at $%d\n", inst->getCISAOff());
#endif
                                instIt = bb->erase(instIt);
                                toErase = bb->end();
                                continue;
                            }
                        }

                        toErase = instIt;

                        lastMov.push_back(inst);
                    }
                }

                instIt++;
            }

            if (toErase != bb->end())
                bb->erase(toErase);
        }
    }

    bool Rematerialization::checkLocalWAR(G4_INST* defInst, G4_BB* bb, INST_LIST_ITER useIter)
    {
        INST_LIST_ITER currIter = useIter;
        while (currIter != bb->begin())
        {
            currIter--;
            auto currInst = *currIter;
            if (currInst == defInst)
                break;

            auto currDst = currInst->getDst();
            if (currDst && !currDst->isNullReg())
            {
                auto dstDcl = currDst->getTopDcl();
                unsigned int curLb = currDst->getLeftBound();
                unsigned int curRb = currDst->getRightBound();

                for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
                {
                    auto srcOpnd = defInst->getSrc(i);
                    if (srcOpnd &&
                        !(srcOpnd->isNullReg()) &&
                        srcOpnd->isSrcRegRegion())
                    {
                        G4_SrcRegRegion* srcRegion = srcOpnd->asSrcRegRegion();
                        auto srcDcl = srcRegion->getTopDcl();
                        unsigned int srcLb = srcRegion->getLeftBound(), srcRb = srcRegion->getRightBound();

                        if (dstDcl == srcDcl &&
                            curRb >= srcLb &&
                            curLb <= srcRb)
                        {
                            return false;
                        }
                    }
                }
            }
        }

        MUST_BE_TRUE(*currIter == defInst, "Cannot find defInst for Remat candidate!");

        return true;
    }

    bool Rematerialization::canRematerialize(G4_SrcRegRegion* src, G4_BB* bb, const Reference*& ref, INST_LIST_ITER instIter)
    {
        // op1 (8) A   B   C
        // ...
        // op2 (8) D   A   X
        //
        // This function will check whether rematerialize an operand,
        // eg A in op2 is possible.
        //
        auto topdcl = src->getTopDcl();
        if (!topdcl)
            return false;

        if (src->getInst()->isSplitIntrinsic())
            return false;

        // ADDRESS/FLAG spilled declare
        if (topdcl->getSpilledDeclare())
            return false;

        if (topdcl->getAddressed())
            return false;

        if (topdcl->getRegVar()->getPhyReg())
            return false;

        // Src must belong to GRF file
        if ((topdcl->getRegFile() &
            (G4_RegFileKind::G4_GRF | G4_RegFileKind::G4_INPUT)) == 0x0)
            return false;

        // Skip remat if src opnd uses special acc registers
        if (src->getAccRegSel() != ACC_UNDEFINED)
            return false;

        // Lookup defs of src in program
        auto opIt = operations.find(topdcl);
        if (opIt == operations.end())
            return false;

        auto&& refs = (*opIt).second;
        auto uniqueDef = findUniqueDef(refs, src);

        if (!uniqueDef)
            return false;

        // Def has a lot of uses so we will need lots of remat to make this profitable
        if (refs.numUses > MAX_USES_REMAT)
            return false;

        if (uniqueDef->first->getPredicate() ||
            uniqueDef->first->getCondMod())
            return false;

        ref = uniqueDef;

        // Check whether op1 can be recomputed
        auto srcInst = src->getInst();
        auto uniqueDefInst = uniqueDef->first;
        auto uniqueDefBB = uniqueDef->second;

        if (!isRematCandidateOp(uniqueDefInst))
            return false;

        unsigned int srcLexId = srcInst->getLexicalId();
        unsigned int origOpLexId = uniqueDefInst->getLexicalId();

        if (origOpLexId > srcLexId)
            return false;

        // Def-use must be far away
        if ((srcLexId - origOpLexId) < MIN_DEF_USE_DISTANCE)
            return false;

        if (!inSameSubroutine(bb, uniqueDefBB))
            return false;

        // If uniqueDefBB is not under SIMD CF, current BB is under SIMD CF
        // then we can remat only if def has NoMask option set.
        if (uniqueDefBB->isAllLaneActive() &&
            !bb->isAllLaneActive() &&
            !uniqueDefInst->isWriteEnableInst())
        {
            return false;
        }

        // Check whether they are in a loop. If yes, they should be in same loop.
        bool uniqueDefOutsideLoop = false;
        bool srcDclSpilled = isRangeSpilled(topdcl);
        bool inSameLoop = areInSameLoop(uniqueDefBB, bb, uniqueDefOutsideLoop);
        bool onlyUseInLoop = uniqueDefOutsideLoop && !inSameLoop;
        bool doNumRematCheck = false;

        // Decide whether it is profitable to push def inside loop before each use
        if (onlyUseInLoop && !srcDclSpilled)
        {
            // If topdcl does not interfere with other spilled
            // range then skip remating this operation.
            // Be less aggressive if this is SIMD8 since we run the
            // chance of perf penalty with this.
            if (kernel.getSimdSize() == 8 ||
                rematCandidates[topdcl->getRegVar()->getId()] == false ||
                rpe.getRegisterPressure(srcInst) < rematLoopRegPressure)
                return false;

            if (getNumRematsInLoop() > 0)
            {
                // Restrict non-SIMD1 remats to a low percent of loop instructions.
                float loopInstToTotalInstRatio = (float)getNumRematsInLoop() / (float)loopInstsBeforeRemat*100.0f;
                if (loopInstToTotalInstRatio > 1.75f)
                    return false;
            }
        }

        if (!inSameLoop)
        {
            if (!uniqueDefOutsideLoop)
                return false;
            else
            {
                // When op1 is outside loop and op2 is indside loop,
                // allow remat if op1 dst dcl is marked spilled.
                // Because that means a load will  be inserted in the
                // loop and remat might be more efficient here.
                if (!srcDclSpilled)
                {
                    // If src dcl is not spilled, check whether all
                    // src opnds of defInst have been remat'd atleast once.
                    // This heuristic helps decide if remat will be worthwhile
                    // in a loop.
                    doNumRematCheck = true;
                }
            }
        }

        if (inSameLoop && !uniqueDefOutsideLoop)
        {
            // Remat is done in loop only if declare
            // is marked as spill, so remat will
            // benefit it. Otherwise, if var has a
            // single use within the loop then remat
            // can be done as it doesnt contribute to
            // increase in inst count.
            if (!srcDclSpilled && refs.numUses > 1)
                return false;
        }

        // Check liveness of each src operand in original op
        bool srcLive[G4_MAX_SRCS];
        bool anySrcNotLive = false;
        for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
        {
            srcLive[i] = true;
            auto srcOpnd = uniqueDefInst->getSrc(i);
            if (!srcOpnd || srcOpnd->isImm() || srcOpnd->isNullReg())
                continue;

            if (srcOpnd->isSrcRegRegion())
            {
                // If src operand base is non-regvar (eg, architecture
                // register) then dont remat. Moving around such
                // registers could be dangerous.
                if (!srcOpnd->getBase()->isRegVar())
                    return false;

                // Check whether this src has a single unique def
                auto srcOpndRgn = srcOpnd->asSrcRegRegion();
                auto srcOpndTopDcl = srcOpndRgn->getTopDcl();

                if (doNumRematCheck && getNumRemats(srcOpndTopDcl) == 0)
                {
                    return false;
                }

                auto pointsToSet = liveness.getPointsToAnalysis().getIndrUseVectorPtrForBB(bb->getId());
                if (srcOpndTopDcl->getAddressed() &&
                    ((uniqueDefBB != bb) ||
                      std::find(pointsToSet->begin(), pointsToSet->end(), srcOpndTopDcl->getRegVar()) != pointsToSet->end()))
                {
                    // Indirectly addressed src opnd should not be extended
                    return false;
                }

                if ((srcOpndTopDcl->getRegFile() &
                    (G4_RegFileKind::G4_GRF | G4_RegFileKind::G4_INPUT)) == 0x0)
                    return false;

                // If an instruction has physical registers allocated then
                // dont optimize it.
                if (srcOpndRgn->getBase()->asRegVar()->getPhyReg() &&
                    !srcOpndTopDcl->isInput())
                    return false;

                if (srcOpndTopDcl->isInput())
                {
                    auto opIt = operations.find(srcOpndTopDcl);
                    if (opIt != operations.end())
                    {
                        // Check whether input variable has explicit def in function
                        if ((*opIt).second.def.size() > 0)
                            return false;
                    }

                    if (uniqueDefInst->getExecSize() == 1 &&
                        (*opIt).second.lastUseLexId < srcLexId)
                    {
                        // Skip remat if src is an input and exec size is 1.
                        // Inputs are pre-assigned and extending such ranges
                        // could lead to worse RA results, unless the input
                        // already extends beyond where we intend to remat.
                        return false;
                    }
                }

                // Run separate checks for sampler
                if (uniqueDefInst->isSplitSend() &&
                    uniqueDefInst->getMsgDesc()->getFuncId() == SFID::SAMPLER &&
                    uniqueDefInst->getSrc(2)->isImm() &&
                    uniqueDefInst->getSrc(3)->isImm())
                {
                    if (!kernel.getOptions()->getOption(vISA_cacheSamplerHeader))
                        return false;

                    // Sampler definition to be rematerialized
                    // sends (8) V54(0,0):f samplerHeader(0,0) V53(0,0) 0x42:ud 0x24a7002:ud{Align1, Q1}
                    // resLen = 4, msgLen = 1, extMsgLen = 1
                    // samplerHeader can be rematerialized as it is r0.0 with modified r0.2.
                    // V53 above will simply be extended since it requires extra computation to rematerialize.
                    // Above sampler inst has a header. Some sampler instructions may not have a header.
                    // For such headerless samplers we need to check whether it is profitable to extend
                    // both src operands.

                    // Ensure resLen > extMsgLen to make rematerialization profitable.
                    unsigned int len = uniqueDefInst->getMsgDesc()->extMessageLength();

                    // For Sanity, just verify V53 has defs before sampler send only.
                    auto extMsgOpnd = uniqueDefInst->getSrc(1);
                    MUST_BE_TRUE(extMsgOpnd->isSrcRegRegion() == true, "Unexpected src opnd for sampler");

                    // Dont remat if sampler def is outside loop and use inside loop
                    if (onlyUseInLoop)
                        return false;

                    if (!areAllDefsInBB(extMsgOpnd->asSrcRegRegion()->getTopDcl(), uniqueDefBB, uniqueDefInst->getLexicalId()))
                        return false;

                    bool samplerHeaderNotUsed = uniqueDefInst->getSrc(0)->asSrcRegRegion()->getTopDcl() != kernel.fg.builder->getBuiltinSamplerHeader();

                    if (!uniqueDefInst->getMsgDesc()->isHeaderPresent() ||
                        samplerHeaderNotUsed)
                    {
                        len += uniqueDefInst->getMsgDesc()->MessageLength();

                        auto msgOpnd = uniqueDefInst->getSrc(0);
                        if (!areAllDefsInBB(msgOpnd->asSrcRegRegion()->getTopDcl(), uniqueDefBB, uniqueDefInst->getLexicalId()))
                            return false;

                        if (liveness.isLiveAtExit(bb, msgOpnd->getTopDcl()->getRegVar()->getId()) ||
                            getLastUseLexId(msgOpnd->getTopDcl()) >= srcLexId)
                            len -= uniqueDefInst->getMsgDesc()->MessageLength();
                    }

                    if (samplerHeaderNotUsed)
                    {
                        // Ensure header creation instructions are used only by sampler
                        auto msgOpndTopDcl = uniqueDefInst->getSrc(0)->asSrcRegRegion()->getTopDcl();
                        auto topDclOpsIt = operations.find(msgOpndTopDcl);
                        if (topDclOpsIt == operations.end())
                            return false;

                        if ((*topDclOpsIt).second.numUses > 1)
                            return false;

                        for (auto& def : (*topDclOpsIt).second.def)
                        {
                            for (unsigned int i = 0; i != G4_MAX_SRCS; i++)
                            {
                                auto src = def.first->getSrc(i);
                                if (!src)
                                    continue;

                                if (src->isImm())
                                    continue;

                                if (src->isSrcRegRegion() &&
                                    (src->asSrcRegRegion()->getTopDcl() == kernel.fg.builder->getBuiltinSamplerHeader() ||
                                        src->asSrcRegRegion()->getTopDcl() == kernel.fg.builder->getBuiltinR0()))
                                    continue;

                                // Using some other var in payload src requires extra checks to remat, so skip it
                                return false;
                            }
                        }
                    }

                    if (liveness.isLiveAtExit(bb, extMsgOpnd->getTopDcl()->getRegVar()->getId()) ||
                        getLastUseLexId(extMsgOpnd->getTopDcl()) >= srcLexId)
                        len -= uniqueDefInst->getMsgDesc()->extMessageLength();

                    if (refs.rowsUsed.size() <= len)
                        return false;

                    return true;
                }
                else
                {
                    // Non-sampler definition to be rematerialized
                    if (uniqueDefInst->isSend())
                        return false;

                    auto opIt = operations.find(srcOpndTopDcl);
                    if (opIt == operations.end())
                        return false;

                    auto&& srcOpndRefs = (*opIt).second;
                    auto srcOpndUniqueDef = findUniqueDef(srcOpndRefs, srcOpndRgn);

                    bool isSrcAvailble = false;
                    if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) == VISA_CM &&
                        uniqueDefBB == bb)
                    {
                        isSrcAvailble = checkLocalWAR(uniqueDefInst, bb, instIter);
                    }

                    if (!srcOpndUniqueDef &&
                        !isSrcAvailble &&
                        !srcOpndTopDcl->isInput())
                        return false;

                    if (srcOpndUniqueDef &&
                        !inSameSubroutine(bb, srcOpndUniqueDef->second))
                        return false;

                    // Check if its live in/live out to/of current BB
                    unsigned int id = srcOpndTopDcl->getRegVar()->getId();
                    if (!liveness.isLiveAtExit(bb, id) &&
                        // Even if a var is not live-out, its live-range
                        // might extend till inst of interest.
                        srcOpndRefs.lastUseLexId < srcInst->getLexicalId())
                    {
                        // Opnd may not be live, but it is still possible to
                        // extend its live-range to remat it. For scalars, this
                        // could be profitable too.
                        srcLive[i] = false;
                        anySrcNotLive = true;
                    }
                }
            }
        }

        if (anySrcNotLive)
        {
            // Apply cost heuristic. It may be profitable to extend
            // scalars sometimes.
            for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
            {
                if (!srcLive[i])
                {
                    G4_SrcRegRegion* srcRgn = uniqueDefInst->getSrc(i)->asSrcRegRegion();

                    if (srcRgn->getTopDcl()->getNumElems() > 1)
                    {
                        // Extending non-scalar operands can be expensive
                        return false;
                    }
                }
            }
        }

        // Record remats in loop only for non-scalar operations. This is a heuristic used
        // to not remat excessively in loops.
        if (!inSameLoop &&
            uniqueDefInst->getExecSize() > 1)
            incNumRematsInLoop();

        if (cr0DefBB && IS_TYPE_FLOAT_ALL(uniqueDefInst->getExecType()))
        {
            return false;
        }

        return true;
    }

    G4_SrcRegRegion* Rematerialization::rematerialize(G4_SrcRegRegion* src, G4_BB* bb, const Reference* uniqueDef, std::list<G4_INST*>& newInst, G4_INST*& cacheInst)
    {
        // op1 (8) A   B   C
        // ...
        // op2 (8) D   A   E
        //
        // =>
        // op1 (8) A   B   C
        // ...
        // op1_dup (8) A1   B   C
        // op2 (8) D   A1   E

        G4_SrcRegRegion* rematSrc = nullptr;

        auto dstInst = uniqueDef->first;
        auto dst = dstInst->getDst();
        bool isSampler = dstInst->isSplitSend() && dstInst->getMsgDesc()->getFuncId() == SFID::SAMPLER;

        for (unsigned int i = 0; i < G4_MAX_SRCS; i++)
        {
            G4_Operand* src = dstInst->getSrc(i);
            if (src &&
                src->isSrcRegRegion())
            {
                incNumRemat(src->asSrcRegRegion()->getTopDcl());
            }
        }

        if (!isSampler)
        {
            unsigned int diffBound = dst->getRightBound() - (dst->getRegOff() * G4_GRF_REG_NBYTES);
            unsigned numElems = (diffBound + 1) / G4_Type_Table[dst->getType()].byteSize;
            auto newTemp = kernel.fg.builder->createTempVar(numElems, dst->getType(), Any, "REMAT_");
            newTemp->copyAlign(dst->getTopDcl());
            gra.copyAlignment(newTemp, dst->getTopDcl());
            G4_DstRegRegion* newDst = kernel.fg.builder->createDst(newTemp->getRegVar(), 0,
                (dst->getLeftBound() % G4_GRF_REG_NBYTES) / G4_Type_Table[dst->getType()].byteSize,
                dst->getHorzStride(), dst->getType());
            G4_INST* dupOp = nullptr;

            dupOp = dstInst->cloneInst();
            dupOp->setDest(newDst);

            dupOp->setLineNo(dstInst->getLineNo());
            dupOp->setCISAOff(dstInst->getCISAOff());

            rematSrc = createSrcRgn(src, dst, newTemp);

            newInst.push_back(dupOp);

            cacheInst = newInst.back();
        }
        else
        {
            G4_Operand* src0 = nullptr;
            // Look up samplerHeader(0,2) definition
            auto sampleHeaderTopDcl = uniqueDef->first->getSrc(0)->asSrcRegRegion()->getTopDcl();
            if (sampleHeaderTopDcl == kernel.fg.builder->getBuiltinSamplerHeader())
            {
                samplerHeader = sampleHeaderTopDcl;
                if (!samplerHeaderMapPopulated)
                {
                    populateSamplerHeaderMap();
                }

                if (deLVNedBBs.find(bb) == deLVNedBBs.end())
                {
                    // DeLVN one bb at a time when required
                    deLVNSamplers(bb);
                    deLVNedBBs.insert(bb);
                }

                auto samplerDefIt = samplerHeaderMap.find(uniqueDef->first);
                auto prevHeaderMov = (*samplerDefIt).second;

                src0 = dstInst->getSrc(0);

                // Duplicate sampler header setup instruction
                auto dupOp = prevHeaderMov->cloneInst();
                newInst.push_back(dupOp);
            }
            else
            {
                // Handle sampler when src0 is not builtin sampler header
                auto src0Rgn = uniqueDef->first->getSrc(0)->asSrcRegRegion();
                auto src0TopDcl = src0Rgn->getTopDcl();
                auto ops = operations.find(src0TopDcl);
                MUST_BE_TRUE(ops != operations.end(), "Didnt find record in map");
                MUST_BE_TRUE((*ops).second.numUses == 1, "Expecting src0 to be used only in sampler");

                auto newSrc0Dcl = kernel.fg.builder->createTempVar(src0TopDcl->getNumElems(),
                    src0TopDcl->getElemType(), gra.getSubRegAlign(src0TopDcl));

                // Clone all defining instructions for sampler's msg header
                for (unsigned int i = 0; i != (*ops).second.def.size(); i++)
                {
                    auto& headerDefInst = (*ops).second.def[i].first;

                    auto dupOp = headerDefInst->cloneInst();
                    auto headerDefDst = headerDefInst->getDst();
                    dupOp->setDest(kernel.fg.builder->createDstRegRegion(headerDefDst->getRegAccess(),
                        newSrc0Dcl->getRegVar(), headerDefDst->getRegOff(), headerDefDst->getSubRegOff(),
                        headerDefDst->getHorzStride(), headerDefDst->getType()));
                    newInst.push_back(dupOp);
                }

                auto rd = kernel.fg.builder->createRegionDesc(src0Rgn->getRegion()->vertStride,
                    src0Rgn->getRegion()->width, src0Rgn->getRegion()->horzStride);

                src0 = kernel.fg.builder->createSrcRegRegion(Mod_src_undef, Direct,
                    newSrc0Dcl->getRegVar(), src0Rgn->getRegOff(), src0Rgn->getSubRegOff(),
                    rd, src0Rgn->getType());
            }

            auto samplerDst = kernel.fg.builder->createTempVar(dst->getTopDcl()->getTotalElems(), dst->getTopDcl()->getElemType(),
                gra.getSubRegAlign(dst->getTopDcl()), "REMAT_SAMPLER_");
            auto samplerDstRgn = kernel.fg.builder->createDst(samplerDst->getRegVar(), 0,
                0, 1, samplerDst->getElemType());

            auto dstMsgDesc = dstInst->getMsgDesc();
            auto newMsgDesc = kernel.fg.builder->createGeneralMsgDesc(dstMsgDesc->getDesc(),
                dstMsgDesc->getExtendedDesc(), dstMsgDesc->getAccess(),
                kernel.fg.builder->duplicateOperand(dstMsgDesc->getBti()),
                kernel.fg.builder->duplicateOperand(dstMsgDesc->getSti()));

            auto dupOp = kernel.fg.builder->createSplitSendInst(nullptr, dstInst->opcode(), dstInst->getExecSize(), samplerDstRgn,
                kernel.fg.builder->duplicateOperand(src0)->asSrcRegRegion(),
                kernel.fg.builder->duplicateOperand(dstInst->getSrc(1))->asSrcRegRegion(),
                kernel.fg.builder->duplicateOperand(dstInst->asSendInst()->getMsgDescOperand()), dstInst->getOption(),
                newMsgDesc, kernel.fg.builder->duplicateOperand(dstInst->getSrc(3)), dstInst->getLineNo());
            dupOp->setLineNo(dstInst->getLineNo());
            dupOp->setCISAOff(dstInst->getCISAOff());

            newInst.push_back(dupOp);

            rematSrc = createSrcRgn(src, dst, samplerDst);

            cacheInst = newInst.back();
        }

        return rematSrc;
    }

    G4_SrcRegRegion* Rematerialization::createSrcRgn(G4_SrcRegRegion* srcToRemat, G4_DstRegRegion* uniqueDef, G4_Declare* rematTemp)
    {
        G4_SrcRegRegion* rematSrc = nullptr;

        unsigned row = (srcToRemat->getLeftBound() / G4_GRF_REG_NBYTES) - (uniqueDef->getLeftBound() / G4_GRF_REG_NBYTES);
        unsigned subReg = (srcToRemat->getLeftBound() % G4_GRF_REG_NBYTES) / G4_Type_Table[srcToRemat->getType()].byteSize;

        rematSrc = kernel.fg.builder->createSrcRegRegion(srcToRemat->getModifier(), Direct,
            rematTemp->getRegVar(), (short)row, (short)subReg, srcToRemat->getRegion(), srcToRemat->getType());

        return rematSrc;
    }

    const Reference* Rematerialization::findUniqueDef(References & refs, G4_SrcRegRegion *src)
    {
        // This function looks up list of definitions for a topdcl (src->getTopDcl()) and
        // returns a single dst region that defines that src region. If more than 1 def
        // match lb/rb of src then nullptr is returned. If a partial unique def is found
        // even then nullptr is returned.

        Reference* uniqueDef = nullptr;

        unsigned int lb = src->getLeftBound(), rb = src->getRightBound();
        for (auto&& r : refs.def)
        {
            auto curdst = r.first->getDst();
            unsigned int curlb = curdst->getLeftBound();
            unsigned int currb = curdst->getRightBound();

            if (curlb <= lb && currb >= rb)
            {
                if (uniqueDef)
                {
                    uniqueDef = nullptr;
                    break;
                }
                else
                {
                    uniqueDef = &r;
                }
            }
            else if ((curlb <= lb && currb >= lb) ||
                (curlb <= rb && currb >= lb))
            {
                // Partial overlap
                uniqueDef = nullptr;
                break;
            }
        }

        if (uniqueDef)
        {
            G4_RegFileKind rf = refs.def.front().first->getDst()->getTopDcl()->getRegFile();
            if (rf == G4_RegFileKind::G4_INPUT)
            {
                // Variable is an input as well as has a def
                uniqueDef = nullptr;
            }
        }

        return uniqueDef;
    }

    unsigned int getNumSamplers(G4_Kernel& kernel)
    {
        unsigned int numSampler = 0;

        for (auto bb : kernel.fg)
        {
            for (auto inst : *bb)
            {
                if (inst->isSplitSend() &&
                    inst->getMsgDesc()->getFuncId() == SFID::SAMPLER)
                {
                    numSampler++;
                }
            }
        }

        return numSampler;
    }

    void Rematerialization::run()
    {
        //unsigned int before = getNumSamplers(kernel);

        if (kernel.fg.builder->getOption(vISA_DumpDotAll))
        {
            kernel.dumpDotFile("before.remat");
        }

        populateRefs();

        auto firstProgInst = kernel.fg.getEntryBB()->getFirstInst();

        for (auto bb : kernel.fg)
        {
            if (kernel.getIntKernelAttribute(Attributes::ATTR_Target) == VISATarget::VISA_3D)
            {
                // For Cm, assume cr0 def is live across BBs
                // For IGC, assume cr0 is reset at each BB entry
                cr0DefBB = false;
            }
            // Store cache of rematerialized operations so nearby instructions
            // can reuse them.
            // <Unique def, <Remat'd def, Lexical id of last ref>>
            std::map<const Reference*, std::pair<G4_INST*, unsigned int>> rematValues;
            for (auto instIt = bb->begin();
                instIt != bb->end();
                instIt++)
            {
                auto inst = (*instIt);
                auto dst = inst->getDst();
                bool runRemat = false;

                cr0DefBB |= dst &&
                    dst->isCrReg() && (inst != firstProgInst);

                // Run remat if any src opnd is spilled
                for (unsigned int opnd = 0; opnd < G4_MAX_SRCS; opnd++)
                {
                    auto src = inst->getSrc(opnd);

                    if (src &&
                        src->isSrcRegRegion())
                    {
                        auto srcTopDcl = src->getTopDcl();
                        if (srcTopDcl && srcTopDcl->getRegVar()->isRegAllocPartaker() &&
                            (isRangeSpilled(srcTopDcl) ||
                            rematCandidates[srcTopDcl->getRegVar()->getId()] == true))
                        {
                            // Run remat for spilled src opnd even if
                            // register pressure is low.
                            runRemat = true;
                            break;
                        }
                    }
                }

                if (!runRemat)
                {
                    auto regPressure = rpe.getRegisterPressure(inst);

                    if (regPressure < rematRegPressure)
                    {
                        continue;
                    }
                }

                // High register pressure found at current instruction so try to remat
                for (unsigned int opnd = 0; opnd < G4_MAX_SRCS; opnd++)
                {
                    auto src = inst->getSrc(opnd);

                    if (src &&
                        src->isSrcRegRegion())
                    {
                        const Reference* uniqueDef = nullptr;
                        G4_SrcRegRegion* rematSrc = nullptr;

                        bool canRemat = canRematerialize(src->asSrcRegRegion(), bb, uniqueDef, instIt);
                        if (canRemat)
                        {
                            bool reUseRemat = false;
                            auto prevRematIt = rematValues.find(uniqueDef);
                            if (prevRematIt != rematValues.end())
                            {
                                if ((inst->getLexicalId() - (*prevRematIt).second.second) <=
                                    MAX_LOCAL_REMAT_REUSE_DISTANCE)
                                {
                                    reUseRemat = true;
                                    rematSrc = createSrcRgn(src->asSrcRegRegion(), uniqueDef->first->getDst(),
                                        (*prevRematIt).second.first->getDst()->getTopDcl());

                                    reduceNumUses(src->getTopDcl());

#if 0
                                    printf("Reusing rematerialized value %s in src%d of $%d from %s\n",
                                        src->getTopDcl()->getName(), opnd, inst->getCISAOff(),
                                        (*prevRematIt).second.first->getDst()->getTopDcl()->getName());
#endif
                                }
                                (*prevRematIt).second.second = inst->getLexicalId();
                            }

                            if (!reUseRemat)
                            {
#if 0
                                printf("Will rematerialize %s in src%d of $%d. Source computation at $%d\n",
                                    src->getTopDcl()->getName(), opnd, inst->getCISAOff(), uniqueDef->first->getCISAOff());
#endif
                                std::list<G4_INST*> newInsts;
                                G4_INST* cacheInst = nullptr;
                                rematSrc = rematerialize(src->asSrcRegRegion(), bb, uniqueDef, newInsts, cacheInst);
                                while (!newInsts.empty())
                                {
                                    bb->insert(instIt, newInsts.front());
                                    newInsts.pop_front();
                                }

                                rematValues.insert(std::make_pair(uniqueDef, std::make_pair(cacheInst, src->getInst()->getLexicalId())));

                                reduceNumUses(src->getTopDcl());

                                IRChanged = true;
                            }

                            inst->setSrc(rematSrc, opnd);
                        }
                    }
                }
            }
        }

        cleanRedundantSamplerHeaders();

        if (kernel.fg.builder->getOption(vISA_DumpDotAll))
        {
            kernel.dumpDotFile("after.remat");
        }

        //unsigned int after = getNumSamplers(kernel);
    }
}
