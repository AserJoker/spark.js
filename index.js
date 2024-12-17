const pro = new Promise((resolve, reject) => {
  setTimeout(() => resolve());
});
pro.then(
  () => {
    throw new Error("test");
  },
  (err) => {
    console.log("on error");
    console.log(err);
  }
);
