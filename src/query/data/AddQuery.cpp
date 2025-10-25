#include "AddQuery.h"

QueryResult::Ptr AddQuery::execute() {
  return std::make_unique<NullQueryResult>();
}

std::string AddQuery::toString() { return "QUERY = ADD"; }
