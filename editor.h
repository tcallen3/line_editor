#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Editor {
public:
  Editor(std::filesystem::path const &filePath);
  ~Editor() { m_editFile.close(); }

  Editor(Editor const &) = delete;
  Editor &operator=(Editor const &) = delete;
  Editor(Editor &&) = default;
  Editor &operator=(Editor &&) = default;

  bool continue_loop() const { return m_inLoop; }
  void display_banner() const;
  void read_line();
  void process_line();

private:
  enum class Alignment { LEFT, RIGHT, CENTER };
  struct CommandInfo {
    std::string command = {};
    std::optional<int> startIdx = {};
    std::optional<int> endIdx = {};
    std::optional<std::string> target = {};
  };

  void display_prompt() const;
  void save_data();
  void print_lines(CommandInfo const &cinfo, bool useLineNumbers) const;
  void align_text(CommandInfo const &cinfo, Alignment alignType);
  void move_cursor(CommandInfo const &cinfo);
  void copy_lines(CommandInfo const &cinfo);
  void delete_lines(CommandInfo const &cinfo);
  int translate_anchors(std::string const &anchor) const;
  std::pair<int, int> get_inclusive_bounds(CommandInfo const &cinfo) const;
  std::string format_line(std::string const &line, int colWidth,
                          Alignment alignType) const;
  CommandInfo parse_command_info(std::string const &line);
  int inclusive_limit(int val) const;
  static std::string trim(std::string const &line);
  static bool has_escape_seq(std::string const &line);
  static std::pair<std::string, std::string>
  split_on_first(std::string const &line, std::string const &patt);
  static int from_user_index(int userIndex);
  static int to_user_index(int index);

  bool m_inLoop = true;
  int32_t m_currIdx = 0;
  std::filesystem::path m_editPath;
  std::fstream m_editFile = {};
  std::string m_currLine = {};
  std::vector<std::string> m_lines = {};
};
