#ifndef SRC_SCHEDULER_FILERESOURCEMANAGER_H_
#define SRC_SCHEDULER_FILERESOURCEMANAGER_H_

#include <cstdint>
#include <string>

// FileResourceManager interface for per-file operation ordering
class FileResourceManager {
public:
  FileResourceManager() = default;
  ~FileResourceManager() = default;
  FileResourceManager(const FileResourceManager &) = delete;
  FileResourceManager &
  operator=(const FileResourceManager &) = delete; // NOLINT
  FileResourceManager(FileResourceManager &&) noexcept = delete;
  FileResourceManager &
  operator=(FileResourceManager &&) noexcept = delete; // NOLINT

  // Register a scheduled operation for filePath with global seq
  // Returns the dependency watermark for LOAD to wait on
  // For DUMP, return value may be ignored by callers
  auto markScheduled(const std::string &filePath,
                     std::uint64_t seq) -> std::uint64_t;

  // Block until seq-1 completed and acquire per-file execution right
  void beginFileOp(const std::string &filePath, std::uint64_t seq);

  // Release per-file lock and advance completed watermark accordingly
  void endFileOp(const std::string &filePath, std::uint64_t seq);

  // Query last completed seq for a file (0 if none).
  [[nodiscard]] auto
  lastCompleted(const std::string &filePath) const -> std::uint64_t;
};

#endif // SRC_SCHEDULER_FILERESOURCEMANAGER_H_
