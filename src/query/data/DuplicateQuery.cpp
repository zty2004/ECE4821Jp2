#include "DuplicateQuery.h"

#include "../../db/Database.h"

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  using namespace std;
  if (!this->operands.empty())
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  try {
    auto &table = db[this->targetTable];
    auto fieldSize = table.field().size();
    auto result = initCondition(table);
    if (result.second) {
      auto currEnd = table.end();
      for (auto it = table.begin(); it != currEnd; ++it) {
        if (this->evalCondition(*it)) {
          Table::KeyType const key = it->key();
          string const copyKey = key + "_copy";
          // if cannot find existed key, avoid copy again!
          if (!table[copyKey]) {
            vector<Table::ValueType> copyData;
            copyData.reserve(fieldSize);
            for (auto i = 0; i < table.field().size(); ++i) {
              copyData.push_back((*it)[i]);
            }
            table.insertByIndex(copyKey, std::move(copyData));
            ++counter;
          }
        }
      }
    }
    return make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table."s);
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE FROM " + this->targetTable;
}
