function Test() {
  return new Promise((resolve) => {
    setTimeout(() => resolve(2), 1000);
  });
}
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
      if (done) {
        return resolve(value);
      } else {
        return Promise.resolve(value)
          .then(next)
          .catch((e) => reject(e));
      }
    }
    try {
      next();
    } catch (e) {
      reject(e);
    }
  });
}
Test2().then(() => print(4));
