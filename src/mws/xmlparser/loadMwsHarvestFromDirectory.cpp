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

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <algorithm>
#include <utility>
#include <map>
#include <iostream>

#include "json/json.h"

#include "mws/index/IndexManager.hpp"
#include "common/utils/Path.hpp"
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

int
loadMwsHarvestFromDirectory(mws::index::IndexManager* indexManager,
                            mws::AbsPath const& dirPath,
                            mws::AbsPath const& elasticSearchOutputPath,
                            bool recursive)
{
    DIR*           directory;
    struct dirent* currEntry;
    struct dirent  tmpEntry;
    int            ret;
    size_t         extenSize;
    int            fd;
    int            totalLoaded;
    pair<int,int>  loadReturn;
    vector<string> files;
    vector<string> subdirs;
    AbsPath        fullPath;
    vector<string> :: iterator it;

    totalLoaded = 0;

    extenSize = strlen(MWS_HARVEST_SUFFIX);

    directory = opendir(dirPath.get());
    if (!directory)
    {
        perror("loadMwsHarvestFromDirectory");
        return -1;
    }

    while ((ret = readdir_r(directory, &tmpEntry, &currEntry)) == 0 &&
            currEntry != NULL)
    {
        size_t entrySize = strlen(currEntry->d_name);

        if (currEntry->d_name[0] == '.') {
            printf("Skipping hidden entry \"%s\"\n", currEntry->d_name);
        } else {
            switch (currEntry->d_type) {
            case DT_DIR:
                if (recursive) {
                    subdirs.push_back(currEntry->d_name);
                } else {
                    printf("Skipping directory \"%s\"\n", currEntry->d_name);
                }
                break;
            case DT_REG:
                if (strcmp(currEntry->d_name + entrySize - extenSize,
                             MWS_HARVEST_SUFFIX) == 0) {
                    files.push_back(currEntry->d_name);
                } else {
                    printf("Skipping bad extension file \"%s\"\n",
                           currEntry->d_name);
                }
                break;
            default:
                printf("Skiping entry \"%s\": not a regular file\n",
                       currEntry->d_name);
                break;
            }
        }
    }
    if (ret != 0)
    {
        perror("readdir:");
    }

    // Closing the directory
    closedir(directory);

    // Sorting the entries
    sort(files.begin(), files.end());

    printf("Loading harvest files...\n");

    // Loading the harvests from the respective entries
    for (it = files.begin(); it != files.end(); it++)
    {
        fullPath.set(dirPath.get());
        fullPath.append(*it);

        printf("Processing %s ...\n", fullPath.get());
        fflush(stdout);
        // Load contents and generate harvest;
        char harvest_path_templ[] = "./tmp_XXXXX";
        mkstemp(harvest_path_templ);
        string doc = common::utils::getFileContents(fullPath.get());
        vector<string> mathElements = crawler::parser::getHarvestFromXhtml(doc, fullPath.get());

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
            fprintf(stderr, "Error while opening \"%s\"\n", fullPath.get());
            continue;
        }
        map<FormulaId, vector<FormulaDocId> > loggedFormulae;
        indexManager->mloggedFormulae = &loggedFormulae;
        loadReturn = loadMwsHarvestFromFd(indexManager, fd);
        if (loadReturn.first == 0)
        {
            printf("%d loaded\n", loadReturn.second);
        }
        else
        {
            printf("%d loaded (with errors)\n", loadReturn.second);
        }

        totalLoaded += loadReturn.second;

        close(fd);
        unlink(harvest_path_templ);

        // Output json harvest for elastic search
        AbsPath elasticSearchFullPath = elasticSearchOutputPath;
        elasticSearchFullPath.append(*it);
        writeElasticSearchHarvest(doc, loggedFormulae, elasticSearchFullPath);

        printf("Total %d\n", totalLoaded);
    }

    // Recursing through directories
    if (recursive) {
        for (it = subdirs.begin(); it != subdirs.end(); it++) {
            fullPath.set(dirPath.get());
            fullPath.append(*it);
            totalLoaded += loadMwsHarvestFromDirectory(indexManager, fullPath,
                                                       elasticSearchOutputPath,
                                                       /* recursive = */ true);
        }
    }

    return totalLoaded;

//failure:
//    return -1;
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
