#include "MinQuery.h"

QueryResult::Ptr MinQuery::execute() {
  return std::make_unique<NullQueryResult>();
}

std::string MinQuery::toString() { return "QUERY = MIN"; }
