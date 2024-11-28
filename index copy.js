function A() {
    try {
        throw "aaa";
    } catch (e) {
        throw e + ":data";
    }
}
A()