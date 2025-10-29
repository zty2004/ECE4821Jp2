//
// Created by liu on 18-10-25.
//

#include "InsertQuery.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../QueryResult.h"

constexpr const char *InsertQuery::qname;

QueryResult::Ptr InsertQuery::execute() {
  if (this->operands.empty())
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable.c_str(),
                                            "No operand (? operands)."_f %
                                                operands.size());
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];
    auto &key = this->operands.front();
    std::vector<Table::ValueType> data;
    data.reserve(this->operands.size() - 1);
    for (auto it = ++this->operands.begin(); it != this->operands.end(); ++it) {
      data.emplace_back(std::strtol(it->c_str(), nullptr, 10));
    }
    table.insertByIndex(key, std::move(data));
    return std::make_unique<NullQueryResult>();
  } catch (const TableNameNotFound &) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::invalid_argument &e) {
    // Cannot convert operand to string
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unkonwn error '?'"_f % e.what());
  }
}

std::string InsertQuery::toString() {
  return "QUERY = INSERT " + this->targetTable + "\"";
}
