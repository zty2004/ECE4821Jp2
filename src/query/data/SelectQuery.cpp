#include "SelectQuery.h"

#include "../../db/Database.h"

constexpr const char *SelectQuery::qname;

QueryResult::Ptr SelectQuery::execute() {
  using namespace std;
  size_t operands_size = this->operands.size();

  // the operands must be ( KEY field ... ), so size >= 2
  if (operands_size < 2)
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid number of operands (? operands)."_f % operands_size);

  // KEY field must always appear at the beginning
  if (this->operands[0] != "KEY")
    return make_unique<ErrorMsgResult>(
        qname, this->targetTable.c_str(),
        "Invalid format, the first must be KEY.");

  Database &db = Database::getInstance();
  stringstream msg;
  try {
    auto &table = db[this->targetTable];
    auto result = initCondition(table);
    if (result.second) {
      // vector of line message, used to sort in ascending lexical order
      vector<string> v_msg;

      // save the target field index in a vector
      for (size_t i = 1; i < operands_size; ++i)
        this->fieldId.push_back(table.getFieldIndex(this->operands[i]));

      // if condition satisfies, push line message to v_msg
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          stringstream line_msg;
          line_msg << "( " << it->key();
          for (auto fieldVal : fieldId)
            line_msg << " " << (*it)[fieldVal];
          line_msg << " )\n";
          v_msg.push_back(line_msg.str());
        }
      }

      // sort in ascending lexical order
      sort(v_msg.begin(), v_msg.end());

      // concat as a whole message
      for (auto x : v_msg)
        msg << x;
    }
    return make_unique<SuccessMsgResult>(qname, targetTable, msg.str());
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const TableFieldNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const invalid_argument &e) {
    // Cannot convert operand to string
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unkonwn error '?'."_f % e.what());
  }
}

std::string SelectQuery::toString() {
  return "QUERY = SELECT " + this->targetTable + "\"";
}
