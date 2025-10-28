#include "SubQuery.h"

#include <memory>
#include <numeric>
#include <string>

#include "../../db/Database.h"

constexpr const char *SubQuery::qname;

QueryResult::Ptr SubQuery::execute() {

  // Expect at least dst + one src
  if (this->operands.size() < 2) {
    return std::make_unique<ErrorMsgResult>(
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

    // Iterate and update: dst = src[0] - sum(src[1..])
    std::size_t counter = 0;
    if (condInit.second) {
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (evalCondition(*it)) {
          int value = (*it)[srcId[0]];
          // subtract the sum of remaining sources using std::accumulate
          int sub_sum = std::accumulate(
              srcId.begin() + 1, srcId.end(), 0,
              [&](int acc, size_t idx) { return acc + (*it)[idx]; });
          value -= sub_sum;
          (*it)[dstId] = value;
          ++counter;
        }
      }
    }

    return std::make_unique<RecordCountResult>(static_cast<int>(counter));
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

std::string SubQuery::toString() {
  return "QUERY = SUB " + this->targetTable + "\"";
}
