"use strict";
const arr = [1, 2, 3];
const o = {};
function get() {
  return o;
}
[get()["a"]] = arr;
console.log(a)