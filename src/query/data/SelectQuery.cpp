#include "SelectQuery.h"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../db/Database.h"
#include "../../db/Table.h"
#include "../../utils/formatter.h"
#include "../../utils/uexception.h"
#include "../QueryResult.h"

auto SelectQuery::execute() -> QueryResult::Ptr {
  std::size_t const operands_size = this->operands.size();

  // the operands must be ( KEY ...), so size >= 1
  if (operands_size < 1) {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        // cppcheck-suppress zerodivcond
        "Invalid number of operands (? operands)."_f % operands_size);
  }

  // KEY field must always appear at the beginning
  if (this->operands[0] != "KEY") {
    return std::make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid format, the first must be KEY.");
  }

  Database &database = Database::getInstance();
  std::stringstream msg;
  try {
    auto &table = database[this->targetTable];
    std::string tmp;
    auto result = initCondition(table);
    if (result.second) {
      // vector of line message, used to sort in ascending lexical order
      std::vector<std::string> v_msg;

      // save the target field index in a vector (append, do not clear)
      auto ids_view =
          this->operands | std::ranges::views::drop(1) |
          std::ranges::views::transform(
              [&table](const std::string &fname) -> Table::FieldIndex {
                return table.getFieldIndex(fname);
              });
      std::ranges::copy(ids_view, std::back_inserter(this->fieldId));

      // if condition satisfies, push line message to v_msg
      for (auto &&obj : table) {
        if (this->evalCondition(obj)) {
          std::stringstream line_msg;
          line_msg << "( " << obj.key();
          for (auto fieldVal : fieldId) {
            line_msg << " " << obj[fieldVal];
          }
          line_msg << " )\n";
          v_msg.push_back(line_msg.str());
        }
      }

      // sort in ascending lexical order
      std::ranges::sort(v_msg);

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

auto SelectQuery::toString() -> std::string {
  return "QUERY = SELECT " + this->targetTable + "\"";
}
