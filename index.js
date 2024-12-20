function Test() {
  throw new Error('test');
  return new Promise((resolve, reject) => {
    setTimeout(() => reject(new Error("test")));
  });
}
const print = console.log;
function Test2() {
  function* awaiter() {
    print(1);
    print(yield Test());
    print(3);
  }
  const gen = awaiter();
  return new Promise((resolve, reject) => {
    function next(res) {
      const { value, done } = gen.next(res);
      print("value");
      print(value);
      print("done");
      print(done);
      if (done) {
        return resolve(value);
      } else {
        return Promise.resolve(value)
          .then(next)
          .catch((e) => {
            print("aaa");
            gen.throw(e);
            print("bbb");
          });
      }
    }
    try {
      next();
    } catch (e) {
      reject(e);
    }
  });
}
Test2()
  .then(() => print(4))
  .catch((e) => print(e));
