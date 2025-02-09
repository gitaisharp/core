/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LO_CLANG_SHARED_PLUGINS

#include "check.hxx"
#include "plugin.hxx"

#include "config_clang.h"

#include <unordered_set>

/** Look for static O*String and O*String[], they can be more efficiently declared as:

        static constexpr OUStringLiteral our_aLBEntryMap[] = {u" ", u", "};
        static constexpr OUStringLiteral sName(u"name");

    which is more efficient at startup time.
 */
namespace {

class StringStatic
    : public loplugin::FilteringPlugin<StringStatic>
{

public:
    explicit StringStatic(loplugin::InstantiationData const& rData):
        FilteringPlugin(rData) {}

    void run() override;
    bool preRun() override;
    void postRun() override;
    bool VisitVarDecl(VarDecl const*);
    bool VisitReturnStmt(ReturnStmt const*);
    bool VisitDeclRefExpr(DeclRefExpr const*);
    bool VisitMemberExpr(MemberExpr const*);
    bool VisitUnaryOperator(UnaryOperator const*);

private:
    std::unordered_set<VarDecl const *> potentialVars;
    std::unordered_set<VarDecl const *> excludeVars;
};

void StringStatic::run()
{
    if( preRun())
        if( TraverseDecl(compiler.getASTContext().getTranslationUnitDecl()))
            postRun();
}

bool StringStatic::preRun()
{
    StringRef fn(handler.getMainFileName());
    // passing around pointers to global OUString
    if (loplugin::hasPathnamePrefix(fn, SRCDIR "/filter/source/svg/"))
         return false;
    // call replaceAll on a 'const OUString", for which there is no OUStringLiteral equivalent
    if (loplugin::hasPathnamePrefix(fn, SRCDIR "/i18npool/qa/cppunit/test_breakiterator.cxx"))
         return false;
    if (loplugin::hasPathnamePrefix(fn, SRCDIR "/sd/qa/unit/export-tests-ooxml2.cxx"))
         return false;
    return true;
}

void StringStatic::postRun()
{
    for (auto const & pVarDecl : excludeVars) {
        potentialVars.erase(pVarDecl);
    }
    for (auto const & varDecl : potentialVars) {
        report(DiagnosticsEngine::Warning,
                "rather declare this using OUStringLiteral/OStringLiteral/char[]",
                varDecl->getLocation())
            << varDecl->getSourceRange();
    }
}

bool StringStatic::VisitVarDecl(VarDecl const* varDecl)
{
    if (ignoreLocation(varDecl))
        return true;
    QualType qt = varDecl->getType();
    if (!varDecl->isThisDeclarationADefinition()
        || !qt.isConstQualified())
        return true;

    if (varDecl->hasGlobalStorage())
    {
        if (varDecl->hasGlobalStorage() && !varDecl->isStaticLocal()) {
            //TODO: For a non-public static member variable from an included file, we could still
            // examine it further if all its uses must be seen in that included file:
            if (!compiler.getSourceManager().isInMainFile(varDecl->getLocation())) {
                return true;
            }
        }
        if (qt->isArrayType())
            qt = qt->getAsArrayTypeUnsafe()->getElementType();

        auto tc = loplugin::TypeCheck(qt);
        if (!tc.Class("OUString").Namespace("rtl").GlobalNamespace()
            && !tc.Class("OString").Namespace("rtl").GlobalNamespace())
            return true;
        if (varDecl->hasInit())
        {
            Expr const * expr = varDecl->getInit();
            while (true) {
                if (ExprWithCleanups const * exprWithCleanups = dyn_cast<ExprWithCleanups>(expr)) {
                    expr = exprWithCleanups->getSubExpr();
                }
                else if (CastExpr const * castExpr = dyn_cast<CastExpr>(expr)) {
                    expr = castExpr->getSubExpr();
                }
                else if (MaterializeTemporaryExpr const * materializeExpr = dyn_cast<MaterializeTemporaryExpr>(expr)) {
                    expr = materializeExpr->getSubExpr();
                }
                else if (CXXBindTemporaryExpr const * bindExpr = dyn_cast<CXXBindTemporaryExpr>(expr)) {
                    expr = bindExpr->getSubExpr();
                }
                else if (CXXConstructExpr const * constructExpr = dyn_cast<CXXConstructExpr>(expr)) {
                    if (constructExpr->getNumArgs() == 0) {
                        return true;
                    }
                    expr = constructExpr->getArg(0);
                } else if (isa<CallExpr>(expr)) {
                    return true;
                } else {
                    break;
                }
            }
        }
    }
    else
    {
        if (isa<ParmVarDecl>(varDecl))
            return true;
        loplugin::TypeCheck const tc(varDecl->getType());
        if (!(tc.Class("OString").Namespace("rtl").GlobalNamespace()
              || tc.Class("OUString").Namespace("rtl").GlobalNamespace()))
        {
            return true;
        }
        if (varDecl->hasInit())
        {
            auto cxxConstruct = dyn_cast<CXXConstructExpr>(varDecl->getInit()->IgnoreImplicit());
            if (!cxxConstruct || cxxConstruct->getNumArgs() == 0)
                return true;
            if (!isa<clang::StringLiteral>(cxxConstruct->getArg(0)))
                return true;
        }
    }
    potentialVars.insert(varDecl);

    return true;
}

bool StringStatic::VisitReturnStmt(ReturnStmt const * returnStmt)
{
    if (ignoreLocation(returnStmt)) {
        return true;
    }
    if (!returnStmt->getRetValue()) {
        return true;
    }
    DeclRefExpr const * declRef = dyn_cast<DeclRefExpr>(returnStmt->getRetValue());
    if (!declRef) {
        return true;
    }
    VarDecl const * varDecl = dyn_cast<VarDecl>(declRef->getDecl());
    if (varDecl) {
        excludeVars.insert(varDecl);
    }
    return true;
}

bool StringStatic::VisitDeclRefExpr(DeclRefExpr const * declRef)
{
    if (ignoreLocation(declRef))
        return true;
    VarDecl const * varDecl = dyn_cast<VarDecl>(declRef->getDecl());
    if (!varDecl)
        return true;
    if (potentialVars.count(varDecl) == 0)
        return true;
    // ignore globals that are used in CPPUNIT_ASSERT expressions, otherwise we can end up
    // trying to compare an OUStringLiteral and an OUString, and CPPUNIT can't handle that
    auto loc = declRef->getBeginLoc();
    if (compiler.getSourceManager().isMacroArgExpansion(loc))
    {
        StringRef name { Lexer::getImmediateMacroName(loc, compiler.getSourceManager(), compiler.getLangOpts()) };
        if (name.startswith("CPPUNIT_ASSERT"))
            excludeVars.insert(varDecl);
    }
    return true;
}

bool StringStatic::VisitMemberExpr(MemberExpr const * expr)
{
    if (ignoreLocation(expr))
        return true;
    auto const declRef = dyn_cast<DeclRefExpr>(expr->getBase());
    if (declRef == nullptr) {
        return true;
    }
    VarDecl const * varDecl = dyn_cast<VarDecl>(declRef->getDecl());
    if (!varDecl)
        return true;
    if (potentialVars.count(varDecl) == 0)
        return true;
    auto const id = expr->getMemberDecl()->getIdentifier();
    if (id == nullptr || id->getName() != "pData") {
        return true;
    }
    excludeVars.insert(varDecl);
    return true;
}

bool StringStatic::VisitUnaryOperator(UnaryOperator const * expr)
{
    if (ignoreLocation(expr))
        return true;
    if (expr->getOpcode() != UO_AddrOf)
        return true;
    if (this->ignoreLocation(expr))
        return true;
    auto const dre = dyn_cast<DeclRefExpr>(
        expr->getSubExpr()->IgnoreParenImpCasts());
    if (!dre)
        return true;
    auto const vd = dyn_cast<VarDecl>(dre->getDecl());
    if (!vd)
        return true;
    excludeVars.insert(vd->getCanonicalDecl());
    return true;
}

loplugin::Plugin::Registration<StringStatic> stringstatic("stringstatic");

} // namespace

#endif // LO_CLANG_SHARED_PLUGINS

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
