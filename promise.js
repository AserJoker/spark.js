function onSettled() {
  this._finallys.forEach((fn) => {
    fn();
  });
  this._callbacks.forEach((fn) => {
    fn(this._value);
  });
}

function onRejected() {
  this._finallys.forEach((fn) => {
    fn();
  });
  this._errors.forEach((fn) => {
    fn(this._value);
  });
}

function pipeline(resolve, reject, callback, value) {
  try {
    resolve(callback(value));
  } catch (e) {
    reject(e);
  }
}

function onNextResolve(resolve, reject, callback, value) {
  if (typeof callback == "function") {
    pipeline(resolve, reject, callback, value);
  } else {
    resolve(value);
  }
}

function onNextReject(resolve, reject, callback, value) {
  if (typeof callback == "function") {
    pipeline(resolve, reject, callback, value);
  } else {
    reject(value);
  }
}

function Promise(receiver) {
  this._callbacks = [];
  this._errors = [];
  this._finallys = [];
  this._value = undefined;
  this._status = "pendding";
  const resolve = (val) => {
    if (this._status == "pendding") {
      this._value = val;
      this._status = "fulfilled";
      nextTick(() => onSettled.bind(this));
    }
  };
  const reject = (err) => {
    if (this._status == "pendding") {
      this._value = err;
      this._status = "fulfilled";
      nextTick(() => onRejected.bind(this));
    }
  };
  receiver(resolve, reject);
}

Promise.prototype.then = function (callback, onError) {
  if (this._status == "fulfilled") {
    if (typeof callback == "function") {
      return new Promise((resolve, reject) => {
        nextTick(() => pipeline(resolve, reject, callback, this._value));
      });
    } else {
      return Promise.resolve(this._value);
    }
  } else if (this._status == "rejected") {
    if (typeof onError == "function") {
      return new Promise((resolve, reject) => {
        nextTick(() => pipeline(resolve, reject, onError, this._value));
      });
    } else {
      return Promise.reject(this._value);
    }
  } else {
    return new Promise((resolve, reject) => {
      this._callbacks.push((val) =>
        onNextResolve(resolve, reject, callback, val)
      );
      this._errors.push((err) => onNextReject(resolve, reject, onError, err));
    });
  }
};
Promise.prototype.catch = function (onError) {
  if (this._status == "rejected") {
    if (typeof onError == "function") {
      return new Promise((resolve, reject) => {
        nextTick(() => {
          pipeline(resolve, reject, onError, this._value);
        });
      });
    } else {
      return Promise.reject(this._value);
    }
  } else if (this._status == "pendding") {
    return new Promise((resolve, reject) => {
      this._errors.push((err) => onNextReject(resolve, reject, onError, err));
      this._callbacks.push(resolve);
    });
  } else {
    return Promise.resolve(this._value);
  }
};
Promise.prototype.finally = function (callback) {
  if (this._status == "pendding") {
    return new Promise((resolve, reject) => {
      this._finallys.push(() =>
        onNextResolve(resolve, reject, callback, undefined)
      );
    });
  } else {
    if (typeof callback == "function") {
      return new Promise((resolve, reject) => {
        nextTick(() => {
          pipeline(resolve, reject, callback, undefined);
        });
      });
    } else {
      return Promise.resolve();
    }
  }
};
Promise.resolve = function (val) {
  return new Promise((resolve) => resolve(val));
};
Promise.reject = function (err) {
  return new Promise((_, reject) => reject(err));
};
const pro = new Promise((resolve) => {
  setTimeout(() => resolve());
});
pro
  .then(() => {
    return 1;
  })
  .then((val) => {
    return val + 1;
  })
  .then((val) => {
    throw new Error(val);
  })
  .then(
    (val) => {
      return val + 1;
    },
    (err) => {
      return err;
    }
  )
  .catch((e) => print(e))
  .then((res) => {
    print(typeof res);
  });
