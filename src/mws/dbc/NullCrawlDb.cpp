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
  * @file CrawlDb.cpp
  * @brief Crawl Data Memory Database implementation
  * @date 12 Nov 2013
  */

#include <map>
#include <stdexcept>
#include "common/utils/ToString.hpp"
#include "common/utils/macro_func.h"

#include "NullCrawlDb.hpp"

using namespace std;

namespace mws { namespace dbc {

NullCrawlDb::NullCrawlDb() {
}

mws::CrawlId
NullCrawlDb::putData(const mws::types::CrawlData& crawlData)
throw (std::exception) {
    UNUSED(crawlData);
    return (mws::CrawlId) 0;
}

const types::CrawlData NullCrawlDb::getData(const mws::CrawlId& crawlId)
throw (std::exception) {
    UNUSED(crawlId);
    throw runtime_error("NullCrawlDb does not support getData()");
}


} }
