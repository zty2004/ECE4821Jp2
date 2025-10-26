#include "DuplicateQuery.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  if (!this->operands.empty())
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  Database &db = Database::getInstance();
  try {
    Table::SizeType counter = 0;
    auto &table = db[this->targetTable];
    auto fieldSize = table.field().size();
    auto result = initCondition(table);
    if (result.second) {
      auto currEnd = table.end();
      for (auto it = table.begin(); it != currEnd; ++it) {
        if (this->evalCondition(*it)) {
          Table::KeyType const key = it->key();
          std::string const copyKey = key + "_copy";
          // if cannot find existed key, avoid copy again!
          if (!table[copyKey]) {
            std::vector<Table::ValueType> copyData;
            copyData.reserve(fieldSize);
            for (std::size_t i = 0; i < table.field().size(); ++i) {
              copyData.push_back((*it)[i]);
            }
            table.insertByIndex(copyKey, std::move(copyData));
            ++counter;
          }
        }
      }
    }
    return std::make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            std::string("No such table."));
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unkonwn error '?'"_f % e.what());
  }
}

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE FROM " + this->targetTable;
}
