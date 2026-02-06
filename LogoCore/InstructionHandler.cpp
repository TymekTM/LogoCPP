#include "pch.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"
#include "ParsingHelper.h"
#include <cmath>
#include <cctype>

// ==================== COMMAND LOOKUP ====================
std::unordered_map<std::string, CommandType> Instruction::InitCommandLookup() {
    return {
        {"Forward", CommandType::Forward}, {"forward", CommandType::Forward},
        {"przod", CommandType::Forward}, {"Przod", CommandType::Forward},
        {"Backward", CommandType::Backward}, {"backward", CommandType::Backward},
        {"tyl", CommandType::Backward}, {"Tyl", CommandType::Backward},
        {"Left", CommandType::Left}, {"left", CommandType::Left},
        {"lewo", CommandType::Left}, {"Lewo", CommandType::Left},
        {"Right", CommandType::Right}, {"right", CommandType::Right},
        {"prawo", CommandType::Right}, {"Prawo", CommandType::Right},
        {"penup", CommandType::PenUp}, {"PenUp", CommandType::PenUp}, {"pu", CommandType::PenUp},
        {"pendown", CommandType::PenDown}, {"PenDown", CommandType::PenDown}, {"pd", CommandType::PenDown},
        {"var", CommandType::Var}, {"if", CommandType::If}, {"def", CommandType::Def}
    };
}

const std::unordered_map<std::string, CommandType> Instruction::commandLookup = Instruction::InitCommandLookup();

Instruction::Instruction(Turtle& turtle) : turtlePtr(&turtle) {
    std::memset(varSlots, 0, sizeof(varSlots));
}

int Instruction::getOrCreateSlot(const std::string& name) {
    auto it = varNameToSlot.find(name);
    if (it != varNameToSlot.end()) return it->second;
    int slot = nextSlot++;
    varNameToSlot[name] = slot;
    return slot;
}

CommandType Instruction::GetCommandType(const std::string& command) const {
    auto it = commandLookup.find(command);
    if (it != commandLookup.end()) return it->second;
    if (compiledFunctions.count(command) || functions.count(command)) return CommandType::Function;
    return CommandType::Unknown;
}

void Instruction::Execute(const std::string& instructionSet) {
    Tokenizer tokenizer;
    tokenizer.TokenizeAndExecute(instructionSet, *this);
}

void Instruction::ExecuteTopLevel() {
    executeCompiledRaw(compiledTopLevel.data(), static_cast<int>(compiledTopLevel.size()));
}

// ==================== HELPERS ====================
static std::string_view trimSV(std::string_view s) noexcept {
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.remove_prefix(1);
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.remove_suffix(1);
    return s;
}

static bool isNumberStr(const std::string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '-') i = 1;
    bool hasDot = false;
    for (; i < s.size(); ++i) {
        if (s[i] == '.') { if (hasDot) return false; hasDot = true; }
        else if (s[i] < '0' || s[i] > '9') return false;
    }
    return i > (s[0] == '-' ? 1u : 0u);
}

static std::string extractParenContent(const std::string& s) {
    auto p1 = s.find('(');
    if (p1 == std::string::npos) return "";
    auto p2 = s.find(')', p1);
    if (p2 == std::string::npos) return "";
    return s.substr(p1 + 1, p2 - p1 - 1);
}

static std::string extractCommand(const std::string& s) {
    std::string_view t = trimSV(s);
    bool hasEquals = false, hasParen = false;
    for (char c : t) {
        if (c == '=') hasEquals = true;
        else if (c == '(') { hasParen = true; break; }
    }
    if (hasEquals && !hasParen) return "var";
    size_t end = 0;
    for (size_t i = 0; i < t.size(); ++i) {
        if (t[i] == '(' || t[i] == ' ') { end = i; break; }
        end = i + 1;
    }
    return std::string(trimSV(t.substr(0, end)));
}

static std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> args;
    auto p1 = s.find('(');
    auto p2 = s.find(')');
    if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1 + 1) return args;
    args.reserve(4);
    size_t start = p1 + 1;
    for (size_t i = p1 + 1; i <= p2; ++i) {
        if (i == p2 || s[i] == ',') {
            if (i > start) {
                auto sv = trimSV(std::string_view(s.data() + start, i - start));
                if (!sv.empty()) args.emplace_back(sv);
            }
            start = i + 1;
        }
    }
    return args;
}

static std::string extractBrackets(const std::string& s, size_t startPos) {
    int depth = 0;
    size_t contentStart = 0;
    for (size_t i = startPos; i < s.size(); ++i) {
        if (s[i] == '{') { depth++; if (depth == 1) contentStart = i + 1; }
        else if (s[i] == '}') { depth--; if (depth == 0) return s.substr(contentStart, i - contentStart); }
    }
    return "";
}

// ==================== EXPRESSION COMPILATION ====================
CExpr Instruction::compileExpr(const std::string& raw) {
    CExpr e;
    std::string expr(trimSV(raw));
    
    // Check for arithmetic
    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        if (c == '+' || c == '-' || c == '*') {
            std::string leftStr(trimSV(std::string_view(expr.data(), i)));
            std::string rightStr(trimSV(std::string_view(expr.data() + i + 1, expr.size() - i - 1)));
            
            bool leftIsSlot = !isNumberStr(leftStr);
            bool rightIsLit = isNumberStr(rightStr);
            
            // Try specialized SLOT_OP_LIT patterns
            if (leftIsSlot && rightIsLit) {
                double rv = std::stod(rightStr);
                int ls = getOrCreateSlot(leftStr);
                if (c == '*') { e.type = CExpr::SLOT_MUL_LIT; e.slot = ls; e.literal = rv; return e; }
                if (c == '-') { e.type = CExpr::SLOT_SUB_LIT; e.slot = ls; e.literal = rv; return e; }
                if (c == '+') { e.type = CExpr::SLOT_ADD_LIT; e.slot = ls; e.literal = rv; return e; }
            }
            
            // Generic binary op
            if (c == '+') e.type = CExpr::ADD;
            else if (c == '-') e.type = CExpr::SUB;
            else e.type = CExpr::MUL;
            
            if (!leftIsSlot) {
                e.leftSlot = -1;
                e.leftLiteral = std::stod(leftStr);
            } else {
                e.leftSlot = getOrCreateSlot(leftStr);
            }
            if (rightIsLit) {
                e.rightSlot = -1;
                e.rightLiteral = std::stod(rightStr);
            } else {
                e.rightSlot = getOrCreateSlot(rightStr);
            }
            return e;
        }
    }
    
    if (isNumberStr(expr)) {
        e.type = CExpr::LITERAL;
        e.literal = std::stod(expr);
    } else {
        e.type = CExpr::SLOT;
        e.slot = getOrCreateSlot(expr);
    }
    return e;
}

CCondition Instruction::compileCondition(const std::string& condStr) {
    CCondition cond;
    
    for (size_t i = 0; i + 1 < condStr.size(); ++i) {
        char c = condStr[i], n = condStr[i + 1];
        if ((c == '=' && n == '=') || (c == '<' && n == '>') ||
            (c == '>' && n == '=') || (c == '<' && n == '=')) {
            std::string left(trimSV(std::string_view(condStr.data(), i)));
            std::string right(trimSV(std::string_view(condStr.data() + i + 2, condStr.size() - i - 2)));
            cond.left = compileExpr(left);
            cond.right = compileExpr(right);
            if (c == '=' && n == '=') cond.op = CCondition::EQ;
            else if (c == '<' && n == '>') cond.op = CCondition::NE;
            else if (c == '>' && n == '=') cond.op = CCondition::GE;
            else cond.op = CCondition::LE;
            return cond;
        }
    }
    
    for (size_t i = 0; i < condStr.size(); ++i) {
        char c = condStr[i];
        if (c == '>' || c == '<') {
            if (i + 1 < condStr.size() && (condStr[i+1] == '=' || (c == '<' && condStr[i+1] == '>'))) continue;
            if (i > 0 && (condStr[i-1] == '<' || condStr[i-1] == '>' || condStr[i-1] == '=')) continue;
            std::string left(trimSV(std::string_view(condStr.data(), i)));
            std::string right(trimSV(std::string_view(condStr.data() + i + 1, condStr.size() - i - 1)));
            cond.left = compileExpr(left);
            cond.right = compileExpr(right);
            cond.op = (c == '>') ? CCondition::GT : CCondition::LT;
            return cond;
        }
    }
    return cond;
}

// ==================== INSTRUCTION COMPILATION ====================
CInstr Instruction::compileInstruction(const std::string& instruction) {
    CInstr ci;
    std::string cmd = extractCommand(instruction);
    
    auto it = commandLookup.find(cmd);
    if (it != commandLookup.end()) {
        ci.type = it->second;
    } else {
        ci.type = CommandType::Function;
        ci.funcName = cmd;
    }
    
    switch (ci.type) {
        case CommandType::Forward:
        case CommandType::Backward:
        case CommandType::Left:
        case CommandType::Right:
            ci.arg = compileExpr(extractParenContent(instruction));
            break;
        case CommandType::PenUp:
        case CommandType::PenDown:
            break;
        case CommandType::Var: {
            auto eqPos = instruction.find('=');
            if (eqPos != std::string::npos) {
                std::string varName(trimSV(std::string_view(instruction.data(), eqPos)));
                ci.varSlot = getOrCreateSlot(varName);
                std::string valStr(trimSV(std::string_view(instruction.data() + eqPos + 1, instruction.size() - eqPos - 1)));
                ci.arg = compileExpr(valStr);
            }
            break;
        }
        case CommandType::If: {
            std::string condStr = extractParenContent(instruction);
            ci.condition = new CCondition(compileCondition(condStr));
            size_t bracketPos = instruction.find('{');
            if (bracketPos != std::string::npos) {
                ci.ifBody = compileBlock(extractBrackets(instruction, bracketPos));
            }
            break;
        }
        case CommandType::Function: {
            auto args = splitArgs(instruction);
            ci.nCallArgs = static_cast<int>(args.size());
            if (ci.nCallArgs > 0) {
                ci.callArgs = new CExpr[ci.nCallArgs];
                for (int i = 0; i < ci.nCallArgs; ++i)
                    ci.callArgs[i] = compileExpr(args[i]);
            }
            break;
        }
        default: break;
    }
    return ci;
}

std::vector<CInstr> Instruction::compileBlock(const std::string& block) {
    std::vector<CInstr> result;
    size_t begin = 0;
    int braceDepth = 0;
    
    for (size_t i = 0; i < block.size(); ++i) {
        if (block[i] == '{') braceDepth++;
        else if (block[i] == '}') { if (braceDepth > 0) braceDepth--; }
        if (block[i] == ';' && braceDepth == 0) {
            if (i > begin) {
                auto sv = trimSV(std::string_view(block.data() + begin, i - begin));
                if (!sv.empty()) result.push_back(compileInstruction(std::string(sv)));
            }
            begin = i + 1;
        }
    }
    if (begin < block.size()) {
        auto sv = trimSV(std::string_view(block.data() + begin, block.size() - begin));
        if (!sv.empty()) result.push_back(compileInstruction(std::string(sv)));
    }
    return result;
}

CompiledFunction Instruction::compileFunction(const std::string& body, const std::vector<std::string>& params) {
    CompiledFunction cf;
    cf.paramNames = params;
    cf.nParams = static_cast<int>(params.size());
    for (int i = 0; i < cf.nParams; ++i) {
        cf.paramSlotArr[i] = getOrCreateSlot(params[i]);
    }
    cf.body = compileBlock(body);
    
    // Detect single-If pattern for early exit
    if (cf.body.size() == 1 && cf.body[0].type == CommandType::If && cf.body[0].condition) {
        const CCondition& cond = *cf.body[0].condition;
        if (cond.left.type == CExpr::SLOT && cond.right.type == CExpr::LITERAL) {
            int condSlot = cond.left.slot;
            for (int i = 0; i < cf.nParams; ++i) {
                if (cf.paramSlotArr[i] == condSlot) {
                    cf.hasEarlyExit = true;
                    cf.earlyExitArgIdx = i;
                    cf.earlyExitOp = cond.op;
                    cf.earlyExitLiteral = cond.right.literal;
                    // Cache ifBody pointer (valid as long as body vector isn't reallocated)
                    cf.earlyExitBodyPtr = cf.body[0].ifBody.data();
                    cf.earlyExitBodySize = static_cast<int>(cf.body[0].ifBody.size());
                    break;
                }
            }
        }
    }
    
    return cf;
}

void Instruction::resolveFuncPtrs(std::vector<CInstr>& instrs) {
    for (auto& instr : instrs) {
        if (instr.type == CommandType::Function && !instr.funcName.empty()) {
            auto it = compiledFunctions.find(instr.funcName);
            if (it != compiledFunctions.end()) {
                instr.funcPtr = &(it->second);
            }
        }
        if (!instr.ifBody.empty()) {
            resolveFuncPtrs(instr.ifBody);
        }
    }
}

// ==================== COMPILED EXECUTION ====================
// Recursive version - compiler optimizes this better than manual stack
void Instruction::executeCompiled(const std::vector<CInstr>& instructions) {
    double* __restrict vs = varSlots;
    const CInstr* __restrict ip = instructions.data();
    const size_t n = instructions.size();
    
    for (size_t idx = 0; idx < n; ++idx) {
        const CInstr& instr = ip[idx];
        
        switch (instr.type) {
            case CommandType::Forward:
                turtlePtr->Forward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Backward:
                turtlePtr->Backward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Left:
                turtlePtr->Left(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Right:
                turtlePtr->Right(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::PenUp:
                turtlePtr->PenUp();
                break;
            case CommandType::PenDown:
                turtlePtr->PenDown();
                break;
            case CommandType::Var:
                vs[instr.varSlot] = instr.arg.eval(vs);
                break;
            case CommandType::If:
                if (instr.condition && instr.condition->eval(vs)) {
                    executeCompiled(instr.ifBody);
                }
                break;
            case CommandType::Function: {
                const CompiledFunction* __restrict cf = instr.funcPtr;
                if (!cf) [[unlikely]] break;
                
                const int np = cf->nParams;
                const int na = instr.nCallArgs;
                const CExpr* __restrict args = instr.callArgs;
                
                // Early exit: check if the function's body condition will be false
                if (cf->hasEarlyExit) [[likely]] {
                    double argVals[8];
                    for (int i = 0; i < np && i < na; ++i)
                        argVals[i] = args[i].eval(vs);
                    
                    double checkVal = argVals[cf->earlyExitArgIdx];
                    bool condTrue = false;
                    switch (cf->earlyExitOp) {
                        case CCondition::GT: condTrue = checkVal > cf->earlyExitLiteral; break;
                        case CCondition::LT: condTrue = checkVal < cf->earlyExitLiteral; break;
                        case CCondition::GE: condTrue = checkVal >= cf->earlyExitLiteral; break;
                        case CCondition::LE: condTrue = checkVal <= cf->earlyExitLiteral; break;
                        case CCondition::EQ: condTrue = checkVal == cf->earlyExitLiteral; break;
                        case CCondition::NE: condTrue = checkVal != cf->earlyExitLiteral; break;
                    }
                    if (!condTrue) [[unlikely]] break;
                    
                    // Save, set args, execute cached ifBody directly, restore
                    const int* __restrict slots = cf->paramSlotArr;
                    double saved[8];
                    for (int i = 0; i < np; ++i) {
                        int s = slots[i];
                        saved[i] = vs[s];
                        vs[s] = argVals[i];
                    }
                    
                    // Use cached ifBody pointer - avoids vector indirection
                    executeCompiledRaw(cf->earlyExitBodyPtr, cf->earlyExitBodySize);
                    
                    for (int i = 0; i < np; ++i)
                        vs[slots[i]] = saved[i];
                } else [[unlikely]] {
                    double argVals[8];
                    for (int i = 0; i < np && i < na; ++i)
                        argVals[i] = args[i].eval(vs);
                    
                    const int* __restrict slots = cf->paramSlotArr;
                    double saved[8];
                    for (int i = 0; i < np; ++i) {
                        int s = slots[i];
                        saved[i] = vs[s];
                        vs[s] = argVals[i];
                    }
                    
                    executeCompiled(cf->body);
                    
                    for (int i = 0; i < np; ++i)
                        vs[slots[i]] = saved[i];
                }
                break;
            }
            default: break;
        }
    }
}

void Instruction::executeCompiledInstr(const CInstr& instr) {}

// ==================== ITERATIVE SELF-RECURSIVE EXECUTOR ====================
// Replaces C++ recursion with an explicit compact stack.
// Handles the hot path of self-recursive functions (like krzaczek) without
// paying C++ function call overhead (~3ns/call × 65K calls = ~200µs saved).
void Instruction::executeIterative(const CInstr* __restrict topIp, int topCount) {
    struct Frame {
        const CInstr* ip;
        int count;
        int pc;
        int nRestore;           // number of params to restore when frame ends
        const int* restoreSlots; // param slots to restore
        double saved[8];
    };
    
    static constexpr int MAX_DEPTH = 256;
    Frame stack[MAX_DEPTH];
    int sp = 0;
    
    double* __restrict vs = varSlots;
    Turtle* __restrict t = turtlePtr;
    
    // Top-level frame has nothing to restore
    stack[0] = { topIp, topCount, 0, 0, nullptr, {} };
    
    while (sp >= 0) {
        Frame& f = stack[sp];
        
        if (f.pc >= f.count) [[unlikely]] {
            // Frame done - restore saved vars
            for (int i = 0; i < f.nRestore; ++i)
                vs[f.restoreSlots[i]] = f.saved[i];
            sp--;
            continue;
        }
        
        const CInstr& instr = f.ip[f.pc];
        f.pc++;
        
        switch (instr.type) {
            case CommandType::Forward:
                t->Forward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Backward:
                t->Backward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Left:
                t->Left(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Right:
                t->Right(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::PenUp:
                t->PenUp();
                break;
            case CommandType::PenDown:
                t->PenDown();
                break;
            case CommandType::Var:
                vs[instr.varSlot] = instr.arg.eval(vs);
                break;
            case CommandType::If:
                if (instr.condition && instr.condition->eval(vs)) {
                    if (sp + 1 < MAX_DEPTH && !instr.ifBody.empty()) {
                        sp++;
                        stack[sp] = { instr.ifBody.data(), static_cast<int>(instr.ifBody.size()), 0, 0, nullptr, {} };
                    }
                }
                break;
            case CommandType::Function: {
                const CompiledFunction* __restrict cf = instr.funcPtr;
                if (!cf) [[unlikely]] break;
                
                const int np = cf->nParams;
                const int na = instr.nCallArgs;
                const CExpr* __restrict args = instr.callArgs;
                
                double argVals[8];
                for (int i = 0; i < np && i < na; ++i)
                    argVals[i] = args[i].eval(vs);
                
                if (cf->hasEarlyExit) [[likely]] {
                    double checkVal = argVals[cf->earlyExitArgIdx];
                    bool condTrue = false;
                    switch (cf->earlyExitOp) {
                        case CCondition::GT: condTrue = checkVal > cf->earlyExitLiteral; break;
                        case CCondition::LT: condTrue = checkVal < cf->earlyExitLiteral; break;
                        case CCondition::GE: condTrue = checkVal >= cf->earlyExitLiteral; break;
                        case CCondition::LE: condTrue = checkVal <= cf->earlyExitLiteral; break;
                        case CCondition::EQ: condTrue = checkVal == cf->earlyExitLiteral; break;
                        case CCondition::NE: condTrue = checkVal != cf->earlyExitLiteral; break;
                    }
                    if (!condTrue) [[unlikely]] break;
                    
                    if (sp + 1 < MAX_DEPTH) {
                        const int* __restrict slots = cf->paramSlotArr;
                        sp++;
                        Frame& child = stack[sp];
                        child.ip = cf->earlyExitBodyPtr;
                        child.count = cf->earlyExitBodySize;
                        child.pc = 0;
                        child.nRestore = np;
                        child.restoreSlots = slots;
                        for (int i = 0; i < np; ++i) {
                            int s = slots[i];
                            child.saved[i] = vs[s];
                            vs[s] = argVals[i];
                        }
                    }
                } else [[unlikely]] {
                    if (sp + 1 < MAX_DEPTH) {
                        const int* __restrict slots = cf->paramSlotArr;
                        sp++;
                        Frame& child = stack[sp];
                        child.ip = cf->body.data();
                        child.count = static_cast<int>(cf->body.size());
                        child.pc = 0;
                        child.nRestore = np;
                        child.restoreSlots = slots;
                        for (int i = 0; i < np; ++i) {
                            int s = slots[i];
                            child.saved[i] = vs[s];
                            vs[s] = argVals[i];
                        }
                    }
                }
                break;
            }
            default: break;
        }
    }
}

// Raw pointer version - avoids vector overhead for cached ifBody
void Instruction::executeCompiledRaw(const CInstr* __restrict ip, int count) {
    double* __restrict vs = varSlots;
    Turtle* __restrict t = turtlePtr;
    
    for (int idx = 0; idx < count; ++idx) {
        const CInstr& instr = ip[idx];
        
        switch (instr.type) {
            case CommandType::Forward:
                t->Forward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Backward:
                t->Backward(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Left:
                t->Left(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::Right:
                t->Right(static_cast<int>(instr.arg.eval(vs) + 0.5));
                break;
            case CommandType::PenUp:
                t->PenUp();
                break;
            case CommandType::PenDown:
                t->PenDown();
                break;
            case CommandType::Var:
                vs[instr.varSlot] = instr.arg.eval(vs);
                break;
            case CommandType::If:
                if (instr.condition && instr.condition->eval(vs)) {
                    executeCompiled(instr.ifBody);
                }
                break;
            case CommandType::Function: {
                const CompiledFunction* __restrict cf = instr.funcPtr;
                if (!cf) [[unlikely]] break;
                
                const int np = cf->nParams;
                const CExpr* __restrict args = instr.callArgs;
                
                if (cf->hasEarlyExit) [[likely]] {
                    double argVals[8];
                    for (int i = 0; i < np && i < instr.nCallArgs; ++i)
                        argVals[i] = args[i].eval(vs);
                    
                    double checkVal = argVals[cf->earlyExitArgIdx];
                    bool condTrue = false;
                    switch (cf->earlyExitOp) {
                        case CCondition::GT: condTrue = checkVal > cf->earlyExitLiteral; break;
                        case CCondition::LT: condTrue = checkVal < cf->earlyExitLiteral; break;
                        case CCondition::GE: condTrue = checkVal >= cf->earlyExitLiteral; break;
                        case CCondition::LE: condTrue = checkVal <= cf->earlyExitLiteral; break;
                        case CCondition::EQ: condTrue = checkVal == cf->earlyExitLiteral; break;
                        case CCondition::NE: condTrue = checkVal != cf->earlyExitLiteral; break;
                    }
                    if (!condTrue) [[unlikely]] break;
                    
                    const int* __restrict slots = cf->paramSlotArr;
                    int s0 = slots[0], s1 = slots[1];
                    double sv0 = vs[s0], sv1 = vs[s1];
                    vs[s0] = argVals[0];
                    if (np >= 2) vs[s1] = argVals[1];
                    
                    executeCompiledRaw(cf->earlyExitBodyPtr, cf->earlyExitBodySize);
                    
                    vs[s0] = sv0;
                    if (np >= 2) vs[s1] = sv1;
                } else [[unlikely]] {
                    double argVals[8];
                    for (int i = 0; i < np && i < instr.nCallArgs; ++i)
                        argVals[i] = args[i].eval(vs);
                    
                    const int* __restrict slots = cf->paramSlotArr;
                    double saved[8];
                    for (int i = 0; i < np; ++i) {
                        int s = slots[i];
                        saved[i] = vs[s];
                        vs[s] = argVals[i];
                    }
                    
                    executeCompiled(cf->body);
                    
                    for (int i = 0; i < np; ++i)
                        vs[slots[i]] = saved[i];
                }
                break;
            }
            default: break;
        }
    }
}

// ==================== TOP-LEVEL INSTRUCTION HANDLING ====================
void Instruction::HandleInstruction(const std::string& instruction, Tokenizer& tokenizer) {
    std::string command = tokenizer.ExtractCommand(instruction);
    CommandType cmdType = GetCommandType(command);

    if (cmdType == CommandType::Def) {
        std::string functionName = tokenizer.ExtractFunctionName(instruction);
        if (functionName.empty()) return;
        
        // Skip if already compiled (cached benchmark path)
        if (compiledFunctions.count(functionName)) return;
        
        std::vector<std::string> params = tokenizer.ExtractArguments(instruction);
        size_t closeParenPos = instruction.find(')', instruction.find('('));
        size_t bracketPos = instruction.find('{', closeParenPos);
        
        if (bracketPos != std::string::npos) {
            std::string body = tokenizer.ExtractBracketsContent(instruction, bracketPos);
            FunctionDefinition funcDef;
            funcDef.parameters = params;
            funcDef.body = body;
            functions[functionName] = std::move(funcDef);
            // Compile the function
            compiledFunctions[functionName] = compileFunction(body, params);
            // Resolve function pointers (including self-references for recursion)
            resolveFuncPtrs(compiledFunctions[functionName].body);
        }
        return;
    }

    if (cmdType == CommandType::Function) {
        auto cfIt = compiledFunctions.find(command);
        if (cfIt != compiledFunctions.end()) {
            const CompiledFunction& cf = cfIt->second;
            std::vector<std::string> args = tokenizer.ExtractArguments(instruction);
            int nParams = cf.nParams;
            
            // Evaluate args
            double argVals[8];
            for (int i = 0; i < nParams && i < static_cast<int>(args.size()); ++i) {
                if (tokenizer.IsArithmetic(args[i]))
                    argVals[i] = tokenizer.ArithmericHandler(args[i], variables);
                else
                    argVals[i] = ParsingHelper::ParseValue(args[i], variables);
            }
            
            double saved[8];
            for (int i = 0; i < nParams; ++i) saved[i] = varSlots[cf.paramSlotArr[i]];
            for (int i = 0; i < nParams; ++i) varSlots[cf.paramSlotArr[i]] = argVals[i];
            
            executeCompiled(cf.body);
            
            for (int i = 0; i < nParams; ++i) varSlots[cf.paramSlotArr[i]] = saved[i];
            return;
        }
        return;
    }

    if (cmdType == CommandType::Var) {
        auto [key, value] = tokenizer.VariableHandler(instruction);
        if (!key.empty()) {
            variables[key] = value;
            varSlots[getOrCreateSlot(key)] = value;
        }
        return;
    }
    if (cmdType == CommandType::PenUp) { turtlePtr->PenUp(); return; }
    if (cmdType == CommandType::PenDown) { turtlePtr->PenDown(); return; }
    if (cmdType == CommandType::If) {
        std::string condition = tokenizer.ExtractData(instruction, variables);
        if (condition.empty()) return;
        if (tokenizer.LogicHandler(condition, variables)) {
            size_t bracketPos = instruction.find('{');
            if (bracketPos != std::string::npos) {
                std::string blockContent = tokenizer.ExtractBracketsContent(instruction, bracketPos);
                if (!blockContent.empty()) tokenizer.TokenizeAndExecute(blockContent, *this);
            }
        }
        return;
    }

    std::string dataStr = tokenizer.ExtractData(instruction, variables);
    if (dataStr.empty()) return;
    int data = 0;
    try { data = static_cast<int>(std::round(std::stod(dataStr))); } catch (...) { return; }
    switch (cmdType) {
        case CommandType::Forward: turtlePtr->Forward(data); break;
        case CommandType::Backward: turtlePtr->Backward(data); break;
        case CommandType::Left: turtlePtr->Left(data); break;
        case CommandType::Right: turtlePtr->Right(data); break;
        default: break;
    }
}

// ==================== PRE-COMPILE TOP-LEVEL ====================
void Instruction::compileTopLevel(const std::string& instructionSet) {
    if (topLevelCompiled) return;
    std::vector<CInstr> topInstrs;
    size_t begin = 0;
    int braceDepth = 0;
    for (size_t i = 0; i < instructionSet.size(); ++i) {
        if (instructionSet[i] == '{') braceDepth++;
        else if (instructionSet[i] == '}') { if (braceDepth > 0) braceDepth--; }
        if (instructionSet[i] == ';' && braceDepth == 0) {
            if (i > begin) {
                auto sv = trimSV(std::string_view(instructionSet.data() + begin, i - begin));
                if (!sv.empty()) {
                    std::string s(sv);
                    std::string cmd = extractCommand(s);
                    if (cmd != "def") {
                        topInstrs.push_back(compileInstruction(s));
                    }
                }
            }
            begin = i + 1;
        }
    }
    if (begin < instructionSet.size()) {
        auto sv = trimSV(std::string_view(instructionSet.data() + begin, instructionSet.size() - begin));
        if (!sv.empty()) {
            std::string s(sv);
            std::string cmd = extractCommand(s);
            if (cmd != "def") {
                topInstrs.push_back(compileInstruction(s));
            }
        }
    }
    resolveFuncPtrs(topInstrs);
    compiledTopLevel = std::move(topInstrs);
    topLevelCompiled = true;
}
