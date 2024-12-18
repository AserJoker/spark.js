const pro = new Promise((resolve) => {
  setTimeout(() => resolve());
});
pro
  .then(() => {
    return 1;
  })
  .then((val) => {
    return val + 1;
  })
  .then((val) => {
    throw new Error(val);
  })
  .then(
    (val) => {
      return val + 1;
    },
    (err) => {
      return err;
    }
  )
  .catch((e) => print(e))
  .then((res) => {
    print(`${res}`);
  });
