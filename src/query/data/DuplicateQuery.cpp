#include "DuplicateQuery.h"

#include "../../db/Database.h"

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {
  using namespace std;
  Database &db = Database::getInstance();
  Table::SizeType counter = 0;
  try {
    auto &table = db[this->targetTable];
    auto result = initCondition(table);
    size_t field_cnt = table.field().size();
    if (result.second) {
      // collect datums to duplicate because can't insert while iterating
      vector<pair<Table::KeyType, vector<Table::ValueType>>> duplicate_datums;

      // iterate through all datums and collect those satisfying conditions
      for (auto it = table.begin(); it != table.end(); ++it) {
        if (this->evalCondition(*it)) {
          Table::KeyType key = it->key();
          vector<Table::ValueType> fields;
          fields.reserve(field_cnt);

          // copy all field values
          for (size_t i = 0; i < field_cnt; ++i)
            fields.push_back((*it)[i]);

          duplicate_datums.emplace_back(key, move(fields));
        }
      }

      // insert duplicates with modified keys
      for (const auto &datum : duplicate_datums) {
        string new_key = datum.first + "_copy";

        // check if key already exists, if yes do nothing
        if (table[new_key] != nullptr)
          continue;

        // insert the duplicate
        vector<Table::ValueType> field_copy = datum.second;
        table.insertByIndex(new_key, move(field_copy));
        ++counter;
      }
    }
    return make_unique<RecordCountResult>(counter);
  } catch (const TableNameNotFound &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const IllFormedQueryCondition &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable, e.what());
  } catch (const invalid_argument &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'"_f % e.what());
  } catch (const exception &e) {
    return make_unique<ErrorMsgResult>(qname, this->targetTable,
                                       "Unknown error '?'."_f % e.what());
  }
}

std::string DuplicateQuery::toString() {
  return "QUERY = DUPLICATE FROM " + this->targetTable;
}
