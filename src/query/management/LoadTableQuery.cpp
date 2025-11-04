//
// Created by liu on 18-10-25.
//

#include "LoadTableQuery.h"

#include <fstream>
#include <memory>
#include <string>
#include <exception>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"
#include "../../utils/formatter.h"

auto LoadTableQuery::execute() -> QueryResult::Ptr {
  // Database &database = Database::getInstance();
  // The reason of removing this line is that loadTableFromStream is a static method.
  try {
    std::ifstream infile(this->fileName);
    if (!infile.is_open()) {
      return std::make_unique<ErrorMsgResult>(qname, "Cannot open file '?'"_f %
                                                         this->fileName);
    }
    Database::loadTableFromStream(infile, this->fileName);
    infile.close();
    return std::make_unique<NullQueryResult>();
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

auto LoadTableQuery::toString() -> std::string {
  return "QUERY = Load TABLE, FILE = \"" + this->fileName + "\"";
}
