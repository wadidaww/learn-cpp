#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

struct Directory {
  std::string name;
  std::vector<Directory> children;
  bool is_file;

  [[nodiscard]] bool empty() const noexcept { return children.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return children.size(); }
};

[[nodiscard]] std::vector<std::string_view> split_path(std::string_view path) {
  std::vector<std::string_view> parts;

  if (path.empty())
    return parts;

  if (path.front() == '/')
    path.remove_prefix(1);

  while (!path.empty()) {
    auto end = path.find('/');
    parts.push_back(path.substr(0, end));

    if (end == std::string_view::npos)
      break;

    path.remove_prefix(end + 1);
  }

  return parts;
}

[[nodiscard]] Directory build_tree(const std::vector<std::string> &paths) {
  Directory root{.name = "/", .children = {}, .is_file = false};

  for (const auto &raw : paths) {
    auto parts = split_path(raw);
    Directory *current = &root;

    for (std::size_t i = 0; i < parts.size(); ++i) {
      const bool last = (i == parts.size() - 1);
      std::string_view part = parts[i];

      auto it =
          std::ranges::find_if(current->children, [&part](const Directory &d) {
            return d.name == part;
          });

      if (it == current->children.end()) {
        current->children.push_back(Directory{
            .name = std::string(part),
            .children = {},
            .is_file = last,
        });
        it = std::prev(current->children.end());
      }

      current = &(*it);
    }
  }

  return root;
}

void print_tree(const Directory &node, std::uint32_t depth = 0) {
  if (depth > 0) {
    for (std::uint32_t i = 0; i < depth - 1; ++i)
      std::cout << "│   ";
    std::cout << "├── ";
  }

  std::cout << node.name;

  if (node.is_file) {
    std::cout << "  (file)";
  }

  std::cout << '\n';

  for (const auto &child : node.children) {
    print_tree(child, depth + 1);
  }
}

[[nodiscard]] std::vector<std::string> flatten(const Directory &node,
                                               std::string prefix = "") {
  std::vector<std::string> result;

  std::string current = prefix.empty()         ? "/"
                        : prefix.back() == '/' ? prefix + node.name
                                               : prefix + "/" + node.name;

  if (node.is_file) {
    result.push_back(current);
  }

  for (const auto &child : node.children) {
    auto sub = flatten(child, current);
    result.insert(result.end(), std::make_move_iterator(sub.begin()),
                  std::make_move_iterator(sub.end()));
  }

  return result;
}

void test() {
  std::vector<std::string> paths = {
      "/usr/bin/ls",
      "/usr/bin/cat",
      "/usr/lib/libc.so",
      "/usr/lib/libm.so",
      "/home/alice/docs/readme.md",
      "/home/alice/docs/notes.txt",
      "/home/alice/pictures/photo.jpg",
      "/home/bob/code/main.cpp",
  };

  Directory tree = build_tree(paths);

  assert(tree.children.size() == 2);
  assert(tree.children[0].name == "usr");
  assert(tree.children[1].name == "home");

  auto usr = &tree.children[0];
  assert(usr->children.size() == 2);

  auto usr_bin = &usr->children[0];
  assert(usr_bin->name == "bin");
  assert(usr_bin->children.size() == 2);
  assert(usr_bin->children[0].name == "ls");
  assert(usr_bin->children[0].is_file == true);

  auto usr_lib = &usr->children[1];
  assert(usr_lib->name == "lib");
  assert(usr_lib->children.size() == 2);

  auto home = &tree.children[1];
  assert(home->children.size() == 2);

  std::cout << "All tests passed.\n";
}

int main() {
  test();

  std::vector<std::string> paths = {
      "/usr/bin/ls",
      "/usr/bin/cat",
      "/usr/lib/libc.so",
      "/usr/lib/libm.so",
      "/home/alice/docs/readme.md",
      "/home/alice/docs/notes.txt",
      "/home/alice/pictures/photo.jpg",
      "/home/bob/code/main.cpp",
  };

  Directory tree = build_tree(paths);

  std::cout << "=== Directory Structure ===\n\n";
  print_tree(tree);

  std::cout << "\n=== Flattened Paths ===\n\n";
  auto flat = flatten(tree);
  for (const auto &p : flat) {
    std::cout << p << '\n';
  }

  return 0;
}
