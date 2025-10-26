//
// Created by liu on 18-10-25.
//

#include "LoadTableQuery.h"

#include <fstream>
#include <memory>
#include <string>

#include "../../db/Database.h"

constexpr const char *LoadTableQuery::qname;

QueryResult::Ptr LoadTableQuery::execute() {
  Database &db = Database::getInstance();
  try {
    std::ifstream infile(this->fileName);
    if (!infile.is_open()) {
      return std::make_unique<ErrorMsgResult>(qname, "Cannot open file '?'"_f %
                                                         this->fileName);
    }
    db.loadTableFromStream(infile, this->fileName);
    infile.close();
    return std::make_unique<SuccessMsgResult>(qname, targetTable);
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, e.what());
  }
}

std::string LoadTableQuery::toString() {
  return "QUERY = Load TABLE, FILE = \"" + this->fileName + "\"";
}
