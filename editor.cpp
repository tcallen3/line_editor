#include "editor.h"

#include <algorithm>
#include <iostream>

#include <fmt/format.h>

Editor::Editor(std::filesystem::path const &filePath) : m_editPath(filePath) {
  if (std::filesystem::exists(filePath)) {
    m_editFile.open(m_editPath, std::fstream::in | std::fstream::out);
    while (getline(m_editFile, m_currLine)) {
      m_lines.push_back(m_currLine);
      m_currIdx++;
    }
  } else {
    m_editFile.open(m_editPath,
                    std::fstream::in | std::fstream::out | std::fstream::trunc);
  }
}

void Editor::display_banner() const {
  fmt::print("================================================================="
             "==============\n");
  fmt::print("Editor v{}.{}.{} by QuestionableDeer (2025)\n", kMajorVersion,
             kMinorVersion, kPatchVersion);
  fmt::print("\n");
  fmt::print("This is a line-oriented editor like UNIX ed. Type '.h' for usage "
             "assistance.\n");
  fmt::print("Typing '.end' will exit and save changes, while '.abort' will "
             "discard them.\n");
  fmt::print("To save changes and continue editing, use '.save'\n");
  fmt::print("================================================================="
             "==============\n");

  fmt::print("< Inserting at line {}. >\n", to_user_index(m_currIdx));
}

void Editor::display_help() const {
  fmt::print("-----------------------------------------------------------------"
             "--------------\n");
  fmt::print("Editor v{}.{}.{} Help\n", kMajorVersion, kMinorVersion,
             kPatchVersion);
  fmt::print("\n");
  fmt::print(
      "Any line not starting with \'.\' is inserted at the current line.\n");
  fmt::print("Lines starting with '..', '.\"', or '.:' are added with the '.' "
             "removed.\n");
  fmt::print("\n");
  fmt::print("In the usage below, arguments in '[]' are optional.\n");
  fmt::print("Abbreviations: sl - start line, el - end line, dl - destination "
             "line.\n");
  fmt::print("Relative line references: ^ - first line, . - current line, $ - "
             "last line\n");
  fmt::print("-----------------------------------------------------------------"
             "--------------\n");

  fmt::print(".end                      Exits after saving changes.\n");
  fmt::print(".abort                    Exits without saving.\n");
  fmt::print(".h                        Displays this help.\n");
  fmt::print(".l [sl [el]]              Lists lines in the range provided "
             "(empty = list all).\n");
  fmt::print(".p [sl [el]]              Like '.l' but prints line numbers.\n");
  fmt::print(
      ".del [sl [el]]            Deletes the given lines (empty = current).\n");
  fmt::print(
      ".copy [sl [el]]=dl        Copies selected lines to destination.\n");
  fmt::print(
      ".move [sl [el]]=dl        Moves selected lines to destination.\n");
  fmt::print(".find [sl]=text           Searches for given text starting "
             "at specified line.\n");
  fmt::print(
      ".replace [sl [el]]=/old/new/  Replaces old text with new in given "
      "range.\n");
  fmt::print(".left [sl [el]]           Align text in range to the left.\n");
  fmt::print(".center [sl [el]]=cols    Center text in range to given column "
             "width.\n");
  fmt::print(".right [sl [el]]=cols     Align text in range to right for given "
             "column width.\n");
  fmt::print("-----------------------------------------------------------------"
             "--------------\n");
}

void Editor::read_line() {
  display_prompt();
  std::getline(std::cin, m_currLine);
}

void Editor::process_line() {
  // not a command, just insert in list
  if (!m_currLine.starts_with(".")) {
    if (m_lines.size() >= kMaxSize) {
      fmt::print("< Max list size reached. Inserts not permitted. >\n");
      return;
    }
    m_lines.insert(m_lines.begin() + m_currIdx, m_currLine);
    m_currIdx++;

    return;
  }

  // to handle special case of lines beginning with .., .", and .:
  if (has_escape_seq(m_currLine)) {
    if (m_lines.size() >= kMaxSize) {
      fmt::print("< Max list size reached. Inserts not permitted. >\n");
      return;
    }

    m_lines.insert(m_lines.begin() + m_currIdx, m_currLine.substr(1));
    m_currIdx++;

    return;
  }

  CommandInfo cinfo = parse_command_info(m_currLine);

  if (cinfo.startIdx.has_value() && cinfo.startIdx.value() == -1) {
    fmt::print("< Invalid starting index. >\n");
    return;
  }

  if (cinfo.endIdx.has_value() && cinfo.endIdx.value() == -1) {
    fmt::print("< Invalid ending index. >\n");
    return;
  }

  if (cinfo.command == ".abort") {
    m_inLoop = false;
    fmt::print("< Editing aborted. >\n");
    fmt::print("< Lines not saved. >\n");
  } else if (cinfo.command == ".center") {
    align_text(cinfo, Alignment::CENTER);
  } else if (cinfo.command == ".copy") {
    copy_lines(cinfo);
  } else if (cinfo.command == ".del") {
    delete_lines(cinfo);
  } else if (cinfo.command == ".end") {
    fmt::print("< Editing finished. >\n");
    save_data();
    m_inLoop = false;
  } else if (cinfo.command == ".find") {
    find_string(cinfo);
  } else if (cinfo.command == ".h") {
    display_help();
  } else if (cinfo.command == ".i") {
    move_cursor(cinfo);
  } else if (cinfo.command == ".l") {
    print_lines(cinfo, false);
  } else if (cinfo.command == ".left") {
    align_text(cinfo, Alignment::LEFT);
  } else if (cinfo.command == ".move") {
    move_lines(cinfo);
  } else if (cinfo.command == ".p") {
    print_lines(cinfo, true);
  } else if (cinfo.command == ".replace") {
    replace(cinfo);
  } else if (cinfo.command == ".right") {
    align_text(cinfo, Alignment::RIGHT);
  } else if (cinfo.command == ".save") {
    save_data();
  } else {
    fmt::print("< Unrecognized command \'{}\'. >\n", cinfo.command);
  }
}

void Editor::display_prompt() const { fmt::print("> "); }

void Editor::save_data() {
  std::filesystem::path tempPath =
      m_editPath.parent_path() /
      std::filesystem::path{m_editPath.stem().string() + ".bak"};

  std::fstream tempFile;
  tempFile.open(tempPath,
                std::fstream::in | std::fstream::out | std::fstream::trunc);

  for (auto const &line : m_lines) {
    tempFile << line << '\n';
  }

  tempFile.flush();

  m_editFile.close();
  std::filesystem::rename(tempPath, m_editPath);

  fmt::print("< Lines successfully saved. >\n");
}

void Editor::print_lines(CommandInfo const &cinfo, bool useLineNumbers) const {
  auto [idx, end] = get_inclusive_bounds(cinfo);
  int start = idx;

  if (idx > end) {
    fmt::print(
        "< Warning: starting index larger than ending index, no text will "
        "be printed. >\n");
  }

  while (idx <= end) {
    if (useLineNumbers) {
      fmt::print("{: >3d}: ", to_user_index(idx));
    }
    fmt::print("{}\n", m_lines[idx]);
    idx++;
  }

  fmt::print("< Listed {} lines, starting at line {}. >\n", end - start + 1,
             to_user_index(start));
}

void Editor::align_text(CommandInfo const &cinfo, Alignment alignType) {
  auto [idx, end] = get_inclusive_bounds(cinfo);
  int const start = idx;

  if (idx > end) {
    fmt::print("< Warning: starting index larger than ending index, no text "
               "will be formatted. >\n");
  }

  if (!cinfo.target.has_value()) {
    fmt::print("< Formatting commands require a number of columns to align to. "
               "See '.h' for details. >\n");
    return;
  }

  int cols = -1;
  try {
    cols = std::stoi(cinfo.target.value());
  } catch (std::exception const &e) {
    fmt::print(
        "< Invalid number of columns for alignment. No lines changed. >\n");
    return;
  }

  while (idx <= end) {
    m_lines[idx] = format_line(trim(m_lines[idx]), cols, alignType);
    idx++;
  }

  fmt::print("< Formatted {} lines, starting at line {}. >\n", end - start + 1,
             start);
}

void Editor::move_cursor(CommandInfo const &cinfo) {
  if (!cinfo.startIdx.has_value()) {
    fmt::print("< You must specify a valid line index to insert at. >\n");
    return;
  }

  m_currIdx = cinfo.startIdx.value();
  fmt::print("< Inserting at line {}. >\n", to_user_index(m_currIdx));
}

void Editor::copy_lines(CommandInfo const &cinfo) {
  if (m_lines.size() >= kMaxSize) {
    fmt::print("< Max list size reached. Copying not permitted. >\n");
    return;
  }

  auto [start, end] = get_inclusive_bounds(cinfo);
  if (start > end) {
    fmt::print("< Warning: starting index larger than ending index, no text "
               "will be copied. >\n");
    return;
  }
  // copy method uses exclusive logic
  end++;

  if (!cinfo.target.has_value()) {
    fmt::print("< Copy command require a destination line to copy to. See '.h' "
               "for details. >\n");
    return;
  }

  int dest = -1;
  try {
    dest = from_user_index(std::stoi(cinfo.target.value()));
  } catch (std::exception const &e) {
    fmt::print("< Invalid target line for copy. No lines changed. >\n");
    return;
  }

  if (dest < 0 || dest > static_cast<int>(m_lines.size())) {
    fmt::print("< Invalid target for line copy. No lines changed. >\n");
    return;
  }

  // make a copy to avoid iterator invalidation
  std::vector<std::string> tempLines(m_lines.begin() + start,
                                     m_lines.begin() + end);

  // now insert directly
  m_lines.insert(m_lines.begin() + dest, tempLines.begin(), tempLines.end());

  // set to sane insert point
  m_currIdx = dest;

  fmt::print("< Copied {} lines at position {}. >\n", end - start,
             to_user_index(dest));
  fmt::print("< Inserting lines at position {}. >\n", to_user_index(m_currIdx));
}

void Editor::delete_lines(CommandInfo const &cinfo) {
  if (!cinfo.startIdx.has_value()) {
    // for delete we only remove current line on empty range input
    if (m_currIdx >= static_cast<int>(m_lines.size())) {
      // do nothing if we're on a blank line
      return;
    }
    m_lines.erase(m_lines.begin() + m_currIdx);
    fmt::print("< Deleted 1 line, at position {}. >\n",
               to_user_index(m_currIdx));
  } else {
    auto [start, end] = get_inclusive_bounds(cinfo);
    if (start >= static_cast<int>(m_lines.size())) {
      return;
    }

    if (start > end) {
      fmt::print("< Warning: starting index larger than ending index. No lines "
                 "deleted. >\n");
      return;
    }

    // erase method uses half-open interval so we adjust the end
    end++;
    m_lines.erase(m_lines.begin() + start, m_lines.begin() + end);
    fmt::print("< Deleted {} lines, starting at line {}. >\n", end - start,
               to_user_index(start));

    // move to safe offset
    m_currIdx = start;
  }

  fmt::print("< Inserting at line {}. >\n", to_user_index(m_currIdx));
}

void Editor::find_string(CommandInfo const &cinfo) {
  auto [searchIdx, end] = get_inclusive_bounds(cinfo);

  if (!cinfo.startIdx.has_value()) {
    searchIdx = m_currIdx;
    end = m_currIdx;
  }

  if (!cinfo.target.has_value()) {
    fmt::print("< Find requires a search target. See '.h' for details. >\n");
    return;
  }

  // NOTE: should we allow for quoted search terms to enable whitespace
  // matching?
  std::string const term = trim(cinfo.target.value());

  while (searchIdx <= end) {
    if (m_lines[searchIdx].find(term) != std::string::npos) {
      m_currIdx = searchIdx;
      fmt::print("< Found match at line {}. Moving there for edits. >\n",
                 to_user_index(m_currIdx));
      return;
    }
    searchIdx++;
  }

  fmt::print("< No matches found. Inserting at line {}. >\n",
             to_user_index(m_currIdx));
}

void Editor::replace(CommandInfo const &cinfo) {
  if (m_lines.size() >= kMaxSize) {
    fmt::print("< Max list size reached. Replacements not permitted. >\n");
    return;
  }

  auto [searchIdx, end] = get_inclusive_bounds(cinfo);
  if (searchIdx > end) {
    fmt::print("< Warning: starting index larger than ending index. No text "
               "replaced. >\n");
    return;
  }

  if (!cinfo.startIdx.has_value()) {
    searchIdx = m_currIdx;
    end = m_currIdx;
  }

  if (!cinfo.target.has_value()) {
    fmt::print("< Replace requires target text to search and replace. See '.h' "
               "for details. >\n");
    return;
  }

  std::string const replaceArgs = trim(cinfo.target.value());

  auto [target, replacement] = parse_replace(replaceArgs);
  if (target.empty()) {
    fmt::print("< Replace requires target text to search and replace. See '.h' "
               "for details. >\n");
    return;
  }

  int rCount = 0;
  size_t pos = 0;
  while (searchIdx <= end) {
    if ((pos = m_lines[searchIdx].find(target)) != std::string::npos) {
      m_lines[searchIdx].replace(pos, target.size(), replacement);
      rCount++;
    }

    searchIdx++;
  }

  fmt::print("< Replaced text on {} lines. >\n", rCount);
}

void Editor::move_lines(CommandInfo const &cinfo) {
  auto [blockStart, blockEnd] = get_inclusive_bounds(cinfo);
  if (blockStart > blockEnd) {
    fmt::print("< Warning: starting index larger than ending index. No lines "
               "moved. >\n");
    return;
  }

  if (!cinfo.target.has_value()) {
    fmt::print("< Move command require a destination line to move to. See '.h' "
               "for details. >\n");
    return;
  }

  int dest = -1;
  try {
    dest = from_user_index(std::stoi(cinfo.target.value()));
  } catch (std::exception const &e) {
    fmt::print("< Invalid target line for move. No lines changed. >\n");
    return;
  }

  if (dest < 0 || dest > static_cast<int>(m_lines.size())) {
    fmt::print("< Invalid target line for move. No lines changed. >\n");
    return;
  }

  if (dest >= blockStart && dest <= blockEnd) {
    // moving a block within itself amounts to a no-op in our model
    fmt::print(
        "< Moving a block within itself has no effect. No lines changed. >\n");
    return;
  }

  // first copy to new vector to avoid iterator invalidation
  std::vector<std::string> copiedLines(m_lines.begin() + blockStart,
                                       m_lines.begin() + blockEnd + 1);

  m_lines.insert(m_lines.begin() + dest, copiedLines.begin(),
                 copiedLines.end());

  // two delete cases: lines moved earlier and lines moved later
  if (dest < blockStart) {
    // we have to add block size to get refs to delete
    int size = blockEnd - blockStart + 1;
    m_lines.erase(m_lines.begin() + blockStart + size,
                  m_lines.begin() + blockEnd + size + 1);

  } else {
    // lines to delete haven't changed index since we added after them
    m_lines.erase(m_lines.begin() + blockStart, m_lines.begin() + blockEnd + 1);
  }

  fmt::print("< Moved {} lines. Inserting at line {}. >\n",
             blockEnd - blockStart + 1, m_currIdx);
}

int Editor::translate_anchors(std::string const &anchor) const {
  if (anchor == "^") {
    return 0;
  } else if (anchor == "$") {
    return m_lines.size();
  } else if (anchor == "=") {
    return m_currIdx;
  }

  int anchorIdx = -1;
  try {
    anchorIdx = std::stoi(anchor);
  } catch (std::exception const &e) {
    return -1;
  }

  int internalIdx = std::max(0, std::min(from_user_index(anchorIdx),
                                         static_cast<int>(m_lines.size())));
  return internalIdx;
}

std::pair<int, int>
Editor::get_inclusive_bounds(CommandInfo const &cinfo) const {
  int start = cinfo.startIdx.has_value() ? cinfo.startIdx.value() : 0;
  int end = cinfo.endIdx.has_value() ? inclusive_limit(cinfo.endIdx.value())
                                     : inclusive_limit(m_lines.size());

  return {start, end};
}

std::string Editor::format_line(std::string const &line, int colWidth,
                                Alignment alignType) const {
  // for left-aligned text, we can always at least eat starting whitespace
  if (alignType == Alignment::LEFT) {
    return line.substr(line.find_first_not_of(" \t"));
  }

  // for other alignments we leave overflowing lines as they are
  if (static_cast<int>(line.size()) >= colWidth) {
    return line;
  }

  int diff = colWidth - line.size();
  if (alignType == Alignment::RIGHT) {
    std::string padding(diff, ' ');
    return padding + line;
  }

  // alignment is CENTER
  int preCount = diff / 2;
  int postCount = diff - preCount;

  std::string prePad(preCount, ' ');
  std::string postPad(postCount, ' ');

  return prePad + line + postPad;
}

Editor::CommandInfo Editor::parse_command_info(std::string const &line) {
  // general pattern should be: <cmd> [start [end]][=target]
  CommandInfo info = {};

  auto const &[cmd, rest] = split_on_first(line, " \t");
  info.command = cmd;

  // rest is [start [end]][=target]
  auto const &[range, target] = split_on_first(rest, "=");
  if (!target.empty()) {
    info.target = target;
  }

  // range is [start [end]]
  auto const &[start, end] = split_on_first(trim(range), " \t");

  if (!start.empty()) {
    info.startIdx = translate_anchors(start);
  }

  if (!end.empty()) {
    info.endIdx = translate_anchors(end);
  }

  return info;
}

int Editor::inclusive_limit(int val) const {
  // for printing we use closed intervals (rather than C-default half-open)
  return std::min(val, static_cast<int>(m_lines.size()) - 1);
}

// static
std::string Editor::trim(std::string const &line) {
  return trim_right(trim_left(line));
}

// static
std::string Editor::trim_left(std::string const &line) {
  size_t start = line.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }

  return line.substr(start);
}

// static
std::string Editor::trim_right(std::string const &line) {
  size_t end = line.find_last_not_of(" \t");
  if (end == std::string::npos) {
    return "";
  }

  return line.substr(0, end + 1);
}

// static
bool Editor::has_escape_seq(std::string const &line) {
  return line.starts_with("..") || line.starts_with(".\"") ||
         line.starts_with(".:");
}

// static
std::pair<std::string, std::string>
Editor::split_on_first(std::string const &line, std::string const &patt) {
  size_t idx = 0;
  size_t end = line.find_first_of(patt, idx);
  if (end == std::string::npos) {
    return {line, ""};
  }

  // this range will exclude the split-on character
  std::string first = line.substr(idx, end - idx);

  idx = line.find_first_not_of(patt, end);
  if (idx == std::string::npos) {
    return {first, ""};
  }

  std::string rest = line.substr(idx);

  return {first, rest};
}

// static
std::pair<std::string, std::string>
Editor::parse_replace(std::string const &line) {
  // Expected format: /<old>/<new>/

  // deal with special case of empty <old> text (not allowed)
  if (line.starts_with("//")) {
    return {};
  }

  // input -> <?>/<old>/<new>/
  auto [pre, argPair] = split_on_first(line, "/");
  if (!pre.empty()) {
    // text before initial '/' not supported
    return {};
  }

  // argPair -> <old>/<new>/
  auto [old, rest] = split_on_first(argPair, "/");
  if (old.empty()) {
    // unsupported
    return {};
  }

  // old -> <old>, rest -> <new>/
  // NOTE: empty <new> is supported as a way to delete target text
  auto [replacement, _] = split_on_first(rest, "/");

  return {old, replacement};
}

// static
int Editor::from_user_index(int userIndex) { return userIndex - 1; }

// static
int Editor::to_user_index(int index) { return index + 1; }
