//
// ListenQuery implementation
//

#include "ListenQuery.h"

#include <memory>
#include <string>

#include "../../query/QueryResult.h"

auto ListenQuery::execute() -> QueryResult::Ptr {
  // Extract just the filename (without path) for the success message
  std::string displayName = this->fileName;
  auto lastSlash = displayName.find_last_of("/\\");
  if (lastSlash != std::string::npos) {
    displayName = displayName.substr(lastSlash + 1);
  }

  // Return success result with the filename
  // The actual file opening will be handled in main.cpp
  return std::make_unique<SuccessMsgResult>("ANSWER = ( listening from " +
                                            displayName + " )");
}

auto ListenQuery::toString() -> std::string {
  return "QUERY = LISTEN, FILE = \"" + this->fileName + "\"";
}
