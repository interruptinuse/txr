(let ((nullsym nil)
      (sym 's)
      (atom "abc")
      (cons '(x y z))
      (dwim '[]))
  (tree-bind (x y (op arg)) ^(a b @,nullsym)
    (assert (eq op 'sys:var))
    (assert (eq arg nullsym)))
  (tree-bind (x y (op arg)) ^(a b @,sym)
    (assert (eq op 'sys:var))
    (assert (eq arg sym)))
  (tree-bind (x y . (op arg)) ^(a b . @,sym)
    (assert (eq op 'sys:var))
    (assert (eq arg sym)))
  (tree-bind (x y (op arg)) ^(a b @,atom)
    (assert (eq op 'sys:var))
    (assert (eq arg atom)))
  (tree-bind (x y . (op arg)) ^(a b . @,atom)
    (assert (eq op 'sys:var))
    (assert (eq arg atom)))
  (tree-bind (x y (op arg)) ^(a b @,cons)
    (assert (eq op 'sys:expr))
    (assert (eq arg cons)))
  (tree-bind (x y . (op arg)) ^(a b . @,cons)
    (assert (eq op 'sys:expr))
    (assert (eq arg cons)))
  (tree-bind (x y (op arg)) ^(a b @,dwim)
    (assert (eq op 'sys:expr))
    (assert (eq arg dwim)))
  (tree-bind (x y . (op arg)) ^(a b . @,dwim)
    (assert (eq op 'sys:expr))
    (assert (eq arg dwim)))
  (tree-bind (x y (op arg . tail)) ^(a b (sys:expr ,sym . foo))
    (assert (eq op 'sys:expr))
    (assert (eq arg sym))
    (assert (eq tail 'foo)))
  (tree-bind (x y (op arg0 arg1)) ^(a b (sys:expr ,sym foo))
    (assert (eq op 'sys:expr))
    (assert (eq arg0 sym))
    (assert (eq arg1 'foo)))
  (tree-bind (x y (op)) ^(a b (sys:expr))
    (assert (eq op 'sys:expr))))
