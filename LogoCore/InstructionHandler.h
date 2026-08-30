#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>

class Turtle;
class Tokenizer;

// ==================== SLOT-BASED VARIABLE SYSTEM ====================
// Variables stored in flat array indexed by slot number
// Much faster than unordered_map for the hot path

static constexpr int MAX_VARS = 64;
static constexpr int MAX_STACK = 256;

// ==================== COMPILED EXPRESSION ====================
struct CExpr {
    enum Type : uint8_t {
        LITERAL, SLOT,
        SLOT_ADD_LIT, SLOT_SUB_LIT, SLOT_MUL_LIT,  // common specializations
        ADD, SUB, MUL  // generic binary ops
    } type = LITERAL;
    double literal = 0.0;
    int slot = -1;           // for SLOT type and SLOT_*_LIT types
    
    // For generic binary ops only
    int leftSlot = -1;
    double leftLiteral = 0.0;
    int rightSlot = -1;
    double rightLiteral = 0.0;
    
    __forceinline double eval(const double* __restrict vars) const noexcept {
        switch (type) {
            case LITERAL: return literal;
            case SLOT: return vars[slot];
            case SLOT_MUL_LIT: return vars[slot] * literal;
            case SLOT_SUB_LIT: return vars[slot] - literal;
            case SLOT_ADD_LIT: return vars[slot] + literal;
            case ADD: {
                double l = (leftSlot >= 0) ? vars[leftSlot] : leftLiteral;
                double r = (rightSlot >= 0) ? vars[rightSlot] : rightLiteral;
                return l + r;
            }
            case SUB: {
                double l = (leftSlot >= 0) ? vars[leftSlot] : leftLiteral;
                double r = (rightSlot >= 0) ? vars[rightSlot] : rightLiteral;
                return l - r;
            }
            case MUL: {
                double l = (leftSlot >= 0) ? vars[leftSlot] : leftLiteral;
                double r = (rightSlot >= 0) ? vars[rightSlot] : rightLiteral;
                return l * r;
            }
        }
        return 0.0;
    }
};

// ==================== COMPILED CONDITION ====================
struct CCondition {
    CExpr left, right;
    enum Op : uint8_t { GT, LT, GE, LE, EQ, NE } op = GT;
    
    __forceinline bool eval(const double* __restrict vars) const noexcept {
        double l = left.eval(vars);
        double r = right.eval(vars);
        switch (op) {
            case GT: return l > r;
            case LT: return l < r;
            case GE: return l >= r;
            case LE: return l <= r;
            case EQ: return l == r;
            case NE: return l != r;
        }
        return false;
    }
};

// ==================== COMPILED INSTRUCTION ====================
enum class CommandType : uint8_t {
    Unknown, Forward, Backward, Left, Right,
    PenUp, PenDown, Var, If, Def, Function
};

struct CompiledFunction; // forward declaration

struct CInstr {
    CommandType type = CommandType::Unknown;
    CExpr arg;
    std::string funcName;                // for func ptr resolution (only used during compilation)
    const CompiledFunction* funcPtr = nullptr; // cached pointer for Function
    CExpr* callArgs = nullptr;           // heap-allocated call args (Function only)
    int nCallArgs = 0;
    CCondition* condition = nullptr;     // heap-allocated condition (If only)
    std::vector<CInstr> ifBody;
    int varSlot = -1;                    // for var assignment
    
    CInstr() = default;
    CInstr(const CInstr& o) : type(o.type), arg(o.arg),
        funcName(o.funcName), funcPtr(o.funcPtr),
        nCallArgs(o.nCallArgs), ifBody(o.ifBody), varSlot(o.varSlot) {
        if (o.callArgs && o.nCallArgs > 0) {
            callArgs = new CExpr[o.nCallArgs];
            for (int i = 0; i < o.nCallArgs; ++i) callArgs[i] = o.callArgs[i];
        }
        if (o.condition) {
            condition = new CCondition(*o.condition);
        }
    }
    CInstr(CInstr&& o) noexcept : type(o.type), arg(o.arg),
        funcName(std::move(o.funcName)), funcPtr(o.funcPtr),
        callArgs(o.callArgs), nCallArgs(o.nCallArgs),
        condition(o.condition), ifBody(std::move(o.ifBody)), varSlot(o.varSlot) {
        o.callArgs = nullptr;
        o.condition = nullptr;
    }
    CInstr& operator=(const CInstr& o) {
        if (this == &o) return *this;
        delete[] callArgs; delete condition;
        type = o.type; arg = o.arg;
        funcName = o.funcName; funcPtr = o.funcPtr;
        nCallArgs = o.nCallArgs; ifBody = o.ifBody; varSlot = o.varSlot;
        callArgs = nullptr; condition = nullptr;
        if (o.callArgs && o.nCallArgs > 0) {
            callArgs = new CExpr[o.nCallArgs];
            for (int i = 0; i < o.nCallArgs; ++i) callArgs[i] = o.callArgs[i];
        }
        if (o.condition) condition = new CCondition(*o.condition);
        return *this;
    }
    CInstr& operator=(CInstr&& o) noexcept {
        if (this == &o) return *this;
        delete[] callArgs; delete condition;
        type = o.type; arg = o.arg;
        funcName = std::move(o.funcName); funcPtr = o.funcPtr;
        callArgs = o.callArgs; nCallArgs = o.nCallArgs;
        condition = o.condition; ifBody = std::move(o.ifBody); varSlot = o.varSlot;
        o.callArgs = nullptr; o.condition = nullptr;
        return *this;
    }
    ~CInstr() { delete[] callArgs; delete condition; }
};

// ==================== COMPILED FUNCTION ====================
struct CompiledFunction {
    std::vector<std::string> paramNames;
    int paramSlotArr[8] = {};
    int nParams = 0;
    std::vector<CInstr> body;
    
    // Early exit optimization: if body is single-If with condition on a param vs literal
    bool hasEarlyExit = false;
    int earlyExitArgIdx = -1;       // which call arg to check
    CCondition::Op earlyExitOp = CCondition::GT;
    double earlyExitLiteral = 0.0;  // RHS literal
    
    // Cached ifBody pointer for early exit (avoids vector indirection)
    const CInstr* earlyExitBodyPtr = nullptr;
    int earlyExitBodySize = 0;
};

// Legacy function definition
struct FunctionDefinition {
    std::vector<std::string> parameters;
    std::string body;
};

// ==================== INSTRUCTION HANDLER ====================
class Instruction {
public:
    Instruction(Turtle& turtle);
    void Execute(const std::string& instructionSet);
    void ExecuteTopLevel(); // Execute pre-compiled top-level instructions directly
    void HandleInstruction(const std::string& instruction, Tokenizer& tokenizer);
    CommandType GetCommandType(const std::string& command) const;
    void setTurtle(Turtle& t) { turtlePtr = &t; }
    void resetVarSlots() { std::memset(varSlots, 0, sizeof(varSlots)); }
    
    // Pre-compiled top-level instructions (skip tokenizer on re-runs)
    std::vector<CInstr> compiledTopLevel;
    bool topLevelCompiled = false;
    void compileTopLevel(const std::string& instructionSet);

    // JIT: native entry point for the top-level program
    // (void fn(Turtle*, double* vars, double* args)), null when not compiled
    void* jitEntry = nullptr;
    bool jitCompile(); // one-shot; keeps bytecode executor as fallback

    // Slot-based variable storage (cache-line aligned for hot path access)
    alignas(64) double varSlots[MAX_VARS] = {};
    std::unordered_map<std::string, int> varNameToSlot;
    int nextSlot = 0;
    
    int getOrCreateSlot(const std::string& name);
    
    std::unordered_map<std::string, double> variables; // legacy for top-level
    std::unordered_map<std::string, FunctionDefinition> functions;
    std::unordered_map<std::string, CompiledFunction> compiledFunctions;
    
    // Compiled execution
    void executeCompiled(const std::vector<CInstr>& instructions);
    void executeCompiledRaw(const CInstr* instructions, int count);
    void executeIterative(const CInstr* instructions, int count); // self-recursive optimizer
    
private:
    Turtle* turtlePtr;
    
    static const std::unordered_map<std::string, CommandType> commandLookup;
    static std::unordered_map<std::string, CommandType> InitCommandLookup();
    
    // Compilation
    CompiledFunction compileFunction(const std::string& body, const std::vector<std::string>& params);
    std::vector<CInstr> compileBlock(const std::string& block);
    CInstr compileInstruction(const std::string& instruction);
    CExpr compileExpr(const std::string& expr);
    CCondition compileCondition(const std::string& condStr);
    void resolveFuncPtrs(std::vector<CInstr>& instrs);
    
    void executeCompiledInstr(const CInstr& instr);
};
