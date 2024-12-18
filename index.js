function* create() {
    yield 1;
    yield 2;
    yield 3;
}
const gen = create();
Array.prototype.forEach.call(gen,(val)=>console.log(val))