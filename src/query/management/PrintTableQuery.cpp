//
// Created by liu on 18-10-25.
//

#include "PrintTableQuery.h"

#include <iostream>
#include <memory>
#include <string>

#include "../../db/Database.h"

constexpr const char *PrintTableQuery::qname;

QueryResult::Ptr PrintTableQuery::execute() {
  const auto &db = Database::getInstance();
  try {
    const auto &table = db[this->targetTable];
    std::cout << "================\n";
    std::cout << "TABLE = ";
    std::cout << table;
    std::cout << "================\n" << std::endl;
    return std::make_unique<SuccessMsgResult>(qname, this->targetTable);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  }
}

std::string PrintTableQuery::toString() {
  return "QUERY = SHOWTABLE, Table = \"" + this->targetTable + "\"";
}
