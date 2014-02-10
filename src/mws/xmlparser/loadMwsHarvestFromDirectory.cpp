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
  * @brief File containing the implementation of the loadMwsHarvestFromDirectory
  * function
  *
  * @file loadMwsHarvestFromDirectory.cpp
  * @date 30 Apr 2012
  *
  * License: GPL v3
  *
  */

// System includes

#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <algorithm>
#include <utility>
#include <map>
#include <iostream>

#include <json/json.h>

#include "mws/index/IndexManager.hpp"
#include "common/utils/Path.hpp"
#include "common/utils/macro_func.h"
#include "loadMwsHarvestFromFd.hpp"

#include "common/utils/util.hpp"
#include "crawler/parser/MathParser.hpp"

#include "config.h"

// Namespaces

using namespace std;

namespace mws {

static void writeElasticSearchHarvest(const string& xhtml,
                                      const map<FormulaId, vector<FormulaDocId> >& id_mappings,
                                      const AbsPath& elasticSearchOutputPath);
static void processXhtmlFile(const std::string& path,
                             const std::string& prefix,
                             index::IndexManager* indexManager,
                             const std::string& elasticSearchOutput,
                             int* totalLoaded);

int
loadMwsHarvestFromDirectory(mws::index::IndexManager* indexManager,
                            mws::AbsPath const& dirPath,
                            mws::AbsPath const& elasticSearchOutputPath,
                            bool recursive)
{
    int totalLoaded = 0;
    common::utils::FileCallback fileCallback =
            [&totalLoaded, indexManager, elasticSearchOutputPath]
            (const std::string& path, const string& partialDirectoryPath) {
        processXhtmlFile(path, partialDirectoryPath, indexManager,
                         elasticSearchOutputPath.get(), &totalLoaded);
        return 0;
    };
    common::utils::DirectoryCallback shouldRecurse =
            [elasticSearchOutputPath](const std::string partialPath) {
        string dirToCreate =
                (string) elasticSearchOutputPath.get() + "/" + partialPath;
        if (mkdir(dirToCreate.c_str(), 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "Error while creating \"%s\": %s\n",
                    dirToCreate.c_str(), strerror(errno));
            return false;
        }
        return true;
    };

    printf("Loading harvest files...\n");
    if (recursive) {
        FAIL_ON(common::utils::foreachEntryInDirectory(dirPath.get(),
                                                       fileCallback,
                                                       shouldRecurse));
    } else {
        FAIL_ON(common::utils::foreachEntryInDirectory(dirPath.get(),
                                                       fileCallback));
    }
    printf("Total %d\n", totalLoaded);

    return totalLoaded;

fail:
    printf("Total %d (errors encountered)\n", totalLoaded);
    return totalLoaded;
}

static void processXhtmlFile(const std::string& path,
                             const std::string& prefix,
                             index::IndexManager* indexManager,
                             const std::string& elasticSearchOutput,
                             int* totalLoaded) {
    if (common::utils::hasSuffix(path, ".xhtml")) {
        int fd;
        printf("Processing %s ...\n", path.c_str());
        fflush(stdout);
        // Load contents and generate harvest;
        char harvest_path_templ[] = "./tmp_XXXXX";
        mkstemp(harvest_path_templ);
        string doc = common::utils::getFileContents(path.c_str());
        vector<string> mathElements =
                crawler::parser::getHarvestFromXhtml(doc, path);

        FILE* harvest = fopen(harvest_path_templ, "w");
        fputs("<?xml version=\"1.0\" ?>\n"
              "<mws:harvest xmlns:m=\"http://www.w3.org/1998/Math/MathML\"\n"
              "             xmlns:mws=\"http://search.mathweb.org/ns\">\n",
              harvest);
        for (const string& mathElement : mathElements) {
            fputs(mathElement.c_str(), harvest);
        }
        fputs("</mws:harvest>\n", harvest);
        fclose(harvest);

        printf("Created harvest %s\n", harvest_path_templ);
        printf("Loading harvest %s ...\n", harvest_path_templ);
        fd = open(harvest_path_templ, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Error while opening \"%s\"\n", path.c_str());
            return;
        }
        map<FormulaId, vector<FormulaDocId> > loggedFormulae;
        indexManager->mloggedFormulae = &loggedFormulae;
        auto loadReturn = loadMwsHarvestFromFd(indexManager, fd);
        if (loadReturn.first == 0) {
            printf("%d loaded\n", loadReturn.second);
        } else {
            printf("%d loaded (with errors)\n", loadReturn.second);
        }

        *totalLoaded += loadReturn.second;

        close(fd);
        unlink(harvest_path_templ);

        // Output json harvest for elastic search
        if (elasticSearchOutput.size() > 0) {
            AbsPath elasticSearchFullPath = elasticSearchOutput;
            elasticSearchFullPath.append(prefix);
            char* pathCopy = strdup(path.c_str());
            char* filename = basename(pathCopy);
            elasticSearchFullPath.append((string)filename + ".json");
            free(pathCopy);
            writeElasticSearchHarvest(doc, loggedFormulae,
                                      elasticSearchFullPath);
        }
    }
    else {
        printf("Skipping bad extension file \"%s\"\n", path.c_str());
    }
}

static void writeElasticSearchHarvest(const string& xhtml,
                                      const map<FormulaId, vector<FormulaDocId> >& id_mappings,
                                      const AbsPath& elasticSearchOutputPath) {
    json_object *json_doc, *json_ids, *json_id_mappings, *json_xhtml;

    json_doc = json_object_new_object();
    json_ids = json_object_new_array();
    json_id_mappings = json_object_new_array();
    json_xhtml = json_object_new_string(xhtml.c_str());

    for (auto id_mapping : id_mappings) {
        uint64_t id = id_mapping.first;
        json_object_array_add(json_ids, json_object_new_int64(id));
        for (auto formulaDocId : id_mapping.second) {
            json_object* mapping = json_object_new_object();
            json_object_object_add(mapping, "id",
                                   json_object_new_int64(id));
            json_object_object_add(mapping, "xpath",
                                   json_object_new_string(formulaDocId.xpath.c_str()));
            json_object_object_add(mapping, "url",
                                   json_object_new_string(formulaDocId.xmlId.c_str()));
            json_object_array_add(json_id_mappings, mapping);
        }
    }

    json_object_object_add(json_doc, "ids", json_ids);
    json_object_object_add(json_doc, "id_mappings", json_id_mappings);
    json_object_object_add(json_doc, "xhtml", json_xhtml);

    string json_string =
            json_object_to_json_string_ext(json_doc, JSON_C_TO_STRING_PRETTY);
    common::utils::putFileContents(elasticSearchOutputPath.get(), json_string);
    printf("Wrote ES harvest to %s\n", elasticSearchOutputPath.get());

    // free
    json_object_put(json_doc);
}

}
