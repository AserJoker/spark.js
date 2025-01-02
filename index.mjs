class Base {
  a = 2;
  #a = 1;
  get data() {
    return this.#a;
  }
  #print() {
    print("hello world");
  }
  write() {
    return () => {
      this.#print();
    };
  }
}
const o = new Base();
const obj = {};
o.write.call(obj)();