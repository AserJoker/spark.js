const print = console.log;
class Base {
    get test() {
        return this.data;
    }
    set test(val) {
        this.data = val;
    }
}

class Test extends Base {
    static C = super.name;
}
const obj = new Test();
print(Test.C);