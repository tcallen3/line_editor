#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "editor.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " <file>\n";
    std::exit(EXIT_FAILURE);
  }

  std::filesystem::path editPath = argv[1];

  Editor editor(editPath);
  editor.display_banner();

  while (editor.continue_loop()) {
    editor.read_line();
    editor.process_line();
  }

  return 0;
}
