import qbs.base 1.0
import "../QtcLibrary.qbs" as QtcLibrary

QtcLibrary {
    name: "CPlusPlus"

    cpp.includePaths: base.concat("../3rdparty")
    cpp.defines: base.concat([
        "NDEBUG",
        "CPLUSPLUS_BUILD_LIB"
    ])
    cpp.optimization: "fast"

    Depends { name: "cpp" }
    Depends { name: "Qt.widgets" }

    Group {
        prefix: "../3rdparty/cplusplus/"
        files: [
            "AST.cpp",
            "AST.h",
            "ASTClone.cpp",
            "ASTMatch0.cpp",
            "ASTMatcher.cpp",
            "ASTMatcher.h",
            "ASTPatternBuilder.cpp",
            "ASTPatternBuilder.h",
            "ASTVisit.cpp",
            "ASTVisitor.cpp",
            "ASTVisitor.h",
            "ASTfwd.h",
            "Bind.cpp",
            "Bind.h",
            "CPlusPlus.h",
            "Control.cpp",
            "Control.h",
            "CoreTypes.cpp",
            "CoreTypes.h",
            "DiagnosticClient.cpp",
            "DiagnosticClient.h",
            "FullySpecifiedType.cpp",
            "FullySpecifiedType.h",
            "Keywords.cpp",
            "Lexer.cpp",
            "Lexer.h",
            "LiteralTable.cpp",
            "LiteralTable.h",
            "Literals.cpp",
            "Literals.h",
            "MemoryPool.cpp",
            "MemoryPool.h",
            "Name.cpp",
            "Name.h",
            "NameVisitor.cpp",
            "NameVisitor.h",
            "Names.cpp",
            "Names.h",
            "ObjectiveCAtKeywords.cpp",
            "ObjectiveCTypeQualifiers.cpp",
            "ObjectiveCTypeQualifiers.h",
            "Parser.cpp",
            "Parser.h",
            "QtContextKeywords.cpp",
            "QtContextKeywords.h",
            "Scope.cpp",
            "Scope.h",
            "Symbol.cpp",
            "Symbol.h",
            "SymbolVisitor.cpp",
            "SymbolVisitor.h",
            "Symbols.cpp",
            "Symbols.h",
            "Templates.cpp",
            "Templates.h",
            "Token.cpp",
            "Token.h",
            "TranslationUnit.cpp",
            "TranslationUnit.h",
            "Type.cpp",
            "Type.h",
            "TypeMatcher.cpp",
            "TypeMatcher.h",
            "TypeVisitor.cpp",
            "TypeVisitor.h",
        ]
    }

    files: [
        "AlreadyConsideredClassContainer.h",
        "ASTParent.cpp",
        "ASTParent.h",
        "ASTPath.cpp",
        "ASTPath.h",
        "BackwardsScanner.cpp",
        "BackwardsScanner.h",
        "CppDocument.cpp",
        "CppDocument.h",
        "CppRewriter.cpp",
        "CppRewriter.h",
        "DependencyTable.cpp",
        "DependencyTable.h",
        "DeprecatedGenTemplateInstance.cpp",
        "DeprecatedGenTemplateInstance.h",
        "ExpressionUnderCursor.cpp",
        "ExpressionUnderCursor.h",
        "FastPreprocessor.cpp",
        "FastPreprocessor.h",
        "FindUsages.cpp",
        "FindUsages.h",
        "Icons.cpp",
        "Icons.h",
        "LookupContext.cpp",
        "LookupContext.h",
        "LookupItem.cpp",
        "LookupItem.h",
        "Macro.cpp",
        "Macro.h",
        "MatchingText.cpp",
        "MatchingText.h",
        "NamePrettyPrinter.cpp",
        "NamePrettyPrinter.h",
        "Overview.cpp",
        "Overview.h",
        "OverviewModel.cpp",
        "OverviewModel.h",
        "PPToken.cpp",
        "PPToken.h",
        "PreprocessorClient.cpp",
        "PreprocessorClient.h",
        "PreprocessorEnvironment.cpp",
        "PreprocessorEnvironment.h",
        "ResolveExpression.cpp",
        "ResolveExpression.h",
        "SimpleLexer.cpp",
        "SimpleLexer.h",
        "SnapshotSymbolVisitor.cpp",
        "SnapshotSymbolVisitor.h",
        "SymbolNameVisitor.cpp",
        "SymbolNameVisitor.h",
        "TypeOfExpression.cpp",
        "TypeOfExpression.h",
        "TypePrettyPrinter.cpp",
        "TypePrettyPrinter.h",
        "cplusplus.qrc",
        "findcdbbreakpoint.cpp",
        "findcdbbreakpoint.h",
        "pp-cctype.h",
        "pp-engine.cpp",
        "pp-engine.h",
        "pp-scanner.cpp",
        "pp-scanner.h",
        "pp.h",
        "images/class.png",
        "images/enum.png",
        "images/enumerator.png",
        "images/func.png",
        "images/func_priv.png",
        "images/func_prot.png",
        "images/keyword.png",
        "images/macro.png",
        "images/namespace.png",
        "images/signal.png",
        "images/slot.png",
        "images/slot_priv.png",
        "images/slot_prot.png",
        "images/var.png",
        "images/var_priv.png",
        "images/var_prot.png",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            "../3rdparty"
        ]
    }
}
