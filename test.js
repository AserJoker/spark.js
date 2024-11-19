"use strict";
const ob = {};
Object.defineProperty(ob, "foo", {
    configurable: false,
    value:1
})
ob.foo = 2