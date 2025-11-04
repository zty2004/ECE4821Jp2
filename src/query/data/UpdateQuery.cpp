//
// Created by liu on 18-10-25.
//

#include "UpdateQuery.h"

#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto UpdateQuery::execute() -> QueryResult::Ptr {
  if (this->operands.size() != 2) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }
  Database &database = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = database[this->targetTable];
    if (this->operands[0] == "KEY") {
      this->keyValue = this->operands[1];
    } else {
      this->fieldId = table.getFieldIndex(this->operands[0]);
      constexpr int dec = 10;
      this->fieldValue = static_cast<Table::ValueType>(
          strtol(this->operands[1].c_str(), nullptr, dec));
    }
    auto result = initCondition(table);
    if (result.second) {
      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          if (this->keyValue.empty()) {
            obj[this->fieldId] = this->fieldValue;
          } else {
            obj.setKey(this->keyValue);
          }
          ++counter;
        }
      }
    }
    return std::make_unique<RecordCountResult>(counter);
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

auto UpdateQuery::toString() -> std::string {
  return "QUERY = UPDATE " + this->targetTable + "\"";
}
