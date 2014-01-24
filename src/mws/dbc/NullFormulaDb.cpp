/*

Copyright (C) 2010-2013 KWARC Group <kwarc.info>

This file is part of MathWebSearch.

MathWebSearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MathWebSearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MathWebSearch.  If not, see <http://www.gnu.org/licenses/>.

*/
/**
  * @file NullFormulaDb.cpp
  * @brief Formula Memory Database implementation
  * @date 12 Nov 2013
  */

#include "common/utils/macro_func.h"

#include "NullFormulaDb.hpp"

using namespace std;

namespace mws { namespace dbc {

int
NullFormulaDb::insertFormula(const mws::FormulaId&   formulaId,
                            const mws::CrawlId&     crawlId,
                            const mws::FormulaPath& formulaPath) {
    UNUSED(formulaId);
    UNUSED(crawlId);
    UNUSED(formulaPath);
    return 0;
}

int
NullFormulaDb::queryFormula(const FormulaId &formulaId,
                           unsigned limitMin,
                           unsigned limitSize,
                           QueryCallback queryCallback) {
    UNUSED(formulaId);
    UNUSED(limitMin);
    UNUSED(limitSize);
    UNUSED(queryCallback);
    return 0;
}

} }
