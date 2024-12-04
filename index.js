"use strict";
function A() {
  try {
    throw new Error(1);
  } catch (e) {
  } finally {
  }
  console.log(e);
}
A();
