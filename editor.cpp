#include "editor.h"

#include <algorithm>
#include <iostream>

#include <fmt/format.h>

// TODO: handle limits on line size and line count for safety

Editor::Editor(std::filesystem::path const &filePath) : m_editPath(filePath) {
  // TODO: check for file existence and read if present
  // TODO: only create file during write out if it isn't present
  // NOTE: if the file doesn't exist, std::fstream::trunc is needed to actually
  // create the file
  m_editFile.open(m_editPath,
                  std::fstream::in | std::fstream::out | std::fstream::trunc);
}

void Editor::display_banner() const {
  fmt::print("================================================================="
             "===========\n");
  fmt::print("Editor v0.5.0 by QuestionableDeer (2025)\n");
  fmt::print("This is a line-oriented editor like UNIX ed. Type .h for usage "
             "assistance.\n");
  fmt::print("================================================================="
             "===========\n");

  fmt::print("< Inserting at line {}. >\n", to_user_index(m_currIdx));
}

void Editor::read_line() {
  display_prompt();
  std::getline(std::cin, m_currLine);
}

void Editor::process_line() {
  if (!m_currLine.starts_with(".")) {
    // not a command, just insert in list
    m_lines.insert(m_lines.begin() + m_currIdx, m_currLine);
    m_currIdx++;

    return;
  }

  if (has_escape_seq(m_currLine)) {
    // to handle special case of lines beginning with .., .", and .:
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

  // TODO: move, find, replace
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
    fmt::print("===HELP TEXT HERE===\n");
  } else if (cinfo.command == ".i") {
    move_cursor(cinfo);
  } else if (cinfo.command == ".l") {
    print_lines(cinfo, false);
  } else if (cinfo.command == ".left") {
    align_text(cinfo, Alignment::LEFT);
  } else if (cinfo.command == ".p") {
    print_lines(cinfo, true);
  } else if (cinfo.command == ".right") {
    align_text(cinfo, Alignment::RIGHT);
  } else if (cinfo.command == ".save") {
    save_data();
  } else {
    fmt::print("< Unrecognized command \"{}\". >\n", cinfo.command);
  }
}

void Editor::display_prompt() const { fmt::print("> "); }

void Editor::save_data() {
  if (std::filesystem::exists(m_editPath)) {
    std::filesystem::resize_file(m_editPath, 0);
    m_editFile.seekp(0);
  }

  for (auto const &line : m_lines) {
    m_editFile << line << '\n';
  }

  m_editFile.flush();

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
               "See .h for details. >\n");
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

  auto [start, end] = get_inclusive_bounds(cinfo);
  // copy method uses exclusive logic
  end++;

  if (!cinfo.target.has_value()) {
    fmt::print("< Copy command require a destination line to copy to. See .h "
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

  m_lines.insert(m_lines.begin() + dest, m_lines.begin() + start,
                 m_lines.begin() + end);

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
      fmt::print("< Invalid range. No lines deleted. >\n");
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
  int searchIdx =
      cinfo.startIdx.has_value() ? cinfo.startIdx.value() : m_currIdx;

  if (!cinfo.target.has_value()) {
    fmt::print("< Find requires a search target. See .h for details. >\n");
    return;
  }

  std::string const term = cinfo.target.value();

  while (searchIdx < static_cast<int>(m_lines.size())) {
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
  // remove leading and trailing whitespace
  size_t start = line.find_first_not_of(" \t");
  if (start == std::string::npos) {
    return "";
  }

  size_t end = line.find_last_not_of(" \t");

  return line.substr(start, end - start + 1);
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
int Editor::from_user_index(int userIndex) { return userIndex - 1; }

// static
int Editor::to_user_index(int index) { return index + 1; }
