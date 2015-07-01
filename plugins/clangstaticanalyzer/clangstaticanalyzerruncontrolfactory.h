/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise ClangStaticAnalyzer Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERRUNCONTROLFACTORY_H
#define CLANGSTATICANALYZERRUNCONTROLFACTORY_H

#include "clangstaticanalyzertool.h"

#include <projectexplorer/runconfiguration.h>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit ClangStaticAnalyzerRunControlFactory(ClangStaticAnalyzerTool *tool,
                                                  QObject *parent = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                Core::Id runMode) const;

    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        Core::Id runMode,
                                        QString *errorMessage);

private:
    ClangStaticAnalyzerTool *m_tool;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERRUNCONTROLFACTORY_H
