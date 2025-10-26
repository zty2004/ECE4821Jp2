//
// Created by liu on 18-10-25.
//

#include "UpdateQuery.h"

#include <memory>
#include <string>

#include "../../db/Database.h"

constexpr const char *UpdateQuery::qname;

QueryResult::Ptr UpdateQuery::execute() {
  if (this->operands.size() != 2)
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  Database &db = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = db[this->targetTable];
    if (this->operands[0] == "KEY") {
      this->keyValue = this->operands[1];
    } else {
      this->fieldId = table.getFieldIndex(this->operands[0]);
      this->fieldValue =
          (Table::ValueType)strtol(this->operands[1].c_str(), nullptr, 10);
    }
    auto result = initCondition(table);
    if (result.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          if (this->keyValue.empty()) {
            (*it)[this->fieldId] = this->fieldValue;
          } else {
            it->setKey(this->keyValue);
          }
          ++counter;
        }
      }
    }
    return std::make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
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

std::string UpdateQuery::toString() {
  return "QUERY = UPDATE " + this->targetTable + "\"";
}
