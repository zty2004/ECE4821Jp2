#include "SubQuery.h"

QueryResult::Ptr SubQuery::execute() {
  return std::make_unique<NullQueryResult>();
}

std::string SubQuery::toString() { return "QUERY = SUB"; }
