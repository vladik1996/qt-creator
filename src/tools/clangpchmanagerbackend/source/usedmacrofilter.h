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

#pragma once

#include "usedmacro.h"
#include "sourceentry.h"

#include <compilermacro.h>

namespace ClangBackEnd {

template<typename InputIterator1, typename InputIterator2, typename OutputIterator, typename Compare>
inline OutputIterator set_greedy_intersection(InputIterator1 first1,
                                              InputIterator1 last1,
                                              InputIterator2 first2,
                                              InputIterator2 last2,
                                              OutputIterator result,
                                              Compare comp)
{
    while (first1 != last1 && first2 != last2)
        if (comp(*first1, *first2))
            ++first1;
        else if (comp(*first2, *first1))
            ++first2;
        else {
            *result = *first1;
            ++first1;
            ++result;
        }
    return result;
}

class UsedMacroFilter
{
public:
    struct Includes
    {
        FilePathIds project;
        FilePathIds system;
    };

    UsedMacroFilter(const SourceEntries &includes,
                    const UsedMacros &usedMacros,
                    const CompilerMacros &compilerMacros)
    {
        filterIncludes(includes);
        systemUsedMacros = filterUsedMarcos(usedMacros, systemIncludes);
        projectUsedMacros = filterUsedMarcos(usedMacros, projectIncludes);
        filter(compilerMacros);
    }

    void filterIncludes(const SourceEntries &includes)
    {
        systemIncludes.reserve(includes.size());
        projectIncludes.reserve(includes.size());
        topSystemIncludes.reserve(includes.size() / 10);
        topProjectIncludes.reserve(includes.size() / 10);

        for (SourceEntry include : includes)
            filterInclude(include);
    }

    void filter(const CompilerMacros &compilerMacros)
    {
        CompilerMacros indexedCompilerMacro  = compilerMacros;

        std::sort(indexedCompilerMacro.begin(),
                  indexedCompilerMacro.end(),
                  [](const CompilerMacro &first, const CompilerMacro &second) {
                      return std::tie(first.key, first.value) < std::tie(second.key, second.value);
                  });

        systemCompilerMacros = filtercompilerMacros(indexedCompilerMacro, systemUsedMacros);
        projectCompilerMacros = filtercompilerMacros(indexedCompilerMacro, projectUsedMacros);
    }

private:
    void filterInclude(SourceEntry include)
    {
        switch (include.sourceType) {
        case SourceType::TopSystemInclude:
            topSystemIncludes.emplace_back(include.sourceId);
            systemIncludes.emplace_back(include.sourceId);
            break;
        case SourceType::SystemInclude:
            systemIncludes.emplace_back(include.sourceId);
            break;
        case SourceType::TopProjectInclude:
            topProjectIncludes.emplace_back(include.sourceId);
            projectIncludes.emplace_back(include.sourceId);
            break;
        case SourceType::ProjectInclude:
            projectIncludes.emplace_back(include.sourceId);
            break;
        case SourceType::UserInclude:
            break;
        }

        allIncludes.emplace_back(include.sourceId);
    }

    static Utils::SmallStringVector filterUsedMarcos(const UsedMacros &usedMacros,
                                                     const FilePathIds &filePathId)
    {
        struct Compare
        {
            bool operator()(const UsedMacro &usedMacro, FilePathId filePathId)
            {
                return usedMacro.filePathId < filePathId;
            }

            bool operator()(FilePathId filePathId, const UsedMacro &usedMacro)
            {
                return filePathId < usedMacro.filePathId;
            }
        };

        Utils::SmallStringVector filtertedMacros;
        filtertedMacros.reserve(usedMacros.size());

        set_greedy_intersection(usedMacros.begin(),
                                usedMacros.end(),
                                filePathId.begin(),
                                filePathId.end(),
                                std::back_inserter(filtertedMacros),
                                Compare{});

        std::sort(filtertedMacros.begin(), filtertedMacros.end());

        return filtertedMacros;
    }

    static CompilerMacros filtercompilerMacros(const CompilerMacros &indexedCompilerMacro,
                                               const Utils::SmallStringVector &usedMacros)
    {
        CompilerMacros filtertedCompilerMacros;
        filtertedCompilerMacros.reserve(indexedCompilerMacro.size() + usedMacros.size());

        struct Compare
        {
            bool operator()(const CompilerMacro &compilerMacro, Utils::SmallStringView usedMacro)
            {
                return compilerMacro.key < usedMacro;
            }

            bool operator()(Utils::SmallStringView usedMacro, const CompilerMacro &compilerMacro)
            {
                return usedMacro < compilerMacro.key;
            }
        };

        set_greedy_intersection(indexedCompilerMacro.begin(),
                                indexedCompilerMacro.end(),
                                usedMacros.begin(),
                                usedMacros.end(),
                                std::back_inserter(filtertedCompilerMacros),
                                Compare{});

        auto split = filtertedCompilerMacros.end();

        std::set_difference(usedMacros.begin(),
                            usedMacros.end(),
                            filtertedCompilerMacros.begin(),
                            filtertedCompilerMacros.end(),
                            std::back_inserter(filtertedCompilerMacros),
                            Compare{});

        std::inplace_merge(filtertedCompilerMacros.begin(), split, filtertedCompilerMacros.end());

        return filtertedCompilerMacros;
    }

public:
    FilePathIds allIncludes;
    FilePathIds projectIncludes;
    FilePathIds systemIncludes;
    FilePathIds topProjectIncludes;
    FilePathIds topSystemIncludes;
    Utils::SmallStringVector projectUsedMacros;
    Utils::SmallStringVector systemUsedMacros;
    CompilerMacros projectCompilerMacros;
    CompilerMacros systemCompilerMacros;
};

} // namespace ClangBackEnd
