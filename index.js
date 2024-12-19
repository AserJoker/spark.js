const pro = Promise.resolve(123);
const print = console.log;
pro
  .then((val) => {
    print(val);
    return new Promise((resolve) => {
      setTimeout(() => resolve(234), 1000);
    });
  })
  .then((res) => {
    print(res);
  }).finally(() => {
    print('finally')
  });
