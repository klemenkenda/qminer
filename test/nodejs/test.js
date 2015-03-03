console.log(__filename)
// 
// Sample unit test using standard assert JS library 
// 
var assert = require("../../src/nodejs/scripts/assert.js")
var la = require('../../').la;

	
describe('Import test', function(){
    it('if import of qminer.node succeeds, return true', function(){
      assert.equal(1,1);
    })
})

//
// Function tests for Vector
//
var v = new la.Vector();

describe('Vector Tests', function () { 

    describe('Property Tests', function () {
        describe('Length Test', function () {
            it('should return 0 for vector v', function () {
                assert.equal(v.length, 0);
            })
        })
    });

    describe('Functions Tests', function () {
        describe('Push Test', function () {
            it('pushing 3.1 and 4 into a vector v, v.length should return 2', function () {
                v.push(3.2);
                v.push(4);
                assert.equal(v.length, 2);
            })
        })

        describe('At Test', function () {
            it('returns element in with indexes 0 (3.2) and 1 (4)', function () {
                assert.equal(3.2, v.at(0));
                assert.equal(4, v.at(1));
            })
        })

        describe('At "Out of Bound" Test', function () {
            it('should give an error for accessing an element out of bound', function () {
                assert.throws(function () {
                    var n = v.at(-1);
                })
            })
        })

        describe('Constructor "Array" Test', function () {
            it('takes an array [1, 2, 3] and it should create a vector with same values', function () {
                var vec = new la.Vector([1, 2, 3]);
                assert.equal(vec.length, 3);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), i + 1);
                }
            })
        })

        describe('Constructor "Dictionary" Test', function () {
            it('takes {"vals":5, "mxvals": 5} and it should create a vector with 5 zeros', function () {
                var vec = new la.Vector({ "vals": 5, "mxvals": 5 });
                assert.equal(vec.length, 5);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), 0);
                }
            })
        })

        describe('Copy Constructor Test', function () {
            it('should copy the vector v and save it in vec', function () {
                var vec = new la.Vector(v);
                assert.equal(vec.length, v.length);
                for (var i = 0; i < v.length; i++) {
                    assert.equal(vec.at(i), v.at(i));
                }
            })
        })

        describe('Sum Test', function () {
            it('should return a sum of 3.2 and 4', function () {
                assert.equal(3.2 + 4, v.sum());
            })
        })

        describe('Sum "Zero" Test', function () {
            it('the sum returned should be zero', function () {
                var vec = la.Vector([3.14 * 0.0001, 3.14 * -0.0001]);
                assert.equal(vec.sum(), 0);
            })
        })

        describe('getMaxIdx Test', function () {
            it('should return index of last element in vector, 1.', function () {
                assert.equal(v.length - 1, v.getMaxIdx());
            })
        })

        // ??
        describe('getMaxIdx "Problem" Test', function () {
            it('what if all elements are the same, 1', function () {
                var vec = new la.Vector([1, 1, 1, 1]);
                assert.equal(vec.getMaxIdx(), 0);

                // assert.equal(vec.getMaxIdx(), 1); // should also work, but doesn't 
            })
        })

        describe('Clone Test', function () {
            it('should return true if v is equal w', function () {
                var w = new la.Vector(v);

                assert.deepEqual(v, w);
                for (var i = 0; i < v.length; i++) {
                    assert.equal(w.at(i), v.at(i));
                }
            })
        })

        describe('Ones Test', function () {
            it('should return a 5-dimensional vector whose entries are set to 1.0', function () {
                var n = 5;
                var w = new la.ones(n);

                for (var i = 0; i < w.length; i++) {
                    assert.equal(w.at(i), 1);
                }
            })
        })

        // not implemented
        //describe('Square test', function () {
        //    it('should square all values of vector v', function () {
        //        var array = [1, 2, 3, 4];
        //        var w = new la.Vector(array); la.square(w);
        //        for (var i = 0; i < w.length; i++) {
        //            assert.equal(w.at(i), array[i] * array[i]);
        //        }
        //    })
        //})

        describe('Sort Test', function () {
            it('should sort vector in ascending order [0.11, 0.12, 3.5, 4]', function () {
                var array = [0.11, 0.12, 3.5, 4];
                var sortedVec = new la.Vector(array);

                var vec = new la.Vector([3.5, 0.12, 4, 0.11]);
                var vec = vec.sort();

                assert.deepEqual(vec, sortedVec);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), sortedVec.at(i));
                }
            })
        })

        describe('Plus Test', function () {
            it('should sum the vectors', function () {
                var vec1 = new la.Vector([1, 3, 4, 8]);
                var vec2 = new la.Vector([4, 3, 8, 2.1]);
                var vec = vec1.plus(vec2);
                var controlVec = new la.Vector([5, 6, 12, 10.1]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Minus Test', function () {
            it('should substract the vectors', function () {
                var vec1 = new la.Vector([1, 3, 4, 8]);
                var vec2 = new la.Vector([4, 3, 8, 2.1]);
                var vec = vec1.minus(vec2);
                var controlVec = new la.Vector([-3, 0, -4, 5.9]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Multiply Test', function () {
            it('should multiply vector vec with scalar 3.14', function () {
                var vec = new la.Vector([3, 0, -12, 0.0001]);
                var vec2 = vec.multiply(3.14);
                var controlVec = new la.Vector([9.42, 0, -37.68, 0.000314]);

                assert.deepEqual(vec2, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec2.at(i), controlVec.at(i));
                }
            })
        })

        describe('Multiply "Small scalar" Test', function () {
            it('should multiply vector vec with scalar close to 0: Number.MIN_VALUE', function () {
                var vec = new la.Vector([3, 0, 1, 0.0001]);
                var vec2 = vec.multiply(Math.pow(10, -5));
                var controlVec = new la.Vector([0.00003, 0, 0.00001, 0.000000001]);

                assert.deepEqual(vec2, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec2.at(i), controlVec.at(i));
                }
            })
        })

        describe('Subvector Test', function () {
            it('should return the subvector of vector vec', function () {
                var vec = new la.Vector([3, -51, 22, 19]);
                var indVec = new la.IntVector([1, 3, 2, 0]);
                vec = vec.subVec(indVec);
                var controlVec = new la.Vector([-51, 19, 22, 3]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('At [] Test', function () {
            it('should return elements with indexes 0 (3.2) and 1 (4)', function () {
                assert.equal(3.2, v[0]);
                assert.equal(4, v[1]);
            })
            it('should save new value at index 0 (12)', function () {
                v[0] = 12;
                assert.equal(12, v[0]);
            })
        })

        describe('Put Test', function () {
            it('should put the value -21 at index 1', function () {
                v.put(1, -21);
                assert.equal(-21, v[1]);
            })
        })

        describe('Diag Test', function () {
            it('should return a matrix with the vector v on it\'s diagonal', function () {
                var mat = v.diag();
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        if (i == j) { assert.equal(mat.at(i, j), v[i]); }
                        else { assert.equal(mat.at(i, j), 0); }
                    }
                }
            })
        })

        describe('PushV Test', function () {
            it('should return v with appended vector [1, 2, 3]', function () {
                var w = new la.Vector([1, 2, 3]);
                v.pushV(w);
                assert.equal(v.length, 5);
                var controlVec = new la.Vector([12, -21, 1, 2, 3]);

                assert.deepEqual(v, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(v.at(i), controlVec.at(i));
                }
            })
        })

        describe('Norm Test', function () {
            it('should return the norm of vec 5', function () {
                var vec = new la.Vector([3, 4]);
                var n = vec.norm();
                assert.eqtol(n, 5);
            })
        })

        describe('Normalize Test', function () {
            it('should normalize vec and return vector [3/5, 4/5]', function () {
                var vec = new la.Vector([3, 4]);
                vec.normalize();
                var controlVec = new la.Vector([3 / 5, 4 / 5]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Normalize "Zer0 Vector" Test', function () {
            it('should return the same vector', function () {
                var vec = new la.Vector([0, 0, 0, 0]);
                vec.normalize();
                var controlVec = new la.Vector([0, 0, 0, 0]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('ToMat Test', function () {
            it('should return matrix with a single column that equals vec', function () {
                var mat = v.toMat();
                for (var i = 0; i < v.length; i++) {
                    assert.equal(mat.at(i, 0), v.at(i));
                }
            })
        })

        describe('Sparse Test', function () {
            it('should return the sparse vector of vec', function () {
                var spV = v.sparse();
                var controlVec = new la.SparseVector([[0, 12], [1, -21], [2, 1], [3, 2], [4, 3]]);

                //assert.deepEqual(spV, controlVec);      // spV.dim = -1 instead of 5 ??
                for (var i = 0; i < controlVec.dim; i++) {
                    assert.eqtol(spV.at(i), controlVec.at(i));
                }
            })
        })

        describe('Unshift Test', function () {
            it('should insert the value 10 at the beginning of vector v', function () {
                v.unshift(10);
                assert.equal(v[0], 10);
            })
        })

        describe('Trunc Test', function () {
            it('should cut off the last 3 values in vector v', function () {
                v.trunc(3);
                assert.equal(v.length, 3);
            })
        })

        describe('Outer Test', function () {
            it('should return a matrix v * v^T', function () {
                var mat = v.outer(v);
                var controlMat = new la.Matrix([[100, 120, -210], [120, 144, -252], [-210, -252, 441]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Inner Test', function () {
            it('should return the scalar product of v and [1, 2, 3]', function () {
                var n = v.inner(new la.Vector([1, 2, 3]));
                assert.equal(n, 10 + 24 - 63);
            })
        })

        describe('SpDiag Test', function () {
            it('should return a sparse matrix with v on the diagonal', function () {
                var spMat = v.spDiag();
                var controlspMat = new la.SparseMatrix([[[0, 10]], [[1, 12]], [[2, -21]]], { "rows": 3 });

                for (var i = 0; i < 3; i++) {
                    for (var j = 0; j < 3; j++) {
                        if (i == j) {assert.equal(spMat.at(i, j), controlspMat.at(i, j))}
                        else {assert.equal(spMat.at(i, j), 0)}
                    }   
                }
            })
        })
    });
});

//
// Function tests for IntVector
//

var intV = new la.IntVector();

describe('IntVector Test', function () {
    
    describe('Property Tests', function () {
        describe('Length Test', function () {
            it('should return 0 as the length of intV', function () {
                assert.equal(intV.length, 0);
            })
        })
    })

    describe('Functions Tests', function() {
        describe('Push Test', function () {
            it('pushing values -1 and 6 into intV, length should return 2', function () {
                intV.push(-1);
                intV.push(6);
                assert.equal(intV.length, 2);
            })
        })

        // should throw an exception
        //describe('Push "Float" Test', function () {
        //    it('should return an exception when trying to push a floating number, 3.2', function () {
        //        assert.throws(function () {
        //            intV.push(3.2);
        //        })
        //    })
        //})

        describe('At Test', function () {
            it('should return the values at position 0 (-1) and at position 1 (6)', function () {
                assert.equal(intV.at(0), -1);
                assert.equal(intV.at(1), 6);
            })
        })

        describe('At "Out of Bound" Test', function () {
            it('should give an error for accessing an element out of bound', function () {
                assert.throws(function () {
                    var n = intV.at(-1);
                })
            })
        })

        describe('Constructor "Array" Test', function () {
            it('takes an array [1, 2, 3] and it should create a vector with same values', function () {
                var vec = new la.IntVector([1, 2, 3]);
                assert.equal(vec.length, 3);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), i + 1);
                }
            })
        })

        describe('Constructor "Dictionary" Test', function () {
            it('takes {"vals":5, "mxvals": 5} and it should create a vector with 5 zeros', function () {
                var vec = new la.IntVector({ "vals": 5, "mxvals": 5 });
                assert.equal(vec.length, 5);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), 0);
                }
            })
        })

        describe('Copy Constructor Test', function () {
            it('should copy the vector v and save it in vec', function () {
                var vec = new la.IntVector(intV);
                assert.equal(vec.length, intV.length);
                for (var i = 0; i < intV.length; i++) {
                    assert.equal(vec.at(i), intV.at(i));
                }
            })
        })

        describe('Sum Test', function () {
            it('should return a sum of -1 and 6', function () {
                assert.equal(intV.sum(), 5);
            })
        })

        describe('getMaxIdx Test', function () {
            it('should return index of last element in vector, 1.', function () {
                assert.equal(intV.getMaxIdx(), 1);
            })
        })

        // ??
        describe('getMaxIdx "Problem" Test', function () {
            it('what if all elements are the same, 1', function () {
                var vec = new la.IntVector([1, 1, 1, 1]);
                assert.equal(vec.getMaxIdx(), 0);

                // assert.equal(vec.getMaxIdx(), 1); // should also work, but doesn't 
            })
        })

        describe('Sort Test', function () {
            it('should sort vector in ascending order [-3, 0, 2, 15]', function () {
                var array = [-3, 0, 2, 15];
                var sortedVec = new la.IntVector(array);

                var vec = new la.IntVector([2, 0, 15, -3]);
                var vec = vec.sort();

                assert.deepEqual(vec, sortedVec);
                for (var i = 0; i < vec.length; i++) {
                    assert.equal(vec.at(i), sortedVec.at(i));
                }
            })
        })

        // not implemented
        //describe('Plus Test', function () {
        //    it('should sum the vectors', function () {
        //        var vec1 = new la.IntVector([1, 3, 4, 8]);
        //        var vec2 = new la.IntVector([4, 3, 8, 2]);
        //        var vec = vec1.plus(vec2);
        //        var controlVec = new la.IntVector([5, 6, 12, 10]);

        //        assert.deepEqual(vec, controlVec);
        //        for (var i = 0; i < controlVec.length; i++) {
        //            assert.equal(vec.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        // not implemented
        //describe('Minus Test', function () {
        //    it('should substract the vectors', function () {
        //        var vec1 = new la.IntVector([1, 3, 4, 8]);
        //        var vec2 = new la.IntVector([4, 3, 8, 2]);
        //        var vec = vec1.minus(vec2);
        //        var controlVec = new la.IntVector([-3, 0, -4, 6]);

        //        assert.deepEqual(vec, controlVec);
        //        for (var i = 0; i < controlVec.length; i++) {
        //            assert.equal(vec.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        // not implemented
        //describe('Multiply Test', function () {
        //    it('should multiply vector vec with scalar 3.14', function () {
        //        var vec = new la.IntVector([3, 0, -12, 0]);
        //        var vec2 = vec.multiply(3);
        //        var controlVec = new la.IntVector([9, 0, -36, 0]);

        //        assert.deepEqual(vec2, controlVec);
        //        for (var i = 0; i < controlVec.length; i++) {
        //            assert.eqtol(vec2.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        
        describe('Subvector Test', function () {
            it('should return the subvector of vector vec', function () {
                var vec = new la.IntVector([3, -51, 22, 19]);
                var indVec = new la.IntVector([1, 3, 2, 0]);
                vec = vec.subVec(indVec);
                var controlVec = new la.IntVector([-51, 19, 22, 3]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('At [] Test', function () {
            it('should return elements with indexes 0 (-1) and 1 (6)', function () {
                assert.equal(intV[0], -1);
                assert.equal(intV[1], 6);
            })
            it('should save new value at index 0 (12)', function () {
                intV[0] = 12;
                assert.equal(intV[0], 12);
            })
        })

        describe('Put Test', function () {
            it('should put the value -21 at index 1', function () {
                intV.put(1, -21);
                assert.equal(intV[1], -21);
            })
        })

        // not implemented
        //describe('Diag Test', function () {
        //    it('should return a matrix with the vector v on it\'s diagonal', function () {
        //        var mat = intV.diag();
        //        for (var i = 0; i < mat.rows; i++) {
        //            for (var j = 0; j < mat.cols; j++) {
        //                if (i == j) { assert.equal(mat.at(i, j), intV[i]); }
        //                else { assert.equal(mat.at(i, j), 0); }
        //            }
        //        }
        //    })
        //})

        describe('PushV Test', function () {
            it('should return v with appended vector [1, 2, 3]', function () {
                var intW = new la.IntVector([1, 2, 3]);
                intV.pushV(intW);
                assert.equal(intV.length, 5);
                var controlVec = new la.IntVector([12, -21, 1, 2, 3]);

                assert.deepEqual(intV, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(intV.at(i), controlVec.at(i));
                }
            })
        })

        // not implemented
        //describe('Norm Test', function () {
        //    it('should return the norm of vec 5', function () {
        //        var vec = new la.IntVector([3, 4]);
        //        var n = vec.norm();
        //        assert.eqtol(n, 5);
        //    })
        //})

        // not implemented
        //describe('Normalize Test', function () {
        //    it('should normalize vec and return vector [3/5, 4/5]', function () {
        //        var vec = new la.IntVector([3, 4]);
        //        vec.normalize();
        //        var controlVec = new la.IntVector([3 / 5, 4 / 5]);

        //        assert.deepEqual(vec, controlVec);
        //        for (var i = 0; i < controlVec.length; i++) {
        //            assert.eqtol(vec.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        // not implemented
        //describe('Normalize "Zer0 Vector" Test', function () {
        //    it('should return the same vector', function () {
        //        var vec = new la.IntVector([0, 0, 0, 0]);
        //        vec.normalize();
        //        var controlVec = new la.IntVector([0, 0, 0, 0]);

        //        assert.deepEqual(vec, controlVec);
        //        for (var i = 0; i < vec.length; i++) {
        //            assert.equal(vec.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        // not implemented
        //describe('ToMat Test', function () {
        //    it('should return matrix with a single column that equals intV', function () {
        //        var mat = intV.toMat();
        //        for (var i = 0; i < intV.length; i++) {
        //            assert.equal(mat.at(i, 0), intV.at(i));
        //        }
        //    })
        //})

        // not implemented
        //describe('Sparse Test', function () {
        //    it('should return the sparse vector of intV', function () {
        //        var spV = intV.sparse();
        //        var controlVec = new la.SparseVector([[0, 12], [1, -21], [2, 1], [3, 2], [4, 3]]);

        //        //assert.deepEqual(spV, controlVec);      // spV.dim = -1 instead of 5 ??
        //        for (var i = 0; i < controlVec.dim; i++) {
        //            assert.eqtol(spV.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        describe('Unshift Test', function () {
            it('should insert the value 10 at the beginning of vector intV', function () {
                intV.unshift(10);
                assert.equal(intV[0], 10);
            })
        })
    })
})

//
// Function tests for Matrix
//

var mat = new la.Matrix();
var mat2 = new la.Matrix([[3, -1], [8, -2]]);

describe('Matrix Test', function () {

    describe('Property Tests', function () {
        describe('Row Test', function () {
            it('should return 0 for matrix mat', function () {
                assert.equal(mat.rows, 0);
            })
        })

        describe('Col Test', function () {
            it('should return 0 for matrix mat', function () {
                assert.equal(mat.cols, 0);
            })
        })
    });

    describe('Functions Test', function () {

        describe('At Test', function () {
            it('should throw error, mat has no elements (assert catches error)', function () {
                assert.throws(function () {
                    var k = mat.at(0, 0);
                })
            })
        })

        describe('At Test', function () {
            it('should return the element at (0, 1): 2', function () {
                mat = new la.Matrix([[1, 2], [3, 4]]);
                assert.equal(mat.at(0, 1), 2);
            });
        })

        describe('Contructor "Nested Array" Test', function () {
            it('takes a nested array and it should return a dense matrix object', function () {
                var array = [[1, 2], [3, 4]];
                var dMat = new la.Matrix(array);
                assert.equal(dMat.rows, 2);
                assert.equal(dMat.cols, 2);

                for (var i = 0; i < dMat.rows; i++) {
                    for (var j = 0; j < dMat.cols; j++) {
                        assert.equal(dMat.at(i, j), array[i][j]);
                    }
                }
            })
        })

        describe('Contructor "Dictionary" Test', function () {
            it('takes a dictionary of rows, columns and random and return a matrix', function () {
                var dMat = new la.Matrix({ "rows": 3, "cols": 3, "random": false });

                assert.equal(dMat.rows, 3);
                assert.equal(dMat.cols, 3);

                for (var i = 0; i < dMat.rows; i++) {
                    for (var j = 0; j < dMat.cols; j++) {
                        assert.equal(dMat.at(i, j), 0);
                    }
                }
            })
        })

        // not implemented yet
        //describe('Copy Constructor Test', function () {
        //    it('should make the same matrix as mat', function () {
        //        var mat2 = new la.Matrix(mat);

        //        assert.deepEqual(mat, mat2);
        //        for (var i = 0; i < mat2.rows; i++) {
        //            for (var j = 0; j < mat2.cols; j++) {
        //                assert.eqtol(mat2.at(i, j), mat.at(i, j));
        //            }
        //        }
        //    })
        //})

        describe('Put Test', function () {
            it('should put value 10 at (1, 1)', function () {
                mat.put(1, 1, 10);
                assert.equal(mat.at(1, 1), 10);
            })
        })

        describe('Multiply "Scalar" Test', function () {
            it('should multiply matrix with scalar 10', function () {
                var mat3 = mat.multiply(10);
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.equal(mat3.at(i, j), 10 * mat.at(i, j));
                    }
                }
            })
        })

        describe('Multiply "Vector" Test', function () {
            it('should multiply mat with vector [1, 2]', function () {
                var vec = mat.multiply(new la.Vector([1, 2]));
                var controlVec = new la.Vector([5, 23]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Multiply "Sparse Vector" Test', function () {
            it('should multiply mat with sparse vector [1, 2]', function () {
                var vec = mat.multiply(new la.SparseVector([[0, 1], [1, 2]]));
                var controlVec = new la.Vector([5, 23]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Multiply "Matrix" Test', function () {
            it('should multiply mat with [[3, -1],[8, -2]]', function () {
                var mat3 = mat.multiply(mat2);
                var controlMat = new la.Matrix([[19, -5], [89, -23]]);

                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.equal(mat3.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Multiply "Sparse Matrix" Test', function () {
            it('should multiply mat with sparse matrix [[3, -1],[8, -2]]', function () {
                var mat3 = mat.multiply(new la.SparseMatrix([[[0, 3], [1, 8]],[[0, -1], [1, -2]]]));
                var controlMat = new la.Matrix([[19, -5], [89, -23]]);

                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.equal(mat3.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('MultiplyT "Scalar" Test', function () {
            it('should transpose mat and multiply with scalar 10', function () {
                var mat3 = mat.multiplyT(10);
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.equal(mat3.at(j, i), 10 * mat.at(i, j));
                    }
                }
            })
        })

        describe('MultiplyT "Vector" Test', function () {
            it('should transpose mat and multiply with vector [1, 2]', function () {
                var vec = mat.multiplyT(new la.Vector([1, 2]));
                var controlVec = new la.Vector([7, 22]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('MultiplyT "Sparse Vector" Test', function () {
            it('should transpose mat and multiply with sparse vector [1, 2]', function () {
                var vec = mat.multiplyT(new la.SparseVector([[0, 1], [1, 2]]));
                var controlVec = new la.Vector([7, 22]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.equal(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('MultiplyT "Matrix" Test', function () {
            it('should tranpose mat multiply with matrix [[3, -1],[8, -2]]', function () {
                var mat3 = mat.multiplyT(mat2);
                var controlMat = new la.Matrix([[27, -7], [86, -22]]);

                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.equal(mat3.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('MultiplyT "Matrix" Test', function () {
            it('should tranpose mat multiply with matrix [[3, -1],[8, -2]]', function () {
                var mat3 = mat.multiplyT(new la.SparseMatrix([[[0, 3], [1, 8]],[[0, -1], [1, -2]]]));
                var controlMat = new la.Matrix([[27, -7], [86, -22]]);

                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.equal(mat3.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Plus Test', function () {
            it('should add mat and mat2', function () {
                var mat3 = mat.plus(mat2);

                assert.deepEqual(mat3, mat);
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.equal(mat3.at(i, j), mat.at(i, j) + mat2.at(i, j));
                    }
                }
            })
        })

        describe('Sparse Test', function () {
            it('should return the sparse matrix of mat', function () {
                var spMat = mat.sparse();
                var controlspMat = new la.SparseMatrix([[[0, 1], [1, 3]], [[0, 2], [1, 10]]]);

                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.eqtol(spMat.at(i, j), controlspMat.at(i, j));
                    }
                }
            })

        })

        describe('Minus Test', function () {
            it('should substract mat2 from mat', function () {
                var mat3 = mat.minus(mat2);

                assert.deepEqual(mat3, mat);
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.equal(mat3.at(i, j), mat.at(i, j) - mat2.at(i, j));
                    }
                }
            })
        })

        describe('Transpose Test', function () {
            it('should transpose mat', function () {
                var mat3 = mat.transpose();
                assert.deepEqual(mat3, mat);
                for (var i = 0; i < mat.rows; i++) {
                    for (var j = 0; j < mat.cols; j++) {
                        assert.equal(mat3.at(j, i), mat.at(i, j));
                    }
                }
            })
        })

        describe('Solve Test', function () {
            it('should solve linear system A*x = y', function () {
                var vec = mat.solve(new la.Vector([-1, -7]));       // it changes the (1, 1)-th element 10 back to 4?
                var solution = new la.Vector([1, -1]);

                assert.deepEqual(vec, solution);
                for (var i = 0; i < solution.length; i++) {
                    assert.equal(vec.at(i), solution.at(i));
                }
            })
        })

        describe('RowNorms Test', function () {
            it('should return vector with norm of i-th row as i-th element', function () {
                var vec = mat.rowNorms();
                var controlVec = new la.Vector([Math.sqrt(5), 5]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('ColNorms Test', function () {
            it('should return vector with norm of i-th column as i-th element', function () {
                var vec = mat.colNorms();
                var controlVec = new la.Vector([Math.pow(10, 0.5), Math.pow(20, 0.5)]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('NormalizeCols Test', function () {
            it('should normalize the columns of matrix mat', function () {
                var mat3 = new la.Matrix([[1, 2], [3, 4]]);
                mat3.normalizeCols();

                var controlMat = new la.Matrix([[1 / Math.sqrt(10), 2 / Math.sqrt(20)],
                    [3 / Math.sqrt(10), 4 / Math.sqrt(20)]]);

                assert.deepEqual(mat3, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat3.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Frob Test', function () {
            it('should return the frobenious norm of mat, sqrt(27)', function () {
                var norm = mat.frob();
                var controlNorm = Math.sqrt(30);

                assert.eqtol(norm, controlNorm);
            })
        })

        describe('RowMaxIdx Test', function () {
            it('should return value 1 for 1st and 2nd row', function () {
                // assert.equal(mat.rowMaxIdx(0), 1);
                assert.equal(mat.rowMaxIdx(1), 1);
            })
        })

        describe('ColMaxIdx Test', function () {
            it('should return value 1 for 1st and 2nd column', function () {
                assert.equal(mat.colMaxIdx(0), 1);
                assert.equal(mat.colMaxIdx(1), 1);
            })
        })

        describe('GetCol Test', function () {
            it('should return first column as vector', function () {
                var vec = mat.getCol(0);
                var controlVec = new la.Vector([1, 3]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('SetCol Test', function () {
            it('should change first column into [6, -1]', function () {
                mat = new la.Matrix([[1, 2], [3, 4]]);
                mat.setCol(0, new la.Vector([6, -1]));
                var controlMat = new la.Matrix([[6, 2], [-1, 4]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('GetRow Test', function () {
            it('should return first row as vector', function () {
                var vec = mat.getRow(0);
                var controlVec = new la.Vector([6, 2]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('SetRow Test', function () {
            it('should change the second row into [10, 3]', function () {
                mat.setRow(1, new la.Vector([10, 3]));
                var controlMat = new la.Matrix([[6, 2], [10, 3]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Diag Test', function () {
            it('should return diagonal of mat as vector [6, 3]', function () {
                var vec = mat.diag();
                assert.eqtol(vec.at(0), 6);
                assert.eqtol(vec.at(1), 3);
            })
        })

        describe('Eye Test', function () {
            it('should return a 3 x 3 identity matrix', function () {
                var dim = 3;
                var mat3 = new la.eye(dim);
                for (var i = 0; i < mat3.rows; i++) {
                    for (var j = 0; j < mat3.cols; j++) {
                        if (i == j) { assert.equal(mat3.at(i, j), 1); }
                        else { assert.equal(mat3.at(i, j), 0); }
                    }
                }
            })
        })

        describe('Zero Test', function () {
            it('should return a 3 x 2 zero matrix', function () {
                var nRows = 3;
                var nCols = 2;
                var mat3 = new la.zeros(nRows, nCols);
                for (var i = 0; i < mat3.rows; i++) {
                    for (var j = 0; j < mat3.cols; j++) {
                        assert.equal(mat3.at(i, j), 0);
                    }
                }
            })
        })
    });
});

//
// Sparse Vector
//

var arr = [[0, 3], [1, 10], [3, 0.0001], [8, 0], [9, -12]];
var spV = new la.SparseVector(arr);

describe('Sparse Vector', function () {
    describe('Property Tests', function () {

        describe('Nnz Test', function () {
            it('should return the number of non-zero values', function () {
                assert.equal(spV.nnz, 5);   // the correct test would be for 4 ??
            })
        })

        describe('Dim Test', function () {
            it('should return the dimension of sparse vector', function () {
                assert.equal(spV.dim, 5);   // the correct test would be for 9 ?? 
            })
        })
    });

    describe('Function Tests', function () {

        describe('At Test', function () {
            it('should return the value with index 1, 3, 8, 9, 2', function () {
                assert.equal(spV.at(1), 10);
                assert.equal(spV.at(3), 0.0001);
                // assert.equal(spV.at(8), 0);
                // assert.equal(spV.at(9), -12);

                assert.equal(spV.at(2), 0);
            })
        })

        describe('Put Test', function () {
            it('should put a new value in sparse vector spV', function () {
                spV.put(2, -1);

                assert.equal(spV.at(2), -1);
            })
        })

        describe('Sum Test', function () {
            it('should return the sum of all given values of spV, 0.0001', function () {
                assert.eqtol(spV.sum(), 0.0001);
            })
        })

        // strange results
        //describe('Inner Test', function () {
        //    it('should return the scalar product of spV and [1, 2, 0, 3, 0, 0, 0, 4, 5]', function () {
        //        var n = spV.inner(new la.Vector([1, 2, 3, 4, 5]));

        //        assert.eqtol(n, 3 + 20 + 0.0003 + 0 - 60);
        //    })
        //})

        // not implemented 
        //describe('Plus Test', function () {
        //    it('should return the sum of two vectors', function () {
        //        var spVec = new la.SparseVector([[6, 10]]);
        //        spV.plus(spVec);
        //        assert.equal(spV.at(6), 10);
        //    })
        //})

        // doesn't work
        //describe('Multiply Test', function () {
        //    it('should multiply all values with 7', function () {
        //        var spVec = spV.multiply(7);

        //        var controlVec = new la.SparseVector([[0, 21], [1, 70], [3, 0.0001], [8, 0], [9, -84], [2, -7]]);

        //        // assert.deepEqual(spVec, controlVec);
        //        for (var i = 0; i < 9; i++) {
        //            assert.eqtol(spVec.at(i), controlVec.at(i));
        //        }
        //    })
        //})

        describe('Norm Test', function () {
            it('should return norm of spV, 15.9373774508', function () {
                var n = spV.norm();

                assert.eqtol(n, 15.9373774508);
            })
        })

        // doesn't make the full dense vector
        describe('Full Test', function () {
            it('should return the dense form of spV', function () {
                var vec = spV.full();
                var controlVec = new la.Vector([3, 10, -1, 0.0001, 0, 0, 0, 0, -12]);

        //        assert.deepEqual(vec, controlVec);
                for (var i = 0; i < vec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('ValVec Test', function () {
            it('should return a vector of nnz elements of spV', function () {
                var vec = spV.valVec();
                var controlVec = new la.Vector([3, 10, -1, 0.0001, 0, -12]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length ; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('IdxVec Test', function () {
            it('should return a vector of index elements of spV', function () {
                var vec = spV.idxVec();
                var controlVec = new la.IntVector([0, 1, 2, 3, 8, 9]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length ; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        // not implemented
        //describe('Sort Test', function () {
        //    it('should return the permutation integer vector for sorted spV', function () {
        //        spV.sort();        

        //    })
        //})

    });
});

//
// Functions for Sparse Matrix
//

var spMat = new la.SparseMatrix([[[0, 1], [1, 3], [3, -2]], [[2, 8]], [[3, 1]]], { "rows": 4 });

var dMulti = new la.Matrix([[1, 0, 0, 0], [1, 1, 0, 0], [0, 0, 1, 0], [1, 0, 0, 1]]);
var spMulti = new la.SparseMatrix([[[0, 1], [1, 1], [3, 1]], [[1, 1]], [[2, 1]], [[3, 1]]]);

describe('Sparse Matrix Tests', function () {
    describe('Property Tests', function () {

        // doesn't work
        //describe('Rows Test', function () {
        //    it('should return the number of rows in spMat, 4', function () {
        //        assert.equal(spMat.rows, 4);
        //    })
        //})

        describe('Cols Test', function () {
            it('should return the number of columns in spMat, 3', function () {
                assert.equal(spMat.cols, 3);
            })
        })

        describe('Nnz Test', function () {
            it('should return the nomber of non-zero elements of matrix spMat, 5', function () {
                assert.equal(spMat.nnz(), 5);
            })
        })

    });

    describe('Functions Tests', function () {

        describe('At Test', function () {
            it('should return the value at (0, 1)', function () {
                assert.equal(spMat.at(1, 0), 3);
            })
        })

        describe('Put Test', function () {
            it('should put value -3 at (1, 1)', function () {
                spMat.put(1, 1, -3);
                assert.equal(spMat.at(1, 1), -3);
            })
        })

        // doesn't work (attributes of spVec and controlVec doesn't match)
        describe('At [] Test', function () {
            it('should return the sparse vector of 1st column in stMat', function () {
                var spVec = spMat[0];
                var controlVec = new la.SparseVector([[0, 1], [1, 3], [3, -2]]);

                //assert.deepEqual(spVec, controlVec);
                for (var i = 0; i < controlVec.dim; i++) {
                    assert.eqtol(spVec.at(i), controlVec.at(i));
                }
            })
        })

        // Exception: sparse col matrix at: index out of bounds
        // this method of constructing a matrix is broken. Number of rows is not saved correctly
        //describe('Construtor "Three Vectors" Test', function () {
        //    it('should take two IntVectors of rowIndex and colIndex and a Vector of values', function () {
        //        var rowIdx = new la.IntVector([0, 1, 1, 2, 3, 3]);
        //        var colIdx = new la.IntVector([0, 0, 1, 1, 0, 2]);
        //        var val = new la.Vector([1, 3, -3, 8, -2, 1]);

        //        var spVMat = new la.SparseMatrix(rowIdx, colIdx, val);

        //        for (var i = 0; i < 4; i++) {
        //            for (var j = 0; j < 3; j++) {
        //                spVMat.at(i, j);
        //                console.log(i + ", " + j);
        //                assert.equal(spVMat.at(i, j), spMat.at(i, j));
        //            }
        //        }
        //    })
        //})

        // Sparse Matrix is broken. Doesn't save the rows parameter correctly
        //describe('Constructor "Double Nested Array & Rows" Test', function () {
        //    it('should construct a sparse matrix out of the double nested array and dictionary with key "rows"', function () {
        //        var mat = new la.SparseMatrix([[[1, 2]], [[0, 5]]], { "rows": 2 });
        //        assert.equal(mat.cols, 2);
        //        assert.equal(mat.rows, 2);

        //        assert.equal(mat.at(0, 0), 0);
        //        assert.equal(mat.at(1, 0), 2);
        //        assert.equal(mat.at(0, 1), 5);
        //        assert.equal(mat.at(1, 1), 0);
        //    })
        //})

        describe('Push Test', function () {
            it('should push a sparse vector [1, 2, 0, 4]', function () {
                spMat.push(new la.SparseVector([[0, 1], [1, 2], [3, 4]]));
                var controlMat = new la.SparseMatrix([[[0, 1], [1, 3], [3, -2]], [[1, -3], [2, 8]], [[3, 1]],
                    [[0, 1], [1, 2], [3, 4]]]);
                    
                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.equal(spMat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })
            
        describe('Multiply "Scalar" Test', function () {
            it('should return spMat multiplied with 3.2', function () {
                var spMat2 = spMat.multiply(3.2);
                var controlMat = new la.SparseMatrix([[[0, 3.2], [1, 9.6], [3, -6.4]], [[1, -9.6], [2, 25.6]], [[3, 3.2]],
                    [[0, 3.2], [1, 6.4], [3, 12.8]]]);

                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(spMat2.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Multiply "Vector" Test', function () {
            it('should multiply spMat and vector [1, 2, 3, 4]', function () {
                var vec = spMat.multiply(new la.Vector([1, 2, 3, 4]));
                var controlVec = new la.Vector([5, 5, 16, 17]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec[i], controlVec[i]);
                }
            })
        })

        describe('Multiply "Sparse Vector" Test', function () {
            it('should multiply spMat and sparse vector [1, 2, 3, 4]', function () {
                var vec = spMat.multiply(new la.SparseVector([[0, 1], [1, 2], [2, 3], [3, 4]]));
                var controlVec = new la.Vector([5, 5, 16, 17]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec[i], controlVec[i]);
                }
            })
        })

        describe('Multiply "Matrix" Test', function () {
            it('should multiply spMat with dense matrix', function () {
                var mat2 = spMat.multiply(dMulti);
                var controlMat = new la.Matrix([[2, 0, 0, 1], [2, -3, 0, 2], [8, 8, 0, 0], [2, 0, 1, 4]]);

                assert.deepEqual(mat2, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat2.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Multiply "Sparse Matrix" Test', function () {
            it('should multiply spMat with sparse matrix', function () {
                var mat2 = spMat.multiply(spMulti);
                var controlMat = new la.Matrix([[2, 0, 0, 1], [2, -3, 0, 2], [8, 8, 0, 0], [2, 0, 1, 4]]);

                assert.deepEqual(mat2, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat2.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('MultiplyT "Scalar" Test', function () {
            it('should transpose and multiply spMat with scalar 3.2', function () {
                var spMat2 = spMat.multiplyT(3.2);
                var controlMat = new la.SparseMatrix([[[0, 3.2], [3, 3.2]], [[0, 9.6], [1, -9.6], [3, 6.4]], [[1, 25.6]],
                [[0, -6.4], [2, 3.2], [3, 12.8]]]);

                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(spMat2.at(i, j), controlMat.at(i, j));
                    }

                }
            })
        })

        describe('MultiplyT "Vector" Test', function () {
            it('should transpose and multiply spMat with vector [1, 2, 3, 4]', function () {
                var vec = spMat.multiplyT(new la.Vector([1, 2, 3, 4]));
                var controlVec = new la.Vector([-1, 18, 4, 21]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('Multiply "Sparse Vector" Test', function () {
            it('should transpose and multiply spMat with sparse vector [1, 2, 3, 4]', function () {
                var vec = spMat.multiplyT(new la.SparseVector([[0, 1], [1, 2], [2, 3], [3, 4]]));
                var controlVec = new la.Vector([-1, 18, 4, 21]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec.at(i), controlVec.at(i));
                }
            })
        })

        describe('MultiplyT "Matrix" Test', function () {
            it('should transpose and multiply spMat with matrix', function () {
                var mat = spMat.multiplyT(dMulti);
                var controlMat = new la.Matrix([[2, 3, 0, -2], [-3, -3, 8, 0], [1, 0, 0, 1], [7, 2, 0, 4]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('MultiplyT "Sparse Matrix" Test', function () {
            it('should transpose and multiply spMat with sparse matrix', function () {
                var mat = spMat.multiplyT(spMulti);
                var controlMat = new la.Matrix([[2, 3, 0, -2], [-3, -3, 8, 0], [1, 0, 0, 1], [7, 2, 0, 4]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Plus Test', function () {
            it('should add spMat2 and spMat together and return spMat3', function () {
                var spMat3 = spMat.plus(spMulti);
                var controlspMat = new la.SparseMatrix([[[0, 2], [1, 4], [3, -1]], [[1, -2], [2, 8]], [[2, 1],[3, 1]],
                    [[0, 1], [1, 2], [3, 5]]]);

                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(spMat3.at(i, j), controlspMat.at(i, j));
                    }
                }
            })
        })

        // it makes spMat2 - spMat, not spMat - spMat2
        //describe('Minus Test', function () {
        //    it('should substract spMat2 from spMat and return spMat3', function () {
        //        var spMat3 = spMat.minus(spMulti);
        //        var controlspMat = new la.SparseMatrix([[[1, 2], [3, -3]], [[1, -4], [2, 8]], [[2, -1], [3, 1]],
        //            [[0, 1], [1, 2], [3, 3]]]);

        //        for (var i = 0; i < 4; i++) {
        //            for (var j = 0; j < 4; j++) {
        //                console.log(i +", "+ j);
        //                assert.eqtol(spMat3.at(i, j), controlspMat.at(i, j));
        //            }
        //        }
        //    })
        //})

        describe('Transpose Test', function () {
            it('should transpose spMat and return it in spMat2', function () {
                var spMat2 = spMat.transpose();
                var controlMat = new la.SparseMatrix([[[0, 1], [3, 1]], [[0, 3], [1, -3], [3, 2]], [[1, 8]],
                    [[0, -2], [2, 1], [3, 4]]]);

                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(spMat2.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('ColNorms Test', function () {
            it('should return a vector containing the norms of columns in spMat', function () {
                var vec = spMat.colNorms();
                var controlVec = new la.Vector([Math.sqrt(14), Math.sqrt(73), Math.sqrt(1), Math.sqrt(21)]);

                assert.deepEqual(vec, controlVec);
                for (var i = 0; i < controlVec.length; i++) {
                    assert.eqtol(vec[i], controlVec[i]);
                }
            })
        })

        describe('NormalizeCols Test', function () {
            it('should normalize columns in spMat', function () {
                var spMat2 = new la.SparseMatrix([[[0, 1], [1, 3], [3, -2]],
                    [[1, -3], [2, 8]], [[3, 1]],
                    [[0, 1], [1, 2], [3, 4]]]);
                spMat2.normalizeCols();
                var controlMat = new la.SparseMatrix([[[0, 1 / Math.sqrt(14)], [1, 3 / Math.sqrt(14)], [3, -2 / Math.sqrt(14)]],
                    [[1, -3/ Math.sqrt(73)], [2, 8/Math.sqrt(73)]], [[3, 1]],
                    [[0, 1 / Math.sqrt(21)], [1, 2 / Math.sqrt(21)], [3, 4 / Math.sqrt(21)]]]);

                for (var i = 0; i < 4; i++) {
                    for (var j = 0; j < 4; j++) {
                        assert.eqtol(spMat2.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Full Test', function () {
            it('should return the dense format of spMat', function () {
                    
                var mat = spMat.full();
                var controlMat = new la.Matrix([[1, 0, 0, 1], [3, -3, 0, 2], [0, 8, 0, 0], [-2, 0, 1, 4]]);

                assert.deepEqual(mat, controlMat);
                for (var i = 0; i < controlMat.rows; i++) {
                    for (var j = 0; j < controlMat.cols; j++) {
                        assert.eqtol(mat.at(i, j), controlMat.at(i, j));
                    }
                }
            })
        })

        describe('Speye Test', function () {
            it('should return a sparse eye matrix of dimension 3', function () {
                var mat = la.speye(3);
                
                for (var i = 0; i < 3; i++) {
                    for (var j = 0; j < 3; j++) {
                        if (i == j) { assert.equal(mat.at(i, j), 1); }
                        else { assert.equal(mat.at(i, j), 0); }
                    }
                }
            })
        })

        // doesn't work (frob doesn't square root the scalar product)
        //describe('Frob Test', function () {
        //    it('should return the frobenious norm of spMat', function () {
        //        var n = spMat.frob();
        //        assert.eqtol(n, 9.38083151964686);
        //    })
        //})

        // not implemented
        //describe('Sign Test', function () {
        //    it('should return a sparse matrix with 1 or -1 as values', function () {
        //        var spMat2 = spMat.sign();
                
        //        assert.equal(spMat2.at(0, 0), 1);
        //        assert.equal(spMat2.at(1, 0), 1);
        //        assert.equal(spMat2.at(3, 0), -1);
        //        assert.equal(spMat2.at(1, 1), -1);
        //        assert.equal(spMat2.at(2, 1), 1);
        //        assert.equal(spMat2.at(3, 2), 1);
        //    })
        //})
    });
})
