const print = console.log;
var reg = /a/g;
let res = reg.exec("aaa");
print(`[0: ${res[0]}, index: ${res.index}, groups:${res.groups}]`);
res = reg.exec("aaa");
print(`[0: ${res[0]}, index: ${res.index}, groups:${res.groups}]`);
res = reg.exec("aaa");
print(`[0: ${res[0]}, index: ${res.index}, groups:${res.groups}]`);
