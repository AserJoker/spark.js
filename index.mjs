function* generator() {
  yield 1;
  yield 2;
  return 3;
}
const print = console.log;
async function test() {
  for await (const a of generator()) {
    print(a);
  }
}
test();
