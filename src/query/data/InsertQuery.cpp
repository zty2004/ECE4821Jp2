//
// Created by liu on 18-10-25.
//

#include "InsertQuery.h"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto InsertQuery::execute() -> QueryResult::Ptr {
  if (this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable.c_str(),
                                            "No operand (? operands)."_f %
                                                operands.size());
  }
  Database &database = Database::getInstance();
  try {
    auto &table = database[this->targetTable];
    auto &key = this->operands.front();
    std::vector<Table::ValueType> data;
    data.resize(this->operands.size() - 1);
    auto tail = this->operands | std::ranges::views::drop(1);
    constexpr int dec = 10;
    std::ranges::transform(tail, data.begin(),
                           [](const std::string &str) -> Table::ValueType {
                             return static_cast<Table::ValueType>(
                                 std::strtol(str.c_str(), nullptr, dec));
                           });
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

auto InsertQuery::toString() -> std::string {
  return "QUERY = INSERT " + this->targetTable + "\"";
}
