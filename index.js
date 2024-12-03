"use strict";
function* T() {
  yield 1;
}
const g = T();
console.log(g.next(2))