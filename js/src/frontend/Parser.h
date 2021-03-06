/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=78:
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef Parser_h__
#define Parser_h__

/*
 * JS parser definitions.
 */
#include "jsversion.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsscript.h"
#include "jswin.h"

#include "frontend/FoldConstants.h"
#include "frontend/FullParseHandler.h"
#include "frontend/ParseMaps.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"
#include "frontend/SyntaxParseHandler.h"

namespace js {
namespace frontend {

struct StmtInfoPC : public StmtInfoBase {
    StmtInfoPC      *down;          /* info for enclosing statement */
    StmtInfoPC      *downScope;     /* next enclosing lexical scope */

    uint32_t        blockid;        /* for simplified dominance computation */

    StmtInfoPC(JSContext *cx) : StmtInfoBase(cx) {}
};

typedef HashSet<JSAtom *> FuncStmtSet;
class SharedContext;

typedef Vector<Definition *, 16> DeclVector;

/*
 * The struct ParseContext stores information about the current parsing context,
 * which is part of the parser state (see the field Parser::pc). The current
 * parsing context is either the global context, or the function currently being
 * parsed. When the parser encounters a function definition, it creates a new
 * ParseContext, makes it the new current context, and sets its parent to the
 * context in which it encountered the definition.
 */
template <typename ParseHandler>
struct ParseContext                 /* tree context for semantic checks */
{
    typedef StmtInfoPC StmtInfo;
    typedef typename ParseHandler::Node Node;

    SharedContext   *sc;            /* context shared between parsing and bytecode generation */

    uint32_t        bodyid;         /* block number of program/function body */
    uint32_t        blockidGen;     /* preincremented block number generator */

    StmtInfoPC      *topStmt;       /* top of statement info stack */
    StmtInfoPC      *topScopeStmt;  /* top lexical scope statement */
    Rooted<StaticBlockObject *> blockChain;
                                    /* compile time block scope chain */

    const unsigned  staticLevel;    /* static compilation unit nesting level */

    uint32_t        parenDepth;     /* nesting depth of parens that might turn out
                                       to be generator expressions */
    uint32_t        yieldCount;     /* number of |yield| tokens encountered at
                                       non-zero depth in current paren tree */
    Node            blockNode;      /* parse node for a block with let declarations
                                       (block with its own lexical scope)  */
  private:
    AtomDecls       decls_;         /* function, const, and var declarations */
    DeclVector      args_;          /* argument definitions */
    DeclVector      vars_;          /* var/const definitions */

  public:
    const AtomDecls &decls() const {
        return decls_;
    }

    uint32_t numArgs() const {
        JS_ASSERT(sc->isFunctionBox());
        return args_.length();
    }

    uint32_t numVars() const {
        JS_ASSERT(sc->isFunctionBox());
        return vars_.length();
    }

    /*
     * This function adds a definition to the lexical scope represented by this
     * ParseContext.
     *
     * Pre-conditions:
     *  + The caller must have already taken care of name collisions:
     *    - For non-let definitions, this means 'name' isn't in 'decls'.
     *    - For let definitions, this means 'name' isn't already a name in the
     *      current block.
     *  + The given 'pn' is either a placeholder (created by a previous unbound
     *    use) or an un-bound un-linked name node.
     *  + The given 'kind' is one of ARG, CONST, VAR, or LET. In particular,
     *    NAMED_LAMBDA is handled in an ad hoc special case manner (see
     *    LeaveFunction) that we should consider rewriting.
     *
     * Post-conditions:
     *  + pc->decls().lookupFirst(name) == pn
     *  + The given name 'pn' has been converted in-place into a
     *    non-placeholder definition.
     *  + If this is a function scope (sc->inFunction), 'pn' is bound to a
     *    particular local/argument slot.
     *  + PND_CONST is set for Definition::COSNT
     *  + Pre-existing uses of pre-existing placeholders have been linked to
     *    'pn' if they are in the scope of 'pn'.
     *  + Pre-existing placeholders in the scope of 'pn' have been removed.
     */
    bool define(JSContext *cx, HandlePropertyName name, Node pn, Definition::Kind);

    /*
     * Let definitions may shadow same-named definitions in enclosing scopes.
     * To represesent this, 'decls' is not a plain map, but actually:
     *   decls :: name -> stack of definitions
     * New bindings are pushed onto the stack, name lookup always refers to the
     * top of the stack, and leaving a block scope calls popLetDecl for each
     * name in the block's scope.
     */
    void popLetDecl(JSAtom *atom);

    /* See the sad story in defineArg. */
    void prepareToAddDuplicateArg(Definition *prevDecl);

    /* See the sad story in MakeDefIntoUse. */
    void updateDecl(JSAtom *atom, Node newDecl);

    /*
     * After a function body has been parsed, the parser generates the
     * function's "bindings". Bindings are a data-structure, ultimately stored
     * in the compiled JSScript, that serve three purposes:
     *  - After parsing, the ParseContext is destroyed and 'decls' along with
     *    it. Mostly, the emitter just uses the binding information stored in
     *    the use/def nodes, but the emitter occasionally needs 'bindings' for
     *    various scope-related queries.
     *  - Bindings provide the initial js::Shape to use when creating a dynamic
     *    scope object (js::CallObject) for the function. This shape is used
     *    during dynamic name lookup.
     *  - Sometimes a script's bindings are accessed at runtime to retrieve the
     *    contents of the lexical scope (e.g., from the debugger).
     */
    bool generateFunctionBindings(JSContext *cx, InternalHandle<Bindings*> bindings) const;

  public:
    Node             yieldNode;     /* parse node for a yield expression that might
                                       be an error if we turn out to be inside a
                                       generator expression */

  private:
    ParseContext    **parserPC;     /* this points to the Parser's active pc
                                       and holds either |this| or one of
                                       |this|'s descendents */

  public:
    OwnedAtomDefnMapPtr lexdeps;    /* unresolved lexical name dependencies */

    ParseContext     *parent;       /* Enclosing function or global context.  */

    FuncStmtSet     *funcStmts;     /* Set of (non-top-level) function statements
                                       that will alias any top-level bindings with
                                       the same name. */

    // The following flags are set when a particular code feature is detected
    // in a function.
    bool            funHasReturnExpr:1; /* function has 'return <expr>;' */
    bool            funHasReturnVoid:1; /* function has 'return;' */

    // The following flags are set when parsing enters a particular region of
    // source code, and cleared when that region is exited.
    bool            parsingForInit:1;   /* true while parsing init expr of for;
                                           exclude 'in' */
    bool            parsingWith:1;  /* true while we are within a
                                       with-statement in the current
                                       ParseContext chain (which stops at the
                                       top-level or an eval() */

    // Set when parsing a declaration-like destructuring pattern.  This flag
    // causes PrimaryExpr to create PN_NAME parse nodes for variable references
    // which are not hooked into any definition's use chain, added to any tree
    // context's AtomList, etc. etc.  CheckDestructuring will do that work
    // later.
    //
    // The comments atop CheckDestructuring explain the distinction between
    // assignment-like and declaration-like destructuring patterns, and why
    // they need to be treated differently.
    bool            inDeclDestructuring:1;

    // True if we are in a function, saw a "use strict" directive, and weren't
    // strict before.
    bool            funBecameStrict:1;

    inline ParseContext(Parser<ParseHandler> *prs, SharedContext *sc, unsigned staticLevel, uint32_t bodyid);
    inline ~ParseContext();

    inline bool init();

    unsigned blockid();

    // True if we are at the topmost level of a entire script or function body.
    // For example, while parsing this code we would encounter f1 and f2 at
    // body level, but we would not encounter f3 or f4 at body level:
    //
    //   function f1() { function f2() { } }
    //   if (cond) { function f3() { if (cond) { function f4() { } } } }
    //
    bool atBodyLevel();

    inline bool useAsmOrInsideUseAsm() const {
        return sc->isFunctionBox() && sc->asFunctionBox()->useAsmOrInsideUseAsm();
    }
};

template <typename ParseHandler>
bool
GenerateBlockId(ParseContext<ParseHandler> *pc, uint32_t &blockid);

template <typename ParseHandler>
struct BindData;

class CompExprTransplanter;

template <typename ParseHandler>
class GenexpGuard;

// XXX fix this backdoor.
bool EmitElemOp(JSContext *cx, ParseNode *pn, JSOp op, BytecodeEmitter *bce);

enum LetContext { LetExpresion, LetStatement };
enum VarContext { HoistVars, DontHoistVars };

template <typename ParseHandler>
struct Parser : private AutoGCRooter, public StrictModeGetter
{
    JSContext           *const context; /* FIXME Bug 551291: use AutoGCRooter::context? */
    TokenStream         tokenStream;
    void                *tempPoolMark;  /* initial JSContext.tempLifoAlloc mark */

    /* list of parsed objects for GC tracing */
    ObjectBox *traceListHead;

    /* innermost parse context (stack-allocated) */
    ParseContext<ParseHandler> *pc;

    SourceCompressionToken *sct;        /* compression token for aborting */

    /* Root atoms and objects allocated for the parsed tree. */
    AutoKeepAtoms       keepAtoms;

    /* Perform constant-folding; must be true when interfacing with the emitter. */
    const bool          foldConstants:1;

  private:
    /* Script can optimize name references based on scope chain. */
    const bool          compileAndGo:1;

    /*
     * In self-hosting mode, scripts emit JSOP_CALLINTRINSIC instead of
     * JSOP_NAME or JSOP_GNAME to access unbound variables. JSOP_CALLINTRINSIC
     * does a name lookup in a special object that contains properties
     * installed during global initialization and that properties from
     * self-hosted scripts get copied into lazily upon first access in a
     * global.
     * As that object is inaccessible to client code, the lookups are
     * guaranteed to return the original objects, ensuring safe implementation
     * of self-hosted builtins.
     * Additionally, the special syntax _CallFunction(receiver, ...args, fun)
     * is supported, for which bytecode is emitted that invokes |fun| with
     * |receiver| as the this-object and ...args as the arguments..
     */
    const bool          selfHostingMode:1;

    /*
     * Not all language constructs can be handled during syntax parsing. If it
     * is not known whether the parse succeeds or fails, this bit is set and
     * the parse will return false.
     */
    bool unknownResult;

    typedef typename ParseHandler::Node Node;
    typedef typename ParseHandler::DefinitionNode DefinitionNode;

  public:
    /* State specific to the kind of parse being performed. */
    ParseHandler handler;

    bool report(ParseReportKind kind, bool strict, Node pn, unsigned errorNumber, ...);

    Parser(JSContext *cx, const CompileOptions &options,
           const jschar *chars, size_t length, bool foldConstants);
    ~Parser();

    friend void AutoGCRooter::trace(JSTracer *trc);

    /*
     * Initialize a parser. The compiler owns the arena pool "tops-of-stack"
     * space above the current JSContext.tempLifoAlloc mark. This means you
     * cannot allocate from tempLifoAlloc and save the pointer beyond the next
     * Parser destructor invocation.
     */
    bool init();

    const char *getFilename() const { return tokenStream.getFilename(); }
    JSVersion versionNumber() const { return tokenStream.versionNumber(); }

    /*
     * Parse a top-level JS script.
     */
    Node parse(JSObject *chain);

    /*
     * Allocate a new parsed object or function container from
     * cx->tempLifoAlloc.
     */
    ObjectBox *newObjectBox(JSObject *obj);
    ModuleBox *newModuleBox(Module *module, ParseContext<ParseHandler> *pc);
    FunctionBox *newFunctionBox(JSFunction *fun, ParseContext<ParseHandler> *pc, bool strict);

    /*
     * Create a new function object given parse context (pc) and a name (which
     * is optional if this is a function expression).
     */
    JSFunction *newFunction(ParseContext<ParseHandler> *pc, HandleAtom atom, FunctionSyntaxKind kind);

    void trace(JSTracer *trc);

    bool hadUnknownResult() {
        return unknownResult;
    }

  private:
    Parser *thisForCtor() { return this; }

    /*
     * Create a parse node with the given kind and op using the current token's
     * atom.
     */
    Node atomNode(ParseNodeKind kind, JSOp op);

    void setUnknownResult() {
        unknownResult = true;
    }

  public:

    /* Public entry points for parsing. */
    Node statement();
    bool maybeParseDirective(Node pn, bool *cont);

    // Parse a function, given only its body. Used for the Function constructor.
    Node standaloneFunctionBody(HandleFunction fun, const AutoNameVector &formals, HandleScript script,
                                Node fn, FunctionBox **funbox, bool strict,
                                bool *becameStrict = NULL);

    /*
     * Parse a function body.  Pass StatementListBody if the body is a list of
     * statements; pass ExpressionBody if the body is a single expression.
     */
    enum FunctionBodyType { StatementListBody, ExpressionBody };
    Node functionBody(FunctionSyntaxKind kind, FunctionBodyType type);

    virtual bool strictMode() { return pc->sc->strict; }

  private:
    /*
     * JS parsers, from lowest to highest precedence.
     *
     * Each parser must be called during the dynamic scope of a ParseContext
     * object, pointed to by this->pc.
     *
     * Each returns a parse node tree or null on error.
     *
     * Parsers whose name has a '1' suffix leave the TokenStream state
     * pointing to the token one past the end of the parsed fragment.  For a
     * number of the parsers this is convenient and avoids a lot of
     * unnecessary ungetting and regetting of tokens.
     *
     * Some parsers have two versions:  an always-inlined version (with an 'i'
     * suffix) and a never-inlined version (with an 'n' suffix).
     */
    Node moduleDecl();
    Node functionStmt();
    Node functionExpr();
    Node statements(bool *hasFunctionStmt = NULL);

    Node switchStatement();
    Node forStatement();
    Node tryStatement();
    Node withStatement();
#if JS_HAS_BLOCK_SCOPE
    Node letStatement();
#endif
    Node expressionStatement();
    Node variables(ParseNodeKind kind, bool *psimple = NULL,
                   StaticBlockObject *blockObj = NULL,
                   VarContext varContext = HoistVars);
    Node expr();
    Node assignExpr();
    Node assignExprWithoutYield(unsigned err);
    Node condExpr1();
    Node orExpr1();
    Node unaryExpr();
    Node memberExpr(TokenKind tt, bool allowCallSyntax);
    Node primaryExpr(TokenKind tt);
    Node parenExpr(bool *genexp = NULL);

    /*
     * Additional JS parsers.
     */
    enum FunctionType { Getter, Setter, Normal };
    bool functionArguments(FunctionSyntaxKind kind, Node *list, Node funcpn, bool &hasRest);

    Node functionDef(HandlePropertyName name, const TokenStream::Position &start,
                     size_t startOffset, FunctionType type, FunctionSyntaxKind kind);
    bool functionArgsAndBody(Node pn, HandleFunction fun, HandlePropertyName funName,
                             size_t startOffset, FunctionType type, FunctionSyntaxKind kind,
                             bool strict, bool *becameStrict = NULL);

    Node unaryOpExpr(ParseNodeKind kind, JSOp op);

    Node condition();
    Node comprehensionTail(Node kid, unsigned blockid, bool isGenexp,
                               ParseNodeKind kind = PNK_SEMI, JSOp op = JSOP_NOP);
    bool arrayInitializerComprehensionTail(Node pn);
    Node generatorExpr(Node kid);
    bool argumentList(Node listNode);
    Node bracketedExpr();
    Node letBlock(LetContext letContext);
    Node returnOrYield(bool useAssignExpr);
    Node destructuringExpr(BindData<ParseHandler> *data, TokenKind tt);

    Node identifierName();

    bool allowsForEachIn() {
#if !JS_HAS_FOR_EACH_IN
        return false;
#else
        return versionNumber() >= JSVERSION_1_6;
#endif
    }

    bool setAssignmentLhsOps(Node pn, JSOp op);
    bool matchInOrOf(bool *isForOfp);

    void addStatementToList(Node pn, Node kid, bool *hasFunctionStmt);
    bool checkFunctionArguments();
    bool makeDefIntoUse(Definition *dn, Node pn, JSAtom *atom);
    bool checkFunctionDefinition(HandlePropertyName funName, Node *pn, FunctionSyntaxKind kind);
    bool finishFunctionDefinition(Node pn, FunctionBox *funbox,
                                  Node prelude, Node body,
                                  ParseContext<ParseHandler> *outerpc);

    bool isValidForStatementLHS(Node pn1, JSVersion version,
                                bool forDecl, bool forEach, bool forOf);
    bool setLvalKid(Node pn, Node kid, const char *name);
    bool setIncOpKid(Node pn, Node kid, TokenKind tt, bool preorder);
    bool checkStrictAssignment(Node lhs);
    bool checkStrictBinding(HandlePropertyName name, Node pn);
    bool checkDeleteExpression(Node *pn);
    bool defineArg(Node funcpn, HandlePropertyName name,
                   bool disallowDuplicateArgs = false, DefinitionNode *duplicatedArg = NULL);
    Node pushLexicalScope(StmtInfoPC *stmt);
    Node pushLexicalScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    Node pushLetScope(Handle<StaticBlockObject*> blockObj, StmtInfoPC *stmt);
    bool noteNameUse(Node pn);
    Node newRegExp(const jschar *chars, size_t length, RegExpFlag flags);
    Node newBindingNode(PropertyName *name, VarContext varContext = HoistVars);
    bool checkDestructuring(BindData<ParseHandler> *data, Node left, bool toplevel = true);
    bool bindDestructuringVar(BindData<ParseHandler> *data, Node pn);
    bool bindDestructuringLHS(Node pn);
    bool makeSetCall(Node pn, unsigned msg);
    PropertyName *foldPropertyByValue(Node pn);
    Node cloneLeftHandSide(Node opn);
    Node cloneParseTree(Node opn);

    static bool
    bindDestructuringArg(JSContext *cx, BindData<ParseHandler> *data,
                         HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindLet(JSContext *cx, BindData<ParseHandler> *data,
            HandlePropertyName name, Parser<ParseHandler> *parser);

    static bool
    bindVarOrConst(JSContext *cx, BindData<ParseHandler> *data,
                   HandlePropertyName name, Parser<ParseHandler> *parser);

    static DefinitionNode null() { return ParseHandler::null(); }

    bool reportRedeclaration(Node pn, bool isConst, JSAtom *atom);
    bool reportBadReturn(Node pn, ParseReportKind kind, unsigned errnum, unsigned anonerrnum);
    bool checkFinalReturn(Node pn);

    bool leaveFunction(Node fn, HandlePropertyName funName,
                       FunctionSyntaxKind kind = Expression);

    friend class CompExprTransplanter;
    friend class GenexpGuard<ParseHandler>;
    friend struct BindData<ParseHandler>;
};

/* Declare some required template specializations. */

template <>
ParseNode *
Parser<FullParseHandler>::expr();

template <>
SyntaxParseHandler::Node
Parser<SyntaxParseHandler>::expr();

template <>
bool
Parser<FullParseHandler>::setAssignmentLhsOps(ParseNode *pn, JSOp op);

template <>
bool
Parser<SyntaxParseHandler>::setAssignmentLhsOps(Node pn, JSOp op);

} /* namespace frontend */
} /* namespace js */

/*
 * Convenience macro to access Parser.tokenStream as a pointer.
 */
#define TS(p) (&(p)->tokenStream)

#endif /* Parser_h__ */
