(load "../common")

(vtest
  (macro-time (defstruct foo nil
                (a 42)))
  (macro-time (find-struct-type 'foo)))

(let ((x 100))
  (defstruct bar foo
    (a (inc x))
    (b (inc x))))

(test ^#S(bar b ,(+ 2 2))
      #S(bar a 103 b 4))

(test (expand '^#S(bar b ,(+ 2 2)))
      (sys:make-struct-lit 'bar (list 'b (+ 2 2))))

(defvarl s (eval ^#S(bar b ,(+ 2 2))))

(test (set (slot s 'a) 100)
      100)

(test s
      #S(bar a 100 b 4))

(test (ignwarn (expand 'a.b.c.d))
      (slot (slot (slot a 'b)
                  'c) 'd))

(defmacro get-current-menv (:env e) e)
(defvarl menv (let (s a b c d) (macro-time (get-current-menv))))

(test (expand 's.a menv)
      (slot s 'a))
(test (expand 's.[a] menv)
      [(slot s 'a) s])
(test (expand 's.[a b c] menv)
      [(slot s 'a) s b c])

(set *gensym-counter* 0)

(stest (ignwarn (expand 's.(a) menv))
       "(call (slot s 'a)\n                     \
       \      s)")
(set *gensym-counter* 0)
(stest (ignwarn (expand 's.(a b c) menv))
       "(call (slot s 'a)\n                     \
       \      s b c)")
(test (expand 's.[a].b menv)
      (slot [(slot s 'a) s] 'b))
(test (expand 's.[a b c].b menv)
      (slot [(slot s 'a) s b c] 'b))
(set *gensym-counter* 0)
(stest (ignwarn (expand 's.(a).d menv))
       "(slot (call (slot s 'a)\n               \
       \            s)\n                        \
       \      'd)")
(set *gensym-counter* 0)
(stest (ignwarn (expand 's.(a b c).d menv))
       "(slot (call (slot s 'a)\n               \
       \            s b c)\n                    \
       \      'd)")

(test s.a 100)

(test (new foo) #S(foo a 42))

(set *gensym-counter* 4)
(stest (expand '(defstruct (boa x y) nil
                      (x 0) (y 0)))
       "(sys:make-struct-type 'boa '() '()\n                                            \
       \                      '(x y) () (lambda (#:g0008)\n                             \
       \                                  (let ((#:g0009 (struct-type #:g0008)))\n      \
       \                                    (if (static-slot-p #:g0009 'x)\n            \
       \                                      () (slotset #:g0008 'x\n                  \
       \                                                  0))\n                         \
       \                                    (if (static-slot-p #:g0009 'y)\n            \
       \                                      () (slotset #:g0008 'y\n                  \
       \                                                  0))))\n                       \
       \                      (lambda (#:g0008 #:g0010\n                                \
       \                               #:g0011)\n                                       \
       \                        (slotset #:g0008 'x\n                                   \
       \                                 #:g0010)\n                                     \
       \                        (slotset #:g0008 'y\n                                   \
       \                                 #:g0011))\n                                    \
       \                      ())")

(defstruct (boa x y) nil
  (x 0) (y 0))

(test (new boa)
      #S(boa x 0 y 0))
(test (new (boa 1 2))
      #S(boa x 1 y 2))
(test (new (boa 1 2) x 10 y (+ 10 10))
      #S(boa x 1 y 2))
(test (new boa x 10 y (+ 10 10))
      #S(boa x 10 y 20))

(defstruct baz nil
  (array (vec 1 2 3))
  (increment (lambda (self which delta)
               (inc [self.array which] delta))))

(defvarl bz (new baz))

(stest bz
       "#S(baz array #(1 2 3) increment #<interpreted fun: lambda (self which delta)>)")

(test [bz.array 2] 3)
(test bz.(increment 0 42) 43)
(test bz.array #(43 2 3))
(test [(meth bz increment) 1 5] 7)
(test bz.array #(43 7 3))

(defstruct (counter key) nil
  key
  (count 0)
  (get-count (lambda (self) self.count))
  (increment (lambda (self key)
               (if (eq self.key key)
                 (inc self.count)))))

(defun map-tree (tree func)
  (if (atom tree)
    [func tree]
    (progn (map-tree (car tree) func)
           (map-tree (cdr tree) func))))

(let ((c (new (counter 'a)))
      (tr '(a (b (a a)) c a d)))
  (map-tree tr (meth c increment))
  (test c.(get-count) 4))

(test (equal #S(bar) #S(bar)) nil)
(test (equal #S(foo) #S(foo)) t)
(test (equal #S(foo a 0) #S(foo a 1)) nil)
(test (equal #S(bar a 3 b 3) #S(bar a 3 b 3)) t)

(defstruct eqsub ()
  key
  (:method equal (me) me.key))

(test (equal (new eqsub key '(1 2))
             (new eqsub key '(1 2)))
      t)
