#include "MaxQuery.h"

#include "../../db/Database.h"

constexpr const char *MaxQuery::qname;

QueryResult::Ptr MaxQuery::execute() {
  using namespace std;

  // check operands
  if (this->operands.empty()) {
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->targetTable];

    fieldId.clear();
    fieldId.reserve(this->operands.size());
    for (const auto &fname : this->operands) {
      fieldId.emplace_back(table.getFieldIndex(fname));
    }

    auto condInit = initCondition(table);

    // Compute max per field
    vector<int> maxs(fieldId.size(), Table::ValueTypeMin);
    size_t matched = 0;

    if (condInit.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (evalCondition(*it)) {
          ++matched;
          for (size_t i = 0; i < fieldId.size(); ++i) {
            auto v = (*it)[fieldId[i]];
            if (v > maxs[i])
              maxs[i] = v;
          }
        }
      }
    }

    if (matched == 0) {
      return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                         "Empty result set");
    }

    return make_unique<SuccessMsgResult>(std::move(maxs));
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "No such table."s);
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string MaxQuery::toString() {
  return "QUERY = MAX " + this->targetTable + "\"";
}
