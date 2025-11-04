#include "DeleteQuery.h"

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto DeleteQuery::execute() -> QueryResult::Ptr {
  Database &database = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = database[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // collect keys to delete because can't delete while iterating
      std::vector<Table::KeyType> del_keys;

      // iterate through all datum and check conditions
      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          del_keys.push_back(obj.key());
        }
      }

      for (const auto &key : del_keys) {
        if (table.deleteByIndex(key)) {
          ++counter;
        }
      }
    }
    return std::make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::invalid_argument &e) {
    // Cannot convert operand to string
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unknown error '?'"_f % e.what());
  }
}

auto DeleteQuery::toString() -> std::string {
  return "QUERY = DELETE " + this->targetTable + "\"";
}
