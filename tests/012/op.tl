(load "../common")

(defun fi (fun)
  (assert (zerop (fun-optparam-count fun)))
  (list (fun-fixparam-count fun)
        (fun-variadic fun)))

(mtest
  (fi (op)) :error
  (fi (op list)) (0 t)
  (fi (op list @1)) (1 t)
  (fi (op list @2)) (2 t)
  (fi (op list @42)) (42 t)
  (fi (op list @rest)) (0 t)
  (fi (op list @1 @rest)) (1 t)
  (fi (op list @2 @rest)) (2 t)
  (fi (op list @42 @rest)) (42 t)
  (fi (op list @1 @3 @rest @2)) (3 t))

(mtest
  (fi (do)) :error
  (fi (do progn)) (1 t)
  (fi (do progn @1)) (1 t)
  (fi (do progn @2)) (2 t)
  (fi (do progn @42)) (42 t)
  (fi (do progn @rest)) (0 t)
  (fi (do progn @1 @rest)) (1 t)
  (fi (do progn @2 @rest)) (2 t)
  (fi (do progn @42 @rest)) (42 t)
  (fi (do progn @1 @3 @rest @2)) (3 t))

(mtest
  (fi (do if)) (1 t)
  (fi (do if @1)) (1 t)
  (fi (do if @2)) (2 t)
  (fi (do if @42)) (42 t)
  (fi (do if @rest)) (0 t)
  (fi (do if @1 @rest)) (1 t)
  (fi (do if @2 @rest)) (2 t)
  (fi (do if @42 @rest)) (42 t)
  (fi (do if @1 @3 @rest @2)) (3 t))

(mtest
  [(do quote x) 3] :error
  [(do quote @1) 3] :error
  (do pop a) :error)

(defun y (f)
  [(op @1 @1)
   (op f (op [@@1 @@1]))])

(defun fac (f)
  (do if (zerop @1)
         1
         (* @1 [f (- @1 1)])))

(defun fac-y (n)
  [(y (do do if (zerop @1)
               1
               (* @1 [@@1 (- @1 1)]))) n])

(defun fac-r (n)
  [(do if (zerop @1) 1 (* @1 [@rec (pred @1)])) n])

(defun fac-r2 (n)
  [(do if (zerop @1) 1 (* @1 @(rec (pred @1)))) n])

(mtest
  [[y fac] 4] 24
  (fac-y 4) 24
  (fac-r 4) 24
  (fac-r2 4) 24)

(mtest
  (flow 1 (+ 2) [dup *] (let ((x @1)) x)) 9
  (flow #S(time year 2021) .year succ) 2022)
