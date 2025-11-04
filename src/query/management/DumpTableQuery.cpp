//
// Created by liu on 18-10-25.
//

#include "DumpTableQuery.h"

#include <exception>
#include <fstream>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"
#include "../../utils/formatter.h"

auto DumpTableQuery::execute() -> QueryResult::Ptr {
  const auto &database = Database::getInstance();
  try {
    std::ofstream outfile(this->fileName);
    if (!outfile.is_open()) {
      return std::make_unique<ErrorMsgResult>(qname, "Cannot open file '?'"_f %
                                                         this->fileName);
    }
    outfile << database[this->targetTable];
    outfile.close();
    return std::make_unique<NullQueryResult>();
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

auto DumpTableQuery::toString() -> std::string {
  return "QUERY = Dump TABLE, FILE = \"" + this->fileName + "\"";
}
