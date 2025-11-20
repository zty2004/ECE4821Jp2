//
// ListenQuery implementation
//

#include "ListenQuery.h"

#include <filesystem>
#include <memory>
#include <string>

#include "../../query/QueryResult.h"
#include "../../utils/formatter.h"

auto ListenQuery::execute() -> QueryResult::Ptr {
  // Extract just the filename (without path) for the success message
  std::filesystem::path filePath(this->fileName);
  std::string displayName = filePath.filename().string();

  // Return success result with the filename
  // The actual file opening will be handled in main.cpp
  return std::make_unique<SuccessMsgResult>("ANSWER = ( listening from " +
                                            displayName + " )");
}

auto ListenQuery::toString() -> std::string {
  return "QUERY = LISTEN, FILE = \"" + this->fileName + "\"";
}
