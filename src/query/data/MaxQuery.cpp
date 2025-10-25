#include "MaxQuery.h"

QueryResult::Ptr MaxQuery::execute() {
  return std::make_unique<NullQueryResult>();
}

std::string MaxQuery::toString() { return "QUERY = MAX"; }
