#include "AddQuery.h"
#include "../../db/Database.h"

constexpr const char *AddQuery::qname;

QueryResult::Ptr AddQuery::execute() {
  using namespace std;

  // Expect at least dst + one src
  if (this->operands.size() < 2) {
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands.size());
  }

  try {
    auto &db = Database::getInstance();
    auto &table = db[this->targetTable];

    dstId = table.getFieldIndex(this->operands.back());
    srcId.clear();
    srcId.reserve(this->operands.size() - 1);
    for (size_t i = 0; i + 1 < this->operands.size(); ++i) {
      srcId.emplace_back(table.getFieldIndex(this->operands[i]));
    }

    auto condInit = initCondition(table);

    // Iterate and update
    size_t counter = 0;
    if (condInit.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (evalCondition(*it)) {
          int sum = 0;
          for (auto idx : srcId) {
            sum += (*it)[idx];
          }
          (*it)[dstId] = sum;
          ++counter;
        }
      }
    }

    return make_unique<RecordCountResult>(static_cast<int>(counter));
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const std::exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string AddQuery::toString() {
  return "QUERY = ADD " + this->targetTable + "\"";
}
