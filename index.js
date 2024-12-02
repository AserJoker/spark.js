'use strict'
function A() {
    arguments.length = 10
    console.log(Object.keys(arguments),arguments.length);
}
A(1)