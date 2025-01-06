class Base {
  static #data = 1;
  static write() {
    console.log(this.#data);
  }
}
class Data extends Base {}
Data.write();
