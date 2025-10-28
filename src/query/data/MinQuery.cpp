#include "MinQuery.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../db/Database.h"

constexpr const char *MinQuery::qname;

QueryResult::Ptr MinQuery::execute() {
  // check operands
  if (this->operands.empty()) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->targetTable];

    fieldId.clear();
    fieldId.resize(this->operands.size());
    std::transform(this->operands.begin(), this->operands.end(),
                   fieldId.begin(), [&table](const std::string &fname) {
                     return table.getFieldIndex(fname);
                   });

    auto condInit = initCondition(table);

    // Compute min per field
    std::vector<int> mins(fieldId.size(), Table::ValueTypeMax);
    std::size_t matched = 0;

    if (condInit.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (evalCondition(*it)) {
          ++matched;
          for (size_t i = 0; i < fieldId.size(); ++i) {
            auto v = (*it)[fieldId[i]];
            if (v < mins[i]) {
              mins[i] = v;
            }
          }
        }
      }
    }

    if (matched == 0) {
      return std::make_unique<NullQueryResult>();
    }

    return std::make_unique<SuccessMsgResult>(std::move(mins));
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                            "Unkonwn error '?'"_f % e.what());
  }
}

std::string MinQuery::toString() {
  return "QUERY = MIN " + this->targetTable + "\"";
}
