"use strict";
function* Test() {
  let a = 1;
  try {
    a = yield 1;
  } finally {
    console.log("finially");
  }
  return a + 1;
}
const g = Test();
console.log(g.next().value);
g.throw(new Error("Test error"));
