function A() {
  throw "a";
}
try {
  A();
} catch (e) {
  return 2;
} finally {
}
