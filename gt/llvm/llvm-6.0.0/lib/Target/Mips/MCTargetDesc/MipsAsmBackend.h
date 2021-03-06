//===-- MipsAsmBackend.h - Mips Asm Backend  ------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MipsAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#ifndef LLVM_LIB_TARGET_MIPS_MCTARGETDESC_MIPSASMBACKEND_H
#define LLVM_LIB_TARGET_MIPS_MCTARGETDESC_MIPSASMBACKEND_H

#include "MCTargetDesc/MipsFixupKinds.h"
#include "llvm/ADT/Triple.h"
#include "llvm/MC/MCAsmBackend.h"

namespace llvm {

class MCAssembler;
struct MCFixupKindInfo;
class MCObjectWriter;
class MCRegisterInfo;
class Target;

class MipsAsmBackend : public MCAsmBackend {
  Triple TheTriple;
  bool IsLittle; // Big or little endian
  bool IsN32;

public:
  MipsAsmBackend(const Target &T, const MCRegisterInfo &MRI, const Triple &TT,
                 StringRef CPU, bool N32)
      : TheTriple(TT), IsLittle(TT.isLittleEndian()), IsN32(N32) {}

  std::unique_ptr<MCObjectWriter>
  createObjectWriter(raw_pwrite_stream &OS) const override;

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved) const override;

  Optional<MCFixupKind> getFixupKind(StringRef Name) const override;
  const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

  unsigned getNumFixupKinds() const override {
    return Mips::NumTargetFixupKinds;
  }

  /// @name Target Relaxation Interfaces
  /// @{

  /// MayNeedRelaxation - Check whether the given instruction may need
  /// relaxation.
  ///
  /// \param Inst - The instruction to test.
  bool mayNeedRelaxation(const MCInst &Inst) const override {
    return false;
  }

  /// fixupNeedsRelaxation - Target specific predicate for whether a given
  /// fixup requires the associated instruction to be relaxed.
   bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                             const MCRelaxableFragment *DF,
                             const MCAsmLayout &Layout) const override {
    // FIXME.
    llvm_unreachable("RelaxInstruction() unimplemented");
    return false;
  }

  /// RelaxInstruction - Relax the instruction in the given fragment
  /// to the next wider instruction.
  ///
  /// \param Inst - The instruction to relax, which may be the same
  /// as the output.
  /// \param [out] Res On return, the relaxed instruction.
  void relaxInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                        MCInst &Res) const override {}

  /// @}

  bool writeNopData(uint64_t Count, MCObjectWriter *OW) const override;

  // unsigned getFixupKindSize(unsigned Kind) const override { return 0; } // Koo [N/A Here]
  unsigned getFixupKindSize(unsigned Kind) const override {
  //return (getFixupKindInfo((MCFixupKind)Kind.TargetSize + 7) / 8;
  unsigned FullSize = 1;
  switch ((unsigned)Kind) {
  case FK_Data_2:
  case Mips::fixup_Mips_16:
  case Mips::fixup_MICROMIPS_PC10_S1:
    FullSize = 2;
    break;
  case FK_Data_8:
  case Mips::fixup_Mips_64:
    FullSize = 8;
    break;
  case FK_Data_4:
  default:
    FullSize = 4;
    break;
  }
  return FullSize;
}

}; // class MipsAsmBackend

} // namespace

#endif
