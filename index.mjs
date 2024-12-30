var _class, _descriptor;
function _initializerDefineProperty(e, i, r, l) {
  r &&
    Object.defineProperty(e, i, {
      enumerable: r.enumerable,
      configurable: r.configurable,
      writable: r.writable,
      value: r.initializer ? r.initializer.call(l) : void 0
    });
}
function _defineProperties(e, r) {
  for (var t = 0; t < r.length; t++) {
    var o = r[t];
    (o.enumerable = o.enumerable || !1),
      (o.configurable = !0),
      "value" in o && (o.writable = !0),
      Object.defineProperty(e, _toPropertyKey(o.key), o);
  }
}
function _createClass(e, r, t) {
  return (
    r && _defineProperties(e.prototype, r),
    t && _defineProperties(e, t),
    Object.defineProperty(e, "prototype", { writable: !1 }),
    e
  );
}
function _classCallCheck(a, n) {
  if (!(a instanceof n))
    throw new TypeError("Cannot call a class as a function");
}
function _defineProperty(e, r, t) {
  return (
    (r = _toPropertyKey(r)) in e
      ? Object.defineProperty(e, r, {
          value: t,
          enumerable: !0,
          configurable: !0,
          writable: !0
        })
      : (e[r] = t),
    e
  );
}
function _toPropertyKey(t) {
  var i = _toPrimitive(t, "string");
  return "symbol" == typeof i ? i : i + "";
}
function _toPrimitive(t, r) {
  if ("object" != typeof t || !t) return t;
  var e = t[Symbol.toPrimitive];
  if (void 0 !== e) {
    var i = e.call(t, r || "default");
    if ("object" != typeof i) return i;
    throw new TypeError("@@toPrimitive must return a primitive value.");
  }
  return ("string" === r ? String : Number)(t);
}
function _applyDecoratedDescriptor(i, e, r, n, l) {
  var a = {};
  return (
    Object.keys(n).forEach(function (i) {
      a[i] = n[i];
    }),
    (a.enumerable = !!a.enumerable),
    (a.configurable = !!a.configurable),
    ("value" in a || a.initializer) && (a.writable = !0),
    (a = r
      .slice()
      .reverse()
      .reduce(function (r, n) {
        return n(i, e, r) || r;
      }, a)),
    l &&
      void 0 !== a.initializer &&
      ((a.value = a.initializer ? a.initializer.call(l) : void 0),
      (a.initializer = void 0)),
    void 0 === a.initializer ? (Object.defineProperty(i, e, a), null) : a
  );
}
function _initializerWarningHelper(r, e) {
  throw Error(
    "Decorating class property failed. Please ensure that transform-class-properties is enabled and runs after the decorators transform."
  );
}
var Logger = function Logger() {
  for (
    var _len = arguments.length, args = new Array(_len), _key = 0;
    _key < _len;
    _key++
  ) {
    args[_key] = arguments[_key];
  }
  console.log(args);
};
var A =
  ((_class = /*#__PURE__*/ _createClass(function A() {
    _classCallCheck(this, A);
    _initializerDefineProperty(this, "a", _descriptor, this);
  })),
  (_descriptor = _applyDecoratedDescriptor(_class.prototype, "a", [Logger], {
    configurable: true,
    enumerable: true,
    writable: true,
    initializer: function initializer() {
      return 1;
    }
  })),
  _class);