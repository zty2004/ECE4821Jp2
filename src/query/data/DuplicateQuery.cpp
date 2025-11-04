#include "DuplicateQuery.h"

#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto DuplicateQuery::execute() -> QueryResult::Ptr {
  if (!this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }
  Database &database = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = database[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // Collect pairs {origKey, copyKey} instead of real data
      std::vector<std::pair<Table::KeyType, Table::KeyType>> to_duplicate;

      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          const Table::KeyType &origKey = obj.key();
          const std::string copyKey = origKey + "_copy";
          if (!table[copyKey]) {
            to_duplicate.emplace_back(origKey, copyKey);
          }
        }
      }
      for (const auto &key_pair : to_duplicate) {
        if (table.duplicateByKey(key_pair.first, key_pair.second)) {
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
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unkonwn error '?'"_f % e.what());
  }
}

auto DuplicateQuery::toString() -> std::string {
  return "QUERY = DUPLICATE FROM " + this->targetTable;
}
