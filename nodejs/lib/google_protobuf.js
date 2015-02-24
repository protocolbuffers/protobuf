// This is the high-level 'google_protobuf' extension. It imports the C++ module
// and then adds additional JavaScript code.

// ------------------------ Base C++ extension -----------------------

// Import the low-level C++ extension and re-export all of its exports.
var google_protobuf_cpp = require('google_protobuf_cpp');
for (var exportName in google_protobuf_cpp) {
  exports[exportName] = google_protobuf_cpp[exportName];
}

// ------------------------ ReadOnlyArray -----------------------
exports.ReadOnlyArray.prototype.forEach = Array.prototype.forEach;

// ------------------------ Array -----------------------

// Add higher-level Array functions to RepeatedField.
//
// We can steal some of these from Array directly, but others are not
// reusable in their original Array implementations or require some modification
// to do stricter typechecking (in the spirit of RepeatedField's
// strong-typed-ness), or are ES6-specific and we wish to be ES5-compatible.
exports.RepeatedField.prototype.concat = function(other) {
  var ret = this.newEmpty();
  for (var i = 0; i < this.length; i++) {
    ret.push(this[i]);
  }
  for (var i = 0; i < other.length; i++) {
    ret.push(other[i]);
  }
  return ret;
}

exports.RepeatedField.prototype.copyWithin = function(target, start, end) {
  if (arguments.length < 3) {
    end = this.length;
  }
  if (start < 0) {
    start += this.length;
  }
  if (end < 0) {
    end += this.length;
  }
  var src = start;
  for (var i = target; i < this.length && src < end; i++, src++) {
    this[i] = this[src];
  }
  return this;
}

exports.RepeatedField.prototype.every = Array.prototype.every;

exports.RepeatedField.prototype.fill = function(value, start, end) {
  if (arguments.length < 3) {
    end = this.length;
  }
  if (arguments.length < 2) {
    start = 0;
  }
  if (start < 0) {
    start += this.length;
  }
  if (end < 0) {
    end += this.length;
  }
  for (var i = start; i < end; i++) {
    this[i] = value;
  }
  return this;
}

exports.RepeatedField.prototype.filter = Array.prototype.filter;

exports.RepeatedField.prototype.find = function(cb, thisArg) {
  var idx = this.findIndex(cb, thisArg);
  if (idx == -1) {
    return undefined;
  } else {
    return this[idx];
  }
}

exports.RepeatedField.prototype.findIndex = function(cb, thisArg) {
  if (arguments.length < 2) {
    thisArg = null;
  }

  for (var i = 0; i < this.length; i++) {
    if (cb.call(thisArg, this[i])) {
      return i;
    }
  }
  return -1;
}

exports.RepeatedField.prototype.forEach = Array.prototype.forEach;

function trueEq(a, b) {
  // Equality as defined by .includes(). Handles NaN appropriately -- NaN is
  // equal to NaN by this definition, even though it is not by IEEE 754.
  return (a === b) ||
         (a !== a && b !== b);
}

exports.RepeatedField.prototype.includes = function(elem, from) {
  if (arguments.length < 2) {
    from = 0;
  }
  for (var i = from; i < this.length; i++) {
    if (trueEq(this[i], elem)) {
      return true;
    }
  }
  return false;
}

exports.RepeatedField.prototype.indexOf = Array.prototype.indexOf;

exports.RepeatedField.prototype.join = Array.prototype.join;

exports.RepeatedField.prototype.lastIndexOf = Array.prototype.lastIndexOf;

exports.RepeatedField.prototype.map = Array.prototype.map;

exports.RepeatedField.prototype.reduce = Array.prototype.reduce;

exports.RepeatedField.prototype.reduceRight = Array.prototype.reduceRight;

exports.RepeatedField.prototype.reverse = function() {
  for (var i = 0; i < this.length/2; i++) {
    var tmp = this[i];
    this[i] = this[this.length - i - 1];
    this[this.length - i - 1] = tmp;
  }
  return this;
}

exports.RepeatedField.prototype.some = Array.prototype.some;

exports.RepeatedField.prototype.sort = function(compare) {
  if (arguments.length < 1) {
    compare = function(a, b) {
      var aStr = a.toString();
      var bStr = b.toString();
      if (aStr < bStr) {
        return -1;
      } else if (aStr > bStr) {
        return 1;
      } else {
        return 0;
      }
    };
  }

  function swap(i, j) {
    var tmp = this[i];
    this[i] = this[j];
    this[j] = tmp;
  }

  // Sort elements in [lo, hi] inclusive.
  function subsort(lo, hi) {
    if (lo >= hi) {
      return;
    }
    // Choose pivot as last element; for all other elements, move toward left if
    // less than pivot.
    var pivot = this[hi];
    var out = lo;
    for (var i = lo; i < hi; i++) {
      if (compare(this[i], pivot) < 0) {
        swap.call(this, i, out);
        out++;
      }
    }
    // Put pivot in place.
    swap.call(this, hi, out);
    // Recursively sort subranges.
    subsort.call(this, lo, out - 1);
    subsort.call(this, out + 1, hi);
  }

  subsort.call(this, 0, this.length - 1);
  return this;
}

exports.RepeatedField.prototype.slice = function(begin, end) {
  if (arguments.length < 2) {
    end = this.length;
  }
  if (arguments.length < 1) {
    begin = 0;
  }
  if (begin < 0) {
    begin += this.length;
  }
  if (end < 0) {
    end += this.length;
  }
  var ret = this.newEmpty();
  for (var i = begin; i < end; i++) {
    ret.push(this[i]);
  }
  return ret;
}

exports.RepeatedField.prototype.splice = function(start, deleteCount) {
  // Extract elements to add from varargs after the two index args.
  var toAdd = [];
  for (var i = 2; i < arguments.length; i++) {
    toAdd.push(arguments[i]);
  }
  // Extract the array that we'll return of deleted elements.
  var del = this.slice(start, start + deleteCount);
  // Create the new tail.
  var newTail = toAdd.concat.apply(toAdd, this.slice(start + deleteCount));
  // Edit our tail.
  this.resize(start + newTail.length);
  for (var i = 0; i < newTail.length; i++) {
    this[start + i] = newTail[i];
  }
  return del;
}

// ------------------------ Map -----------------------

exports.Map.prototype.forEach = function(cb, thisArg) {
  if (arguments.length < 2) {
    thisArg = null;
  }

  var keys = this.keys;
  for (var i = 0; i < keys.length; i++) {
    var key = keys[i];
    cb.call(thisArg, key, this.get(key), this);
  }
}
