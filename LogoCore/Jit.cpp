#include "pch.h"
#include "Jit.h"
#include "InstructionHandler.h"
#include "Turtle.h"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstddef>

#if defined(_M_X64) || defined(__x86_64__)
#define LOGOCPP_JIT_AVAILABLE 1
#include <windows.h>
#else
#define LOGOCPP_JIT_AVAILABLE 0
#endif

// C++ helper called from JIT code for the long-line (Bresenham) case of a
// move. The trig, rounding and sub-pixel stores are emitted inline by
// emitMove; only real line drawing goes through here.
extern "C" {
    __declspec(noinline) void jitMoveDraw(Turtle* t, int nx, int ny) { t->MoveTo(nx, ny); }
}

#if LOGOCPP_JIT_AVAILABLE

// ==================== TINY X64 EMITTER ====================
namespace {

struct Label {
    std::vector<uint32_t> uses;   // offsets of rel32 fields to patch
    uint32_t bound = UINT32_MAX;
};

class Emitter {
public:
    std::vector<uint8_t> code;

    // Constant pool (appended after all code; rip-relative references are
    // patched in finalize()). Avoids mov-rax-imm64/movq round trips for
    // every literal in the hot path.
    std::vector<uint64_t> pool;
    std::unordered_map<uint64_t, uint32_t> poolIndex;   // bits -> item offset
    std::vector<std::pair<uint32_t, uint32_t>> ripRefs; // (ref offset, item offset)

    uint32_t here() const { return static_cast<uint32_t>(code.size()); }

    void emit8(uint8_t b) { code.push_back(b); }
    void emit32(uint32_t v) {
        for (int i = 0; i < 4; ++i) { code.push_back(v & 0xFF); v >>= 8; }
    }
    void emit64(uint64_t v) {
        for (int i = 0; i < 8; ++i) { code.push_back(v & 0xFF); v >>= 8; }
    }
    void patch32(uint32_t at, uint32_t value) {
        std::memcpy(code.data() + at, &value, 4);
    }

    uint32_t poolAdd(double v) {
        uint64_t bits;
        std::memcpy(&bits, &v, 8);
        auto it = poolIndex.find(bits);
        if (it != poolIndex.end()) return it->second;
        uint32_t off = static_cast<uint32_t>(pool.size()) * 8;
        pool.push_back(bits);
        poolIndex[bits] = off;
        return off;
    }

    // xmm0 = [rip + pool item]       (movsd load of a literal)
    void loadConstRip(double v) { ripOp(0x10, v); }
    // xmm0 += [rip + item]
    void addConstRip(double v) { ripOp(0x58, v); }
    void subConstRip(double v) { ripOp(0x5C, v); }
    void mulConstRip(double v) { ripOp(0x59, v); }

    void finalize() {
        uint32_t poolBase = here();
        for (uint64_t bits : pool) emit64(bits);
        for (auto& [ref, item] : ripRefs)
            patch32(ref, poolBase + item - (ref + 4));
    }

    void useLabel(Label& L) { L.uses.push_back(here()); emit32(0); }
    void bind(Label& L) {
        L.bound = here();
        for (uint32_t at : L.uses)
            patch32(at, L.bound - (at + 4));
    }

    // --- stack ---
    void subRsp(uint32_t imm) {
        if (imm < 128) { emit8(0x48); emit8(0x83); emit8(0xEC); emit8((uint8_t)imm); }
        else { emit8(0x48); emit8(0x81); emit8(0xEC); emit32(imm); }
    }
    void addRsp(uint32_t imm) {
        if (imm < 128) { emit8(0x48); emit8(0x83); emit8(0xC4); emit8((uint8_t)imm); }
        else { emit8(0x48); emit8(0x81); emit8(0xC4); emit32(imm); }
    }

    // --- moves ---
    void movRaxImm(uint64_t v) { emit8(0x48); emit8(0xB8); emit64(v); }
    void movqXmm0Rax() { emit8(0x66); emit8(0x48); emit8(0x0F); emit8(0x6E); emit8(0xC0); }
    void movRbxRdx() { emit8(0x48); emit8(0x89); emit8(0xD3); }   // mov rbx, rdx
    void movRsiRcx() { emit8(0x48); emit8(0x89); emit8(0xCE); }   // mov rsi, rcx
    void movRcxRsi() { emit8(0x48); emit8(0x89); emit8(0xF1); }   // mov rcx, rsi
    void movRdxRbx() { emit8(0x48); emit8(0x89); emit8(0xDA); }   // mov rdx, rbx

    // xmm0 = [rbx + slot*8]   (global var load)
    void loadVar(int slot) {
        int d = slot * 8;
        emit8(0xF2); emit8(0x0F); emit8(0x10);
        if (d < 128) { emit8(0x43); emit8((uint8_t)d); }
        else { emit8(0x83); emit32((uint32_t)d); }
    }
    // [rbx + slot*8] = xmm0   (global var store)
    void storeVar(int slot) {
        int d = slot * 8;
        emit8(0xF2); emit8(0x0F); emit8(0x11);
        if (d < 128) { emit8(0x43); emit8((uint8_t)d); }
        else { emit8(0x83); emit32((uint32_t)d); }
    }
    // xmm0 = [rsp + d]   (expression spill slots, rsp-relative and balanced)
    void loadRsp(int d) {
        emit8(0xF2); emit8(0x0F); emit8(0x10); emit8(0x44); emit8(0x24); emit8((uint8_t)d);
    }
    // xmm0 = [rbp + disp32]   (function parameter load; disp32 is negative)
    void loadParam(int32_t d) {
        emit8(0xF2); emit8(0x0F); emit8(0x10); emit8(0x85); emit32((uint32_t)d);
    }
    // [rbp + disp32] = xmm{reg}
    void storeParamReg(int32_t d, int reg) {
        emit8(0xF2); emit8(0x0F); emit8(0x11);
        emit8(0x85 | (uint8_t)(reg << 3));
        emit32((uint32_t)d);
    }
    // [rsp + d] = xmm0
    void storeRsp(int d) {
        emit8(0xF2); emit8(0x0F); emit8(0x11); emit8(0x44); emit8(0x24); emit8((uint8_t)d);
    }
    // xmm1 = [rsp + d]
    void loadXmm1Rsp(int d) {
        emit8(0xF2); emit8(0x0F); emit8(0x10); emit8(0x4C); emit8(0x24); emit8((uint8_t)d);
    }

    // --- scalar float ---
    void addsdXmm0Xmm1() { emit8(0xF2); emit8(0x0F); emit8(0x58); emit8(0xC1); }
    void mulsdXmm0Xmm1() { emit8(0xF2); emit8(0x0F); emit8(0x59); emit8(0xC1); }
    void subsdXmm1Xmm0() { emit8(0xF2); emit8(0x0F); emit8(0x5C); emit8(0xC8); }
    void movapsXmm0Xmm1() { emit8(0x0F); emit8(0x28); emit8(0xC1); }
    // movaps xmm{dst}, xmm{src}
    void movapsXY(int dst, int src) { emit8(0x0F); emit8(0x28); emit8((uint8_t)(0xC0 | (dst << 3) | src)); }
    void comisdXmm1Xmm0() { emit8(0x66); emit8(0x0F); emit8(0x2F); emit8(0xC8); }
    // cvttsd2si edx, xmm0
    void cvttsd2siEdxXmm0() { emit8(0xF2); emit8(0x0F); emit8(0x2C); emit8(0xD0); }
    void movEaxImm(int32_t v) { emit8(0xB8); emit32((uint32_t)v); }

    // --- control flow ---
    void jmp(Label& L) { emit8(0xE9); useLabel(L); }
    void jcc(uint8_t op, Label& L) { emit8(0x0F); emit8(0x80 | op); useLabel(L); }
    void ret() { emit8(0xC3); }

    // --- integer ops on edx (angle) ---
    void movEaxEdx() { emit8(0x89); emit8(0xD0); }             // mov eax, edx
    void movEdxDwordRsi(int off) { emit8(0x8B); emit8(0x56); emit8((uint8_t)off); }
    void storeEdxToRsi(int off) { emit8(0x89); emit8(0x56); emit8((uint8_t)off); }
    void addEdxEax() { emit8(0x01); emit8(0xC2); }
    void subEdxEax() { emit8(0x29); emit8(0xC2); }
    void addEdxImm(int v) {
        if (-128 <= v && v <= 127) { emit8(0x83); emit8(0xC2); emit8((uint8_t)v); }
        else { emit8(0x81); emit8(0xC2); emit32((uint32_t)v); }
    }
    void subEdxImm(int v) {
        if (-128 <= v && v <= 127) { emit8(0x83); emit8(0xEA); emit8((uint8_t)v); }
        else { emit8(0x81); emit8(0xEA); emit32((uint32_t)v); }
    }

    // --- inline move (Forward/Backward) support ---
    // eax-side arithmetic
    void movEaxDwordRsi(int off) { emit8(0x8B); emit8(0x46); emit8((uint8_t)off); }   // eax = [rsi+off]
    void addEaxImm(int v) { emit8(0x05); emit32((uint32_t)v); }                       // eax += imm32
    void subEaxImm(int v) { emit8(0x2D); emit32((uint32_t)v); }                       // eax -= imm32
    void cmpEaxImm(int v) { emit8(0x3D); emit32((uint32_t)v); }                       // cmp eax, imm32
    void cmpEaxImm8(int v) { emit8(0x83); emit8(0xF8); emit8((uint8_t)v); }           // cmp eax, imm8
    void imulEaxEaxImm8(int v) { emit8(0x6B); emit8(0xC0); emit8((uint8_t)v); }       // eax *= imm8
    // trig tables and fixed-point products
    void movR10dR14Eax() { emit8(0x45); emit8(0x8B); emit8(0x14); emit8(0x86); }      // r10d = [r14+rax*4]
    void movR11dR15Eax() { emit8(0x45); emit8(0x8B); emit8(0x1C); emit8(0x87); }      // r11d = [r15+rax*4]
    void imulR10dEdx() { emit8(0x44); emit8(0x0F); emit8(0xAF); emit8(0xD2); }        // r10d *= edx
    void imulR11dEdx() { emit8(0x44); emit8(0x0F); emit8(0xAF); emit8(0xDA); }        // r11d *= edx
    // branchless roundShift temps
    void movR9dR10d() { emit8(0x45); emit8(0x89); emit8(0xD1); }                      // r9d = r10d
    void movR9dR11d() { emit8(0x45); emit8(0x89); emit8(0xD9); }                      // r9d = r11d
    void sarR9dImm(int v) { emit8(0x41); emit8(0xC1); emit8(0xF9); emit8((uint8_t)v); }
    void sarR10dImm(int v) { emit8(0x41); emit8(0xC1); emit8(0xFA); emit8((uint8_t)v); }
    void sarR11dImm(int v) { emit8(0x41); emit8(0xC1); emit8(0xFB); emit8((uint8_t)v); }
    void addR10dImm(int v) { emit8(0x41); emit8(0x81); emit8(0xC2); emit32((uint32_t)v); }
    void addR11dImm(int v) { emit8(0x41); emit8(0x81); emit8(0xC3); emit32((uint32_t)v); }
    void addR10dR9d() { emit8(0x45); emit8(0x01); emit8(0xCA); }                      // r10d += r9d
    void addR11dR9d() { emit8(0x45); emit8(0x01); emit8(0xCB); }                      // r11d += r9d
    // newX / newY
    void movR8dDwordRsi(int off) { emit8(0x44); emit8(0x8B); emit8(0x46); emit8((uint8_t)off); }  // r8d = [rsi+off]
    void movR9dDwordRsi(int off) { emit8(0x44); emit8(0x8B); emit8(0x4E); emit8((uint8_t)off); }  // r9d = [rsi+off]
    void addR8dR10d() { emit8(0x45); emit8(0x01); emit8(0xD0); }                      // r8d += r10d
    void addR9dR11d() { emit8(0x45); emit8(0x01); emit8(0xD9); }                      // r9d += r11d
    void movDwordRsiR8(int off) { emit8(0x44); emit8(0x89); emit8(0x46); emit8((uint8_t)off); }   // [rsi+off] = r8d
    void movDwordRsiR9(int off) { emit8(0x44); emit8(0x89); emit8(0x4E); emit8((uint8_t)off); }   // [rsi+off] = r9d
    // pen / sub-pixel window
    void cmpByteRsiImm(int off, int imm) { emit8(0x80); emit8(0x7E); emit8((uint8_t)off); emit8((uint8_t)imm); }
    void leaEaxR10d1() { emit8(0x41); emit8(0x8D); emit8(0x42); emit8(0x01); }        // eax = r10d + 1
    void leaEaxR11d1() { emit8(0x41); emit8(0x8D); emit8(0x43); emit8(0x01); }        // eax = r11d + 1
    // canvas-relative loads and stores
    void movRaxQwordRsi(int off) { emit8(0x48); emit8(0x8B); emit8(0x46); emit8((uint8_t)off); } // rax = [rsi+off]
    void movRaxQwordRax() { emit8(0x48); emit8(0x8B); emit8(0x00); }                   // rax = [rax] (canvas -> grid)
    void movEdxDwordRax(int off) { emit8(0x8B); emit8(0x50); emit8((uint8_t)off); }   // edx = [rax+off]
    void movEcxDwordRax(int off) { emit8(0x8B); emit8(0x48); emit8((uint8_t)off); }   // ecx = [rax+off]
    void movR10dDwordRax(int off) { emit8(0x44); emit8(0x8B); emit8(0x50); emit8((uint8_t)off); } // r10d = [rax+off]
    void addEdxDwordRsi0() { emit8(0x03); emit8(0x16); }                              // edx += [rsi]
    void addEcxDwordRsi4() { emit8(0x03); emit8(0x4E); emit8(0x04); }                 // ecx += [rsi+4]
    void addEdxR8d() { emit8(0x44); emit8(0x01); emit8(0xC2); }                       // edx += r8d
    void addEcxR9d() { emit8(0x44); emit8(0x01); emit8(0xC9); }                       // ecx += r9d
    void cmpEdxDwordRax(int off) { emit8(0x3B); emit8(0x50); emit8((uint8_t)off); }   // cmp edx, [rax+off]
    void cmpEcxDwordRax(int off) { emit8(0x3B); emit8(0x48); emit8((uint8_t)off); }   // cmp ecx, [rax+off]
    void imulEcxEcxR10d() { emit8(0x41); emit8(0x0F); emit8(0xAF); emit8(0xCA); }     // ecx *= r10d
    void addEcxEdx() { emit8(0x03); emit8(0xCA); }                                    // ecx += edx
    void movzxR11dByteRsi(int off) { emit8(0x44); emit8(0x0F); emit8(0xB6); emit8(0x5E); emit8((uint8_t)off); } // r11d = zext byte [rsi+off]
    void movByteRaxRcxR11d() { emit8(0x44); emit8(0x88); emit8(0x1C); emit8(0x08); }  // [rax+rcx] = r11b
    // long-line fallback call
    void movEdxR8d() { emit8(0x44); emit8(0x89); emit8(0xC2); }                       // edx = r8d
    void movR8dR9d() { emit8(0x45); emit8(0x89); emit8(0xC8); }                       // r8d = r9d
    void callRax() { emit8(0xFF); emit8(0xD0); }
    void testEdxEdx() { emit8(0x85); emit8(0xD2); }
    void cmpEdxImm(int v) { emit8(0x81); emit8(0xFA); emit32((uint32_t)v); }
    void storeByteRsi(int off, uint8_t v) {
        emit8(0xC6); emit8(0x46); emit8((uint8_t)off); emit8(v);
    }

private:
    void ripOp(uint8_t op, double v) {
        uint32_t item = poolAdd(v);
        emit8(0xF2); emit8(0x0F); emit8(op); emit8(0x05);
        ripRefs.push_back({here(), item});
        emit32(0);
    }
};

// jcc opcodes (rel32 form uses 0x0F 0x8x with these low nibbles)
constexpr uint8_t JO = 0x0, JB = 0x2, JE = 0x4, JNE = 0x5, JBE = 0x6, JA = 0x7,
                  JAE = 0x3, JS = 0x8, JL = 0xC, JGE = 0xD, JLE = 0xE, JG = 0xF;

struct PendingCall {
    uint32_t relOffset;                 // position of rel32 field
    const CompiledFunction* target;
};

// Per-function emission context: which var slots are parameters
// (they live in the native frame instead of the global var array).
// frameSlot stores the RBP-relative offset of each param slot.
struct FnCtx {
    int frameSlot[MAX_VARS];            // var slot -> rbp offset, -1 if global
    const CompiledFunction* cf;
};

// Register conventions inside emitted code:
//   rbp = frame base (params addressed [rbp+off], stable across rsp shifts)
//   rbx = global varSlots base
//   rsi = Turtle*
//   r14 = &Turtle::cosTable, r15 = &Turtle::sinTable (set by top-level prologue)
class CodeGen {
public:
    Emitter& e;
    std::vector<PendingCall> pendingCalls;
    std::unordered_map<const CompiledFunction*, uint32_t> entries;

    explicit CodeGen(Emitter& em) : e(em) {}

    static constexpr int kPushedBytes = 40;    // rbp, rbx, rsi, r14, r15

    void emitFunction(const CompiledFunction* cf, const std::vector<CInstr>& body,
                      bool isTopLevel) {
        FnCtx ctx;
        ctx.cf = cf;
        for (int i = 0; i < MAX_VARS; ++i) ctx.frameSlot[i] = -1;
        int np = cf ? cf->nParams : 0;

        // Frame layout, low to high: [rsp, rsp+0x20) is the callee home
        // space for helper calls (no per-call subRsp needed), then the np
        // parameter slots. Parity: entry rsp = 16n+8; five pushes bring it
        // to 16n; frame is 16-aligned, so all interior calls stay 16-aligned.
        uint32_t frame = ((np * 8 + 0x20 + 15) & ~15u);
        e.emit8(0x55);                                // push rbp
        e.emit8(0x48); e.emit8(0x89); e.emit8(0xE5);  // mov rbp, rsp
        e.emit8(0x53);                                // push rbx
        e.emit8(0x56);                                // push rsi
        e.emit8(0x41); e.emit8(0x56);                 // push r14
        e.emit8(0x41); e.emit8(0x57);                 // push r15
        e.subRsp(frame);
        e.movRbxRdx();                                // rbx = vars
        e.movRsiRcx();                                // rsi = turtle
        if (isTopLevel) {
            // mov r14, &Turtle::cosTable ; mov r15, &Turtle::sinTable
            e.emit8(0x49); e.emit8(0xBE);
            e.emit64(reinterpret_cast<uint64_t>(&Turtle::cosTable));
            e.emit8(0x49); e.emit8(0xBF);
            e.emit64(reinterpret_cast<uint64_t>(&Turtle::sinTable));
        }
        for (int i = 0; i < np; ++i) {
            // Param slots live above the 32B home space at the bottom of the
            // subRsp frame: slot np-1 at [rsp+0x20], slot 0 at the top.
            // (Only 32 bytes of pushes are below rbp: rbx, rsi, r14, r15.)
            int off = 0x20 + 8 * (np - 1 - i) - 32 - static_cast<int>(frame);
            ctx.frameSlot[cf->paramSlotArr[i]] = off;
            if (i < 4) {
                e.storeParamReg(off, i);              // [rbp+off] = xmm{i}
            } else {
                // xmm0 = [r8 + 8*(i-4)]; store to param slot
                e.emit8(0xF2); e.emit8(0x41); e.emit8(0x0F); e.emit8(0x10);
                e.emit8(0x40); e.emit8((uint8_t)(8 * (i - 4)));
                e.storeParamReg(off, 0);
            }
        }

        emitBody(body, ctx);

        e.addRsp(frame);
        e.emit8(0x41); e.emit8(0x5F);                 // pop r15
        e.emit8(0x41); e.emit8(0x5E);                 // pop r14
        e.emit8(0x5E);                                // pop rsi
        e.emit8(0x5B);                                // pop rbx
        e.emit8(0x5D);                                // pop rbp
        e.ret();
    }

    void emitBody(const std::vector<CInstr>& body, const FnCtx& ctx) {
        for (const CInstr& instr : body) emitInstr(instr, ctx);
    }

    // ==================== EXPRESSIONS (result in xmm0) ====================
    void emitExpr(const CExpr& expr, const FnCtx& ctx) {
        switch (expr.type) {
            case CExpr::LITERAL:
                e.loadConstRip(expr.literal);
                break;
            case CExpr::SLOT:
                loadSlot(expr.slot, ctx);
                break;
            case CExpr::SLOT_ADD_LIT:
                loadSlot(expr.slot, ctx);
                e.addConstRip(expr.literal);
                break;
            case CExpr::SLOT_SUB_LIT:
                loadSlot(expr.slot, ctx);
                e.subConstRip(expr.literal);
                break;
            case CExpr::SLOT_MUL_LIT:
                loadSlot(expr.slot, ctx);
                e.mulConstRip(expr.literal);
                break;
            case CExpr::ADD:
            case CExpr::SUB:
            case CExpr::MUL: {
                // left -> stash on stack, right -> xmm0, left -> xmm1
                emitSide(expr, true, ctx);
                emitSide(expr, false, ctx);
                e.loadXmm1Rsp(0);                      // xmm1 = left
                e.addRsp(8);                           // xmm0 = right
                if (expr.type == CExpr::ADD) e.addsdXmm0Xmm1();       // left + right
                else if (expr.type == CExpr::MUL) e.mulsdXmm0Xmm1();  // left * right
                else { e.subsdXmm1Xmm0(); e.movapsXmm0Xmm1(); }       // left - right
                break;
            }
        }
    }

    void emitSide(const CExpr& expr, bool left, const FnCtx& ctx) {
        CExpr side;
        if (left) {
            side.type = (expr.leftSlot >= 0) ? CExpr::SLOT : CExpr::LITERAL;
            side.slot = expr.leftSlot;
            side.literal = expr.leftLiteral;
        } else {
            side.type = (expr.rightSlot >= 0) ? CExpr::SLOT : CExpr::LITERAL;
            side.slot = expr.rightSlot;
            side.literal = expr.rightLiteral;
        }
        if (left) {
            e.subRsp(8);
            emitExpr(side, ctx);
            e.storeRsp(0);
        } else {
            emitExpr(side, ctx);
        }
    }

    void loadSlot(int slot, const FnCtx& ctx) {
        int f = ctx.frameSlot[slot];
        if (f != -1) e.loadParam(f);
        else e.loadVar(slot);
    }
    void storeSlot(int slot, const FnCtx& ctx) {
        int f = ctx.frameSlot[slot];
        if (f != -1) e.storeParamReg(f, 0);
        else e.storeVar(slot);
    }

    // Evaluate expression and round exactly like the interpreter:
    // static_cast<int>(value + 0.5). Result in edx.
    void emitExprInt(const CExpr& expr, const FnCtx& ctx) {
        if (expr.type == CExpr::LITERAL) {
            // Constant-fold: the runtime would compute int(v + 0.5)
            e.movEaxImm(static_cast<int32_t>(expr.literal + 0.5));
            e.emit8(0x89); e.emit8(0xC2);              // mov edx, eax
            return;
        }
        emitExpr(expr, ctx);
        e.addConstRip(0.5);
        e.cvttsd2siEdxXmm0();
    }

    // ==================== INSTRUCTIONS ====================
    void emitInstr(const CInstr& instr, const FnCtx& ctx) {
        switch (instr.type) {
            case CommandType::Forward:
            case CommandType::Backward:
                emitMove(instr.type == CommandType::Forward, ctx, instr);
                break;
            case CommandType::Left:
            case CommandType::Right:
                emitTurn(instr.type == CommandType::Right, ctx, instr);
                break;
            case CommandType::PenUp: {
                e.storeByteRsi(static_cast<int>(offsetof(Turtle, penDown)), 0);
                break;
            }
            case CommandType::PenDown: {
                e.storeByteRsi(static_cast<int>(offsetof(Turtle, penDown)), 1);
                break;
            }
            case CommandType::Var: {
                emitExpr(instr.arg, ctx);
                storeSlot(instr.varSlot, ctx);
                break;
            }
            case CommandType::If: {
                Label end = emitConditionCheck(instr.condition, ctx);
                emitBody(instr.ifBody, ctx);
                e.bind(end);
                break;
            }
            case CommandType::Function: {
                const CompiledFunction* cf = instr.funcPtr;
                if (!cf) break;                        // unresolved: no-op, like interpreter
                emitCall(cf, instr, ctx);
                break;
            }
            default:
                break;
        }
    }

    void emitCall(const CompiledFunction* cf, const CInstr& instr, const FnCtx& ctx) {
        int np = cf->nParams;
        int na = instr.nCallArgs;
        // Early exit: when the callee is a single If guarding its whole body
        // with a param-vs-literal comparison, evaluate just that argument and
        // skip the entire call when the condition is false. Recursive Logo
        // patterns (trees, spirals) spend half their calls on guard-false
        // leaves, so this removes both the call and the callee frame.
        Label skip;
        const bool guarded = cf->hasEarlyExit && cf->earlyExitArgIdx < na;
        if (guarded) {
            emitExpr(instr.callArgs[cf->earlyExitArgIdx], ctx);  // xmm0 = arg
            e.movapsXY(1, 0);                                    // xmm1 = arg
            e.loadConstRip(cf->earlyExitLiteral);                // xmm0 = literal
            e.comisdXmm1Xmm0();
            switch (cf->earlyExitOp) {                           // skip when NOT(op)
                case CCondition::GT: e.jcc(JBE, skip); break;
                case CCondition::LT: e.jcc(JAE, skip); break;
                case CCondition::GE: e.jcc(JB, skip); break;
                case CCondition::LE: e.jcc(JA, skip); break;
                case CCondition::EQ: e.jcc(JNE, skip); break;
                case CCondition::NE: e.jcc(JE, skip); break;
            }
        }
        if (np <= 4) {
            // Register convention: args in xmm0..xmm3. Evaluate into
            // xmm2..xmm5 first (emitExpr uses only xmm0/xmm1), then shuffle
            // down in reverse order.
            for (int i = 0; i < np && i < na; ++i) {
                emitExpr(instr.callArgs[i], ctx);
                e.movapsXY(2 + i, 0);                  // xmm{2+i} = xmm0
            }
            for (int i = np - 1; i >= 0; --i)
                e.movapsXY(i, 2 + i);                  // xmm{i} = xmm{2+i}
            e.movRcxRsi();
            e.movRdxRbx();
            e.emit8(0xE8);                             // call rel32
            pendingCalls.push_back({e.here(), cf});
            e.emit32(0);
        } else {
            // Stack convention (rare): args at [rsp+0x20], r8 points to them
            uint32_t argArea = ((static_cast<uint32_t>(np) * 8 + 0x20 + 15) & ~15u);
            e.subRsp(argArea);
            for (int i = 0; i < np && i < na; ++i) {
                emitExpr(instr.callArgs[i], ctx);
                e.storeRsp(0x20 + i * 8);
            }
            e.movRcxRsi();
            e.movRdxRbx();
            e.emit8(0x4C); e.emit8(0x8D); e.emit8(0x44); e.emit8(0x24); e.emit8(0x20); // lea r8,[rsp+0x20]
            e.emit8(0xE8);
            pendingCalls.push_back({e.here(), cf});
            e.emit32(0);
            e.addRsp(argArea);
        }
        if (guarded) e.bind(skip);
    }

    // Inline copy of Turtle::Forward/Backward. Trig tables come from
    // r14/r15, rounding is the branchless roundShift, and the sub-pixel
    // pixel stores are emitted inline; only real (multi-pixel) lines call
    // jitMoveDraw. Must stay bit-exact with Turtle::MoveTo.
    void emitMove(bool isForward, const FnCtx& ctx, const CInstr& instr) {
        static_assert(offsetof(Turtle, posX) == 0);
        static_assert(offsetof(Turtle, posY) == 4);
        static_assert(offsetof(Turtle, angle) == 8);
        static_assert(offsetof(Turtle, penDown) == 12);
        static_assert(offsetof(Turtle, pen) == 24);
        static_assert(sizeof(Turtle) == 32);
        // offsetof is unreliable for the Canvas& reference member on MSVC,
        // but the asserts above pin the layout: penDown ends at 13 and the
        // 8-byte-aligned canvas reference fills [16, 24), so canvas == 16.
        // (emitMove loads it with movRaxQwordRsi(16).)
        static_assert(offsetof(Canvas, grid) == 0);
        static_assert(offsetof(Canvas, gridWidth) == 8);
        static_assert(offsetof(Canvas, gridHeight) == 12);
        static_assert(offsetof(Canvas, offsetX) == 16);
        static_assert(offsetof(Canvas, offsetY) == 20);

        emitExprInt(instr.arg, ctx);                  // edx = distance
        Label done;
        e.testEdxEdx();
        e.jcc(JLE, done);                             // if (distance <= 0) return

        e.movEaxDwordRsi(static_cast<int>(offsetof(Turtle, angle)));
        if (!isForward) {
            e.addEaxImm(180);
            Label nosub;
            e.cmpEaxImm(360);
            e.jcc(JL, nosub);
            e.subEaxImm(360);
            e.bind(nosub);
        }
        e.imulEaxEaxImm8(10);                         // idx = angle * 10

        e.movR10dR14Eax();                            // r10d = cosTable[idx]
        e.movR11dR15Eax();                            // r11d = sinTable[idx]
        e.imulR10dEdx();
        e.imulR11dEdx();

        // roundShift(v) = (v + HALF + (v >> 31)) >> SHIFT, for both axes.
        e.movR9dR10d();  e.sarR9dImm(31);
        e.addR10dImm(Turtle::TRIG_HALF); e.addR10dR9d(); e.sarR10dImm(Turtle::TRIG_SHIFT);
        e.movR8dDwordRsi(0); e.addR8dR10d();          // r8d = newX
        e.movR9dR11d();  e.sarR9dImm(31);
        e.addR11dImm(Turtle::TRIG_HALF); e.addR11dR9d(); e.sarR11dImm(Turtle::TRIG_SHIFT);
        e.movR9dDwordRsi(4); e.addR9dR11d();          // r9d = newY

        // Turtle::MoveTo: pen-up moves only update the position.
        Label storePos;
        e.cmpByteRsiImm(12, 0);
        e.jcc(JE, storePos);

        // Sub-pixel window: (unsigned)(adx+1) <= 2 && (unsigned)(ady+1) <= 2
        Label longPath;
        e.leaEaxR10d1(); e.cmpEaxImm8(2); e.jcc(JA, longPath);
        e.leaEaxR11d1(); e.cmpEaxImm8(2); e.jcc(JA, longPath);

        // Inline pixel stores (bounds-checked, no canvas expansion).
        // Storing the endpoint pixel unconditionally is identical to the
        // adx|ady test in MoveTo: for a zero move it rewrites the same
        // grid cell with the same pen character. The grid offset is
        // computed in ecx because writing eax would clear the upper half
        // of rax (the grid pointer).
        e.movRaxQwordRsi(16);                         // rax = &canvas
        e.movR10dDwordRax(8);                         // r10d = gridWidth
        e.movzxR11dByteRsi(24);                       // r11d = pen char

        e.movEdxDwordRax(16); e.addEdxDwordRsi0();    // edx = posX + offsetX
        e.movEcxDwordRax(20); e.addEcxDwordRsi4();    // ecx = posY + offsetY
        Label skip0;
        e.cmpEdxDwordRax(8);  e.jcc(JAE, skip0);      // ix0 >= gridWidth
        e.cmpEcxDwordRax(12); e.jcc(JAE, skip0);      // iy0 >= gridHeight
        e.movRaxQwordRax();                           // rax = canvas->grid
        e.imulEcxEcxR10d(); e.addEcxEdx();            // ecx = iy0 * gw + ix0
        e.movByteRaxRcxR11d();
        e.bind(skip0);

        e.movRaxQwordRsi(16);                         // rax = &canvas
        e.movEdxDwordRax(16); e.addEdxR8d();          // edx = newX + offsetX
        e.movEcxDwordRax(20); e.addEcxR9d();          // ecx = newY + offsetY
        Label skip1;
        e.cmpEdxDwordRax(8);  e.jcc(JAE, skip1);
        e.cmpEcxDwordRax(12); e.jcc(JAE, skip1);
        e.movRaxQwordRax();                           // rax = canvas->grid
        e.imulEcxEcxR10d(); e.addEcxEdx();            // ecx = iy1 * gw + ix1
        e.movByteRaxRcxR11d();
        e.bind(skip1);
        e.jmp(storePos);

        // Multi-pixel line: jitMoveDraw(turtle, newX, newY) draws and
        // stores the position; home space at [rsp) is pre-reserved.
        // It also clobbers r8-r11 (volatile), so this branch must skip
        // the position stores below.
        e.bind(longPath);
        e.movEdxR8d();
        e.movR8dR9d();
        e.movRcxRsi();
        e.movRaxImm(reinterpret_cast<uint64_t>(&jitMoveDraw));
        e.callRax();
        e.jmp(done);

        e.bind(storePos);
        e.movDwordRsiR8(0);                           // posX = newX
        e.movDwordRsiR9(4);                           // posY = newY
        e.bind(done);
    }

    // Emits the turn logic; shared by Left/Right after arg eval (edx).
    void emitTurn(bool isRight, const FnCtx& ctx, const CInstr& instr) {        const int angleOff = static_cast<int>(offsetof(Turtle, angle));
        emitExprInt(instr.arg, ctx);            // result in edx (eax only
        e.movEaxEdx();                          //   reliable for literals)
        e.movEdxDwordRsi(angleOff);             // edx = turtle->angle
        if (isRight) e.addEdxEax();
        else e.subEdxEax();
        Label skip, done;
        if (isRight) {
            // if (edx >= 360) edx -= 360; else if (edx < 0) edx += 360;
            e.cmpEdxImm(360);
            e.jcc(JL, skip);
            e.subEdxImm(360);
            e.jmp(done);
            e.bind(skip);
            e.testEdxEdx();
            e.jcc(JGE, done);
            e.addEdxImm(360);
        } else {
            // if (edx < 0) edx += 360; else if (edx >= 360) edx -= 360;
            e.testEdxEdx();
            e.jcc(JGE, skip);
            e.addEdxImm(360);
            e.jmp(done);
            e.bind(skip);
            e.cmpEdxImm(360);
            e.jcc(JL, done);
            e.subEdxImm(360);
        }
        e.bind(done);
        e.storeEdxToRsi(angleOff);
    }

    // Emits condition evaluation plus an inverted jump to skip the body.
    // Returns the end label (caller binds it after emitting the body).
    Label emitConditionCheck(const CCondition* cond, const FnCtx& ctx) {
        Label end;
        if (!cond) { e.jmp(end); return end; }         // no condition: never run
        emitExpr(cond->left, ctx);
        e.subRsp(8);
        e.storeRsp(0);
        emitExpr(cond->right, ctx);
        e.loadXmm1Rsp(0);                              // xmm1 = left
        e.addRsp(8);                                   // xmm0 = right
        e.comisdXmm1Xmm0();
        switch (cond->op) {                            // skip body when NOT(op)
            case CCondition::GT: e.jcc(JBE, end); break;
            case CCondition::LT: e.jcc(JAE, end); break;
            case CCondition::GE: e.jcc(JB, end); break;
            case CCondition::LE: e.jcc(JA, end); break;
            case CCondition::EQ: e.jcc(JNE, end); break;
            case CCondition::NE: e.jcc(JE, end); break;
        }
        return end;
    }
};

// Executable allocations live for the process lifetime (programs compile
// once per run; the interpreter is also free to compile and drop handlers).
std::vector<void*>& jitAllocations() {
    static std::vector<void*> v;
    return v;
}

} // namespace

bool Instruction::jitCompile() {
    if (jitEntry) return true;
    if (std::getenv("LOGOCPP_NO_JIT")) return false;
    if (compiledTopLevel.empty()) return false;

    Emitter em;
    CodeGen cg(em);

    for (auto& [name, cf] : compiledFunctions) {
        cg.entries[&cf] = em.here();
        cg.emitFunction(&cf, cf.body, false);
    }
    uint32_t topEntry = em.here();
    cg.emitFunction(nullptr, compiledTopLevel, true);

    for (const PendingCall& pc : cg.pendingCalls) {
        auto it = cg.entries.find(pc.target);
        if (it == cg.entries.end()) continue;          // target never emitted
        uint32_t rel = it->second - (pc.relOffset + 4);
        em.patch32(pc.relOffset, rel);
    }
    em.finalize();                                     // append pool, patch rips

    if (std::getenv("LOGOCPP_JIT_DUMP")) {
        FILE* f = std::fopen("jit_dump.bin", "wb");
        if (f) { std::fwrite(em.code.data(), 1, em.code.size(), f); std::fclose(f); }
    }

    SIZE_T size = em.code.size();
    void* mem = VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!mem) return false;
    std::memcpy(mem, em.code.data(), em.code.size());
    DWORD oldProtect = 0;
    if (!VirtualProtect(mem, size, PAGE_EXECUTE_READ, &oldProtect)) {
        VirtualFree(mem, 0, MEM_RELEASE);
        return false;
    }
    FlushInstructionCache(GetCurrentProcess(), mem, size);
    jitAllocations().push_back(mem);

    jitEntry = static_cast<uint8_t*>(mem) + topEntry;
    return true;
}

#else // !LOGOCPP_JIT_AVAILABLE

bool Instruction::jitCompile() { return false; }

#endif
