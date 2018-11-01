/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "compilationdatabaseproject.h"

#include "compilationdatabaseconstants.h"
#include "compilationdatabaseutils.h"

#include <coreplugin/icontext.h>
#include <cpptools/projectinfo.h>
#include <cpptools/cppprojectupdater.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchainconfigwidget.h>
#include <projectexplorer/toolchainmanager.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

using namespace ProjectExplorer;

namespace CompilationDatabaseProjectManager {
namespace Internal {

namespace {
class DBProjectNode : public ProjectNode
{
public:
    explicit DBProjectNode(const Utils::FileName &projectFilePath)
        : ProjectNode(projectFilePath)
    {}
};

QStringList jsonObjectFlags(const QJsonObject &object)
{
    QStringList flags;
    const QJsonArray arguments = object["arguments"].toArray();
    if (arguments.isEmpty()) {
        flags = splitCommandLine(object["command"].toString());
    } else {
        for (const QJsonValue &arg : arguments)
            flags.append(arg.toString());
    }

    return flags;
}

bool isGccCompiler(const QString &compilerName)
{
    return compilerName.contains("gcc") || compilerName.contains("g++");
}

Core::Id getCompilerId(QString compilerName)
{
    if (Utils::HostOsInfo::isWindowsHost()) {
        if (compilerName.endsWith(".exe"))
            compilerName.chop(4);
        if (isGccCompiler(compilerName))
            return ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID;

        // Default is clang-cl
        return ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID;
    }
    if (isGccCompiler(compilerName))
        return ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID;

    // Default is clang
    return ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
}

ToolChain *toolchainFromCompilerId(const Core::Id &compilerId, const Core::Id &language)
{
    return ToolChainManager::toolChain([&compilerId, &language](const ToolChain *tc) {
        if (!tc->isValid() || tc->language() != language)
            return false;
        return tc->typeId() == compilerId;
    });
}

QString compilerPath(QString pathFlag)
{
    if (pathFlag.isEmpty())
        return pathFlag;
#ifdef Q_OS_WIN
    // Handle short DOS style file names (cmake can generate them).
    const DWORD pathLength = GetLongPathNameW((LPCWSTR)pathFlag.utf16(), 0, 0);
    wchar_t* buffer = new wchar_t[pathLength];
    GetLongPathNameW((LPCWSTR)pathFlag.utf16(), buffer, pathLength);
    pathFlag = QString::fromUtf16((ushort *)buffer, pathLength - 1);
    delete[] buffer;
#endif
    return QDir::fromNativeSeparators(pathFlag);
}

ToolChain *toolchainFromFlags(const Kit *kit, const QStringList &flags, const Core::Id &language)
{
    if (flags.empty())
        return ToolChainKitInformation::toolChain(kit, language);

    // Try exact compiler match.
    const Utils::FileName compiler = Utils::FileName::fromString(compilerPath(flags.front()));
    ToolChain *toolchain = ToolChainManager::toolChain([&compiler, &language](const ToolChain *tc) {
        return tc->isValid() && tc->language() == language && tc->compilerCommand() == compiler;
    });
    if (toolchain)
        return toolchain;

    Core::Id compilerId = getCompilerId(compiler.fileName());
    if ((toolchain = toolchainFromCompilerId(compilerId, language)))
        return toolchain;

    if (compilerId != ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID &&
            compilerId != ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID) {
        compilerId = Utils::HostOsInfo::isWindowsHost()
                ? ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
                : ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID;
        if ((toolchain = toolchainFromCompilerId(compilerId, language)))
            return toolchain;
    }

    toolchain = ToolChainKitInformation::toolChain(kit, language);
    qWarning() << QCoreApplication::translate("CompilationDatabaseProject",
                                              "No matching toolchain found, use the default.");
    return toolchain;
}

Utils::FileName jsonObjectFilename(const QJsonObject &object)
{
    const QString workingDir = object["directory"].toString();
    Utils::FileName fileName = Utils::FileName::fromString(
                QDir::fromNativeSeparators(object["file"].toString()));
    if (fileName.toFileInfo().isRelative()) {
        fileName = Utils::FileUtils::canonicalPath(
                    Utils::FileName::fromString(workingDir + "/" + fileName.toString()));
    }
    return fileName;
}

void addDriverModeFlagIfNeeded(const ToolChain *toolchain, QStringList &flags)
{
    if (toolchain->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
            && !flags.empty() && !flags.front().endsWith("cl")
            && !flags.front().endsWith("cl.exe")) {
        flags.insert(1, "--driver-mode=g++");
    }
}

CppTools::RawProjectPart makeRawProjectPart(const Utils::FileName &projectFile,
                                            Kit *kit,
                                            ToolChain *&cToolchain,
                                            ToolChain *&cxxToolchain,
                                            const QString &workingDir,
                                            const Utils::FileName &fileName,
                                            QStringList flags)
{
    HeaderPaths headerPaths;
    Macros macros;
    CppTools::ProjectFile::Kind fileKind = CppTools::ProjectFile::Unclassified;

    const QStringList originalFlags = flags;
    filteredFlags(fileName.fileName(),
                  workingDir,
                  flags,
                  headerPaths,
                  macros,
                  fileKind);

    CppTools::RawProjectPart rpp;
    rpp.setProjectFileLocation(projectFile.toString());
    rpp.setBuildSystemTarget(workingDir);
    rpp.setDisplayName(fileName.fileName());
    rpp.setFiles({fileName.toString()});
    rpp.setHeaderPaths(headerPaths);
    rpp.setMacros(macros);

    if (fileKind == CppTools::ProjectFile::Kind::CHeader
            || fileKind == CppTools::ProjectFile::Kind::CSource) {
        if (!cToolchain) {
            cToolchain = toolchainFromFlags(kit, originalFlags,
                                            ProjectExplorer::Constants::C_LANGUAGE_ID);
            ToolChainKitInformation::setToolChain(kit, cToolchain);
        }
        addDriverModeFlagIfNeeded(cToolchain, flags);
        rpp.setFlagsForC({cToolchain, flags});
    } else {
        if (!cxxToolchain) {
            cxxToolchain = toolchainFromFlags(kit, originalFlags,
                                              ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            ToolChainKitInformation::setToolChain(kit, cxxToolchain);
        }
        addDriverModeFlagIfNeeded(cxxToolchain, flags);
        rpp.setFlagsForCxx({cxxToolchain, flags});
    }

    return rpp;
}

} // anonymous namespace

void CompilationDatabaseProject::buildTreeAndProjectParts(const Utils::FileName &projectFile)
{
    QFile file(projectFilePath().toString());
    if (!file.open(QIODevice::ReadOnly)) {
        emitParsingFinished(false);
        return;
    }

    const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();

    auto root = std::make_unique<DBProjectNode>(projectDirectory());
    root->addNode(std::make_unique<FileNode>(
                      projectFile,
                      FileType::Project,
                      false));
    auto headers = std::make_unique<VirtualFolderNode>(
                Utils::FileName::fromString("Headers"), 0);
    auto sources = std::make_unique<VirtualFolderNode>(
                Utils::FileName::fromString("Sources"), 0);

    CppTools::RawProjectParts rpps;
    ToolChain *cToolchain = nullptr;
    ToolChain *cxxToolchain = nullptr;
    for (const QJsonValue &element : array) {
        const QJsonObject object = element.toObject();

        Utils::FileName fileName = jsonObjectFilename(object);
        const QStringList flags = jsonObjectFlags(object);
        const QString filePath = fileName.toString();

        const CppTools::ProjectFile::Kind kind = CppTools::ProjectFile::classify(filePath);
        FolderNode *parent = nullptr;
        FileType type = FileType::Unknown;
        if (CppTools::ProjectFile::isHeader(kind)) {
            parent = headers.get();
            type = FileType::Header;
        } else if (CppTools::ProjectFile::isSource(kind)) {
            parent = sources.get();
            type = FileType::Source;
        } else {
            parent = root.get();
        }
        parent->addNode(std::make_unique<FileNode>(fileName, type, false));

        CppTools::RawProjectPart rpp = makeRawProjectPart(projectFile,
                                                          m_kit.get(),
                                                          cToolchain,
                                                          cxxToolchain,
                                                          object["directory"].toString(),
                                                          fileName,
                                                          flags);
        int rppIndex = Utils::indexOf(rpps, [&rpp](const CppTools::RawProjectPart &currentRpp) {
            return rpp.buildSystemTarget == currentRpp.buildSystemTarget
                    && rpp.headerPaths == currentRpp.headerPaths
                    && rpp.projectMacros == currentRpp.projectMacros
                    && rpp.flagsForCxx.commandLineFlags == currentRpp.flagsForCxx.commandLineFlags;
        });
        if (rppIndex == -1)
            rpps.append(rpp);
        else
            rpps[rppIndex].files.append(rpp.files);
    }

    root->addNode(std::move(headers));
    root->addNode(std::move(sources));

    setRootProjectNode(std::move(root));

    addTarget(createTarget(m_kit.get()));

    m_cppCodeModelUpdater->update({this, cToolchain, cxxToolchain, m_kit.get(), rpps});

    emitParsingFinished(true);
}

CompilationDatabaseProject::CompilationDatabaseProject(const Utils::FileName &projectFile)
    : Project(Constants::COMPILATIONDATABASEMIMETYPE, projectFile)
    , m_cppCodeModelUpdater(std::make_unique<CppTools::CppProjectUpdater>(this))
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(projectDirectory().fileName());
    setRequiredKitPredicate([](const Kit *) { return false; });
    setPreferredKitPredicate([](const Kit *) { return false; });

    m_kit.reset(KitManager::defaultKit()->clone());

    emitParsingStarted();

    const QFuture<void> future = ::Utils::runAsync([this, projectFile](){
        buildTreeAndProjectParts(projectFile);
    });
    m_parserWatcher.setFuture(future);
}

CompilationDatabaseProject::~CompilationDatabaseProject()
{
    m_parserWatcher.cancel();
    m_parserWatcher.waitForFinished();
}

static TextEditor::TextDocument *createCompilationDatabaseDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    doc->setMimeType(Constants::COMPILATIONDATABASEMIMETYPE);
    return doc;
}

CompilationDatabaseEditorFactory::CompilationDatabaseEditorFactory()
{
    setId(Constants::COMPILATIONDATABASEPROJECT_ID);
    setDisplayName("Compilation Database");
    addMimeType(Constants::COMPILATIONDATABASEMIMETYPE);

    setEditorCreator([]() { return new TextEditor::BaseTextEditor; });
    setEditorWidgetCreator([]() { return new TextEditor::TextEditorWidget; });
    setDocumentCreator(createCompilationDatabaseDocument);
    setUseGenericHighlighter(true);
    setCommentDefinition(Utils::CommentDefinition::HashStyle);
    setCodeFoldingSupported(true);
}

} // namespace Internal
} // namespace CompilationDatabaseProjectManager