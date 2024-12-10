"use strict";
const obj = {
  [Symbol.iterator]() {
    return {
      index: 0,
      next() {
        if (this.index < 5) {
          return { value: this.index++, done: false };
        } else {
          return { value: this.index, done: true };
        }
      },
    };
  },
};
function* test() {
  yield* obj;
}
const g = test();
console.log(g.next().value);
console.log(g.next().value);
