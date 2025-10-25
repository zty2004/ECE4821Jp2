//
// Created by zuo on 25-10-25.
//

#include "DuplicateQuery.h"

#include <algorithm>

#include "../../db/Database.h"
#include "../QueryResult.h"

constexpr const char *DuplicateQuery::qname;

QueryResult::Ptr DuplicateQuery::execute() {}

std::string DuplciateQuery::toString() {
  return "QUERY = Duplicate " + this->targetTable + "\"";
}
