async function* generator() {
  yield new Promise((resolve) => setTimeout(() => resolve(1), 1000));
  yield new Promise((resolve) => setTimeout(() => resolve(2), 1000));
  return 3;
}
const print = console.log;
async function test() {
  for await (const a of generator()) {
    print(a);
  }
}
test();
