//
// Created by liu on 18-10-25.
//

#include "PrintTableQuery.h"

#include <iostream>
#include <memory>
#include <string>

#include "../../db/Database.h"
#include "../../query/QueryResult.h"
#include "../../utils/uexception.h"

auto PrintTableQuery::execute() -> QueryResult::Ptr {
  const auto &database = Database::getInstance();
  try {
    const auto &table = database[this->targetTable];
    std::cout << "================\n";
    std::cout << "TABLE = ";
    std::cout << table;
    std::cout << "================\n\n"; // previously, it is std::cout << "================\n" << std::endl; 
    // I am not sure flush is needed or not here. Just keep "\n". Let's see.
    return std::make_unique<SuccessMsgResult>(qname, this->targetTable);
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  }
}

auto PrintTableQuery::toString() -> std::string {
  return "QUERY = SHOWTABLE, Table = \"" + this->targetTable + "\"";
}
