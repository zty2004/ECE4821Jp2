#include "DeleteQuery.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../db/Database.h"

constexpr const char *DeleteQuery::qname;

QueryResult::Ptr DeleteQuery::execute() {
  Database &db = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = db[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // collect keys to delete because can't delete while iterating
      std::vector<Table::KeyType> del_keys;

      // iterate through all datum and check conditions
      for (auto it = table.begin(); it != table.end(); ++it)
        if (this->evalCondition(*it))
          del_keys.push_back(it->key());

      for (const auto &key : del_keys)
        if (table.deleteByIndex(key))
          ++counter;
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

std::string DeleteQuery::toString() {
  return "QUERY = DELETE " + this->targetTable + "\"";
}
