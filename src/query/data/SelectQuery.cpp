#include "SelectQuery.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "../../db/Database.h"

constexpr const char *SelectQuery::qname;

QueryResult::Ptr SelectQuery::execute() {
  std::size_t const operands_size = this->operands.size();

  // the operands must be ( KEY ...), so size >= 1
  if (operands_size < 1) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        // cppcheck-suppress zerodivcond
        "Invalid number of operands (? operands)."_f % operands_size);
  }

  // KEY field must always appear at the beginning
  if (this->operands[0] != "KEY")
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid format, the first must be KEY.");

  Database &db = Database::getInstance();
  std::stringstream msg;
  try {
    auto &table = db[this->targetTable];
    std::string tmp;
    auto result = initCondition(table);
    if (result.second) {
      // vector of line message, used to sort in ascending lexical order
      std::vector<std::string> v_msg;

      // save the target field index in a vector
      for (std::size_t i = 1; i < operands_size; ++i) {
        this->fieldId.push_back(table.getFieldIndex(this->operands[i]));
      }

      // if condition satisfies, push line message to v_msg
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          std::stringstream line_msg;
          line_msg << "( " << it->key();
          for (auto fieldVal : fieldId) {
            line_msg << " " << (*it)[fieldVal];
          }
          line_msg << " )\n";
          v_msg.push_back(line_msg.str());
        }
      }

      // sort in ascending lexical order
      std::sort(v_msg.begin(), v_msg.end());

      // concat as a whole message
      for (const auto &line : v_msg) {
        msg << line;
      }

      tmp = msg.str();
      if (!tmp.empty()) {
        tmp.pop_back();
      } else {
        return std::make_unique<NullQueryResult>();
      }
    }
    return std::make_unique<SuccessMsgResult>(tmp);
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
                                            "Unkonwn error '?'"_f % e.what());
  }
}

std::string SelectQuery::toString() {
  return "QUERY = SELECT " + this->targetTable + "\"";
}
