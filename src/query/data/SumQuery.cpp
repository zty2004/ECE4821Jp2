#include "SumQuery.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../db/Database.h"

constexpr const char *SumQuery::qname;

QueryResult::Ptr SumQuery::execute() {
  Database &db = Database::getInstance();
  try {
    auto &table = db[this->targetTable];

    // check whether operands are provided
    if (this->operands.empty()) {
      return std::make_unique<ErrorMsgResult>(qname, this->targetTable,
                                              "No operands (? operands)."_f %
                                                  operands.size());
    }

    // get field indices for all operands
    for (const auto &field : this->operands) {
      if (field == "KEY") {
        // KEY field cannot be summed over
        return std::make_unique<ErrorMsgResult>(
            qname, this->targetTable, "The KEY field cannot be summed over.");
      } else {
        fieldId.push_back(table.getFieldIndex(field));
      }
    }

    // if no record is affected, then the sum is set to zero
    // use int64_t to avoid overflow during accumulation
    std::vector<int64_t> sums(fieldId.size(), 0);

    auto result = initCondition(table);
    if (result.second) {
      // iterate through all datum and sum the one satisfying condition
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          for (size_t i = 0; i < fieldId.size(); ++i) {
            sums[i] += (*it)[fieldId[i]];
          }
        }
      }
    }
    // Convert back to int for the result
    std::vector<int> result_sums(sums.size());
    for (size_t i = 0; i < sums.size(); ++i) {
      result_sums[i] = static_cast<int>(sums[i]);
    }
    return std::make_unique<SuccessMsgResult>(result_sums);
  } catch (const TableNameNotFound &e) {
    return std::make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
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

std::string SumQuery::toString() {
  return "QUERY = SUM FROM " + this->targetTable + "\"";
}
