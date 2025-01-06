#pragma once
namespace spark::vm {
enum JSRegExpFlag {
  GLOBAL = 1 << 0,
  ICASE = 1 << 1,
  MULTILINE = 1 << 2,
  DOTALL = 1 << 3,
  UNICODE = 1 << 4,
  STICKY = 1 << 5
};
};