#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Represents a single parsed item from the file system string.
 */
struct FileSystemEntry {
  std::string name; // Name of the file or directory
  int depth;        // Nesting level (0 for root)
  bool isFile;      // True if this entry is a file (contains '.')

  FileSystemEntry(std::string n, int d, bool f)
      : name(std::move(n)), depth(d), isFile(f) {}
};

/**
 * @brief Parser for the LeetCode 388 file system string format.
 *
 * Responsible for tokenizing the input string into FileSystemEntry objects.
 */
class FileSystemParser {
public:
  /**
   * @brief Parses the input string and returns a vector of entries.
   *
   * @param input The raw input string containing '\n' and '\t'.
   * @return std::vector<FileSystemEntry> A vector of parsed entries.
   * @throws std::invalid_argument if the input format is unexpected.
   */
  static std::vector<FileSystemEntry> parse(const std::string &input) {
    std::vector<FileSystemEntry> entries;
    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
      // 1. Count the number of leading tabs ('\t') to determine depth
      int depth = 0;
      while (i < n && input[i] == '\t') {
        ++depth;
        ++i;
      }

      // 2. Extract the entry name until a newline ('\n') or end of string
      size_t start = i;
      bool isFile = false;
      while (i < n && input[i] != '\n') {
        if (input[i] == '.') {
          isFile = true;
        }
        ++i;
      }

      // Guard against malformed input (e.g., trailing newline without name)
      if (i == start) {
        // Skip the newline if we are at the end of a line with no name.
        if (i < n && input[i] == '\n') {
          ++i;
        }
        continue;
      }

      std::string name = input.substr(start, i - start);
      entries.emplace_back(std::move(name), depth, isFile);

      // 3. Skip the newline character to move to the next line
      if (i < n && input[i] == '\n') {
        ++i;
      }
    }

    return entries;
  }
};

/**
 * @brief Solver for the Longest Absolute File Path problem.
 *
 * Encapsulates the core logic for computing the longest path to a file.
 */
class LongestAbsoluteFilePathSolver {
public:
  /**
   * @brief Computes the length of the longest absolute path to a file.
   *
   * @param input The file system representation string.
   * @return int The length of the longest absolute file path, or 0 if no file
   * exists.
   */
  int lengthLongestPath(const std::string &input) {
    if (input.empty()) {
      return 0;
    }

    auto entries = FileSystemParser::parse(input);
    if (entries.empty()) {
      return 0;
    }

    // `pathLengthAtDepth[d]` stores the length of the absolute path
    // for the directory at depth `d`. For a file at depth `d`,
    // its absolute path length is `pathLengthAtDepth[d-1] + 1 + name.length()`.
    // We use a vector sized to the maximum possible depth + 1 for safety.
    std::vector<int> pathLengthAtDepth(entries.size() + 1, 0);
    int longestPath = 0;

    for (const auto &entry : entries) {
      int currentPathLength = 0;

      if (entry.depth == 0) {
        // Root level: path length is just the name length.
        currentPathLength = static_cast<int>(entry.name.length());
      } else {
        // For deeper levels: parent path length + '/' + name length.
        // `entry.depth - 1` is the parent's depth.
        int parentPathLength = pathLengthAtDepth[entry.depth - 1];
        // The parent path length already includes the parent's name.
        // We add 1 for the '/' separator and the current name's length.
        currentPathLength =
            parentPathLength + 1 + static_cast<int>(entry.name.length());
      }

      // Store the computed path length for this depth.
      // If a sibling directory at the same depth is encountered later,
      // this value will be overwritten, which is correct.
      pathLengthAtDepth[entry.depth] = currentPathLength;

      if (entry.isFile) {
        longestPath = std::max(longestPath, currentPathLength);
      }
    }

    return longestPath;
  }
};

// The LeetCode required class and function signature.
class Solution {
public:
  int lengthLongestPath(std::string input) {
    LongestAbsoluteFilePathSolver solver;
    return solver.lengthLongestPath(input);
  }
};