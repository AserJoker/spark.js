"use strict";
function A() {
  try {
    throw new Error(1);
  } catch (e) {
    throw new Error(2);
  } finally {
    return;
  }
}
A();
