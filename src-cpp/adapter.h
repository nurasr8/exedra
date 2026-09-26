#pragma once
// Binary-adapter: fix up foreign trees (shebangs, .desktop, symlinks),
// validate ELF arch, unpack archives.
#include <string>
#include <vector>

namespace adapter {

void adaptTree(const std::string& staging);
void validateTree(const std::string& staging, const std::string& pkgdir,
                  std::vector<std::string>& errors);
void unpack(const std::string& archive, const std::string& dest);

}  // namespace adapter
