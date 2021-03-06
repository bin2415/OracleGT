//===- llvm/MC/MCAsmBackend.h - MC Asm Backend ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCASMBACKEND_H
#define LLVM_MC_MCASMBACKEND_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCFragment.h"
#include <cstdint>
#include <memory>

namespace llvm {

class MCAsmLayout;
class MCAssembler;
class MCCFIInstruction;
class MCCodePadder;
struct MCFixupKindInfo;
class MCFragment;
class MCInst;
class MCObjectStreamer;
class MCObjectWriter;
struct MCCodePaddingContext;
class MCRelaxableFragment;
class MCSubtargetInfo;
class MCValue;
class raw_pwrite_stream;

/// Generic interface to target specific assembler backends.
class MCAsmBackend {
  std::unique_ptr<MCCodePadder> CodePadder;

protected: // Can only create subclasses.
  MCAsmBackend();
  MCAsmBackend(std::unique_ptr<MCCodePadder> TargetCodePadder);

public:
  MCAsmBackend(const MCAsmBackend &) = delete;
  MCAsmBackend &operator=(const MCAsmBackend &) = delete;
  virtual ~MCAsmBackend();

  /// lifetime management
  virtual void reset() {}

  /// Create a new MCObjectWriter instance for use by the assembler backend to
  /// emit the final object file.
  virtual std::unique_ptr<MCObjectWriter>
  createObjectWriter(raw_pwrite_stream &OS) const = 0;

  /// \name Target Fixup Interfaces
  /// @{

  /// Get the number of target specific fixup kinds.
  virtual unsigned getNumFixupKinds() const = 0;

  /// Map a relocation name used in .reloc to a fixup kind.
  virtual Optional<MCFixupKind> getFixupKind(StringRef Name) const;

  /// Get information on a fixup kind.
  virtual const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const;

  /// Hook to check if a relocation is needed for some target specific reason.
  virtual bool shouldForceRelocation(const MCAssembler &Asm,
                                     const MCFixup &Fixup,
                                     const MCValue &Target) {
    return false;
  }

  /// Apply the \p Value for given \p Fixup into the provided data fragment, at
  /// the offset specified by the fixup and following the fixup kind as
  /// appropriate. Errors (such as an out of range fixup value) should be
  /// reported via \p Ctx.
  virtual void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                          const MCValue &Target, MutableArrayRef<char> Data,
                          uint64_t Value, bool IsResolved) const = 0;

  // Koo: Make the function virual for the use
  //      All backend but x86 does not employee it (or implemented locally)
  //      PPCMachO and X86MachO might rewrite the following function
  virtual unsigned getFixupKindSize(unsigned Kind) const = 0;
  
  /// @}

  /// \name Target Relaxation Interfaces
  /// @{

  /// Check whether the given instruction may need relaxation.
  ///
  /// \param Inst - The instruction to test.
  virtual bool mayNeedRelaxation(const MCInst &Inst) const = 0;

  /// Target specific predicate for whether a given fixup requires the
  /// associated instruction to be relaxed.
  virtual bool fixupNeedsRelaxationAdvanced(const MCFixup &Fixup, bool Resolved,
                                            uint64_t Value,
                                            const MCRelaxableFragment *DF,
                                            const MCAsmLayout &Layout) const;

  /// Simple predicate for targets where !Resolved implies requiring relaxation
  virtual bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                                    const MCRelaxableFragment *DF,
                                    const MCAsmLayout &Layout) const = 0;

  /// Relax the instruction in the given fragment to the next wider instruction.
  ///
  /// \param Inst The instruction to relax, which may be the same as the
  /// output.
  /// \param STI the subtarget information for the associated instruction.
  /// \param [out] Res On return, the relaxed instruction.
  virtual void relaxInstruction(const MCInst &Inst, const MCSubtargetInfo &STI,
                                MCInst &Res) const = 0;

  /// @}

  /// Returns the minimum size of a nop in bytes on this target. The assembler
  /// will use this to emit excess padding in situations where the padding
  /// required for simple alignment would be less than the minimum nop size.
  ///
  virtual unsigned getMinimumNopSize() const { return 1; }

  /// Write an (optimal) nop sequence of Count bytes to the given output. If the
  /// target cannot generate such a sequence, it should return an error.
  ///
  /// \return - True on success.
  virtual bool writeNopData(uint64_t Count, MCObjectWriter *OW) const = 0;

  /// Give backend an opportunity to finish layout after relaxation
  virtual void finishLayout(MCAssembler const &Asm,
                            MCAsmLayout &Layout) const {}

  /// Handle any target-specific assembler flags. By default, do nothing.
  virtual void handleAssemblerFlag(MCAssemblerFlag Flag) {}

  /// \brief Generate the compact unwind encoding for the CFI instructions.
  virtual uint32_t
      generateCompactUnwindEncoding(ArrayRef<MCCFIInstruction>) const {
    return 0;
  }

  /// Handles all target related code padding when starting to write a new
  /// basic block to an object file.
  ///
  /// \param OS The streamer used for writing the padding data and function.
  /// \param Context the context of the padding, Embeds the basic block's
  /// parameters.
  void handleCodePaddingBasicBlockStart(MCObjectStreamer *OS,
                                        const MCCodePaddingContext &Context);
  /// Handles all target related code padding after writing a block to an object
  /// file.
  ///
  /// \param Context the context of the padding, Embeds the basic block's
  /// parameters.
  void handleCodePaddingBasicBlockEnd(const MCCodePaddingContext &Context);
  /// Handles all target related code padding before writing a new instruction
  /// to an object file.
  ///
  /// \param Inst the instruction.
  void handleCodePaddingInstructionBegin(const MCInst &Inst);
  /// Handles all target related code padding after writing an instruction to an
  /// object file.
  ///
  /// \param Inst the instruction.
  void handleCodePaddingInstructionEnd(const MCInst &Inst);

  /// Relaxes a fragment (changes the size of the padding) according to target
  /// requirements. The new size computation is done w.r.t a layout.
  ///
  /// \param PF The fragment to relax.
  /// \param Layout Code layout information.
  ///
  /// \returns true iff any relaxation occured.
  bool relaxFragment(MCPaddingFragment *PF, MCAsmLayout &Layout);
};

} // end namespace llvm

#endif // LLVM_MC_MCASMBACKEND_H
