//===-- NVPTXMCExpr.cpp - NVPTX specific MC expression classes ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "NVPTXMCExpr.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Format.h"
using namespace llvm;

#define DEBUG_TYPE "nvptx-mcexpr"

const NVPTXFloatMCExpr *
NVPTXFloatMCExpr::create(VariantKind Kind, const APFloat &Flt, MCContext &Ctx) {
  return new (Ctx) NVPTXFloatMCExpr(Kind, Flt);
}

const MCExpr *NVPTXFloatMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  bool Ignored;
  unsigned NumHex;
  APFloat APF = getAPFloat();

  switch (Kind) {
  default: llvm_unreachable("Invalid kind!");
  case VK_NVPTX_HALF_PREC_FLOAT:
    // ptxas does not have a way to specify half-precision floats.
    // Instead we have to print and load fp16 constants as .b16
    OS << "0x";
    NumHex = 4;
    APF.convert(APFloat::IEEEhalf(), APFloat::rmNearestTiesToEven, &Ignored);
    break;
  case VK_NVPTX_SINGLE_PREC_FLOAT:
    OS << "0f";
    NumHex = 8;
    APF.convert(APFloat::IEEEsingle(), APFloat::rmNearestTiesToEven, &Ignored);
    break;
  case VK_NVPTX_DOUBLE_PREC_FLOAT:
    OS << "0d";
    NumHex = 16;
    APF.convert(APFloat::IEEEdouble(), APFloat::rmNearestTiesToEven, &Ignored);
    break;
  }

  APInt API = APF.bitcastToAPInt();
  OS << format_hex_no_prefix(API.getZExtValue(), NumHex, /*Upper=*/true);
  return NULL;
}

const NVPTXGenericMCSymbolRefExpr*
NVPTXGenericMCSymbolRefExpr::create(const MCSymbolRefExpr *SymExpr,
                                    MCContext &Ctx) {
  return new (Ctx) NVPTXGenericMCSymbolRefExpr(SymExpr);
}

const MCExpr *NVPTXGenericMCSymbolRefExpr::printImpl(raw_ostream &OS,
                                            const MCAsmInfo *MAI) const {
  OS << "generic(";
  SymExpr->print(OS, MAI);
  OS << ")";
  return NULL;
}
