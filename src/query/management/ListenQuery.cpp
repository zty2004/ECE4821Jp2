//
// ListenQuery implementation
//

#include "ListenQuery.h"

#include <memory>
#include <string>

#include "../../query/QueryResult.h"

auto ListenQuery::execute() -> QueryResult::Ptr {
  // Use the full path as provided in the query
  const std::string displayName = this->fileName;

  // Return success result with the full path
  // The actual file opening will be handled in main.cpp
  return std::make_unique<SuccessMsgResult>("ANSWER = ( listening from " +
                                            displayName + " )");
}

auto ListenQuery::toString() -> std::string {
  return "QUERY = LISTEN, FILE = \"" + this->fileName + "\"";
}
