;; Copyright 2017-2024
;; Kaz Kylheku <kaz@kylheku.com>
;; Vancouver, Canada
;; All rights reserved.
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions are met:
;;
;; 1. Redistributions of source code must retain the above copyright notice,
;;    this list of conditions and the following disclaimer.
;;
;; 2. Redistributions in binary form must reproduce the above copyright notice,
;;    this list of conditions and the following disclaimer in the documentation
;;    and/or other materials provided with the distribution.
;;
;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
;; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
;; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
;; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
;; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
;; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
;; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;; POSSIBILITY OF SUCH DAMAGE.

(defmacro sys:dlib-expr (spec)
  (typecase spec
    (null ^(dlopen))
    (str ^(dlopen ,spec rtld-now))
    (t spec)))

(defmacro with-dyn-lib (lib . body)
  ^(let ((sys:ffi-lib (sys:dlib-expr ,lib)))
     ,*body))

(defun sys:with-dyn-lib-check (f e ref)
  (unless (lexical-var-p e 'sys:ffi-lib)
    (compile-warning f "simple ref ~s requires ~s"
                     ref 'with-dyn-lib)))

(defun sys:expand-sym-ref (f e exp)
  (cond
    ((stringp exp)
       (sys:with-dyn-lib-check f e exp)
       ^(dlsym-checked sys:ffi-lib ,exp))
    ((and (consp exp) (stringp (car exp)))
       (mac-param-bind f (sym ver) exp
         (sys:with-dyn-lib-check f e exp)
         ^(dlvsym-checked sys:ffi-lib ,sym ,ver)))
    (t exp)))

(defun sys:analyze-argtypes (form argtypes)
  (tree-bind (: ftypes vtypes) (split* argtypes (op where (op eq :)))
    (when vtypes
      (when (null ftypes)
        (compile-error form "variadic with zero fixed arguments not allowed"))
      (set vtypes
           (collect-each ((vt vtypes))
             (caseq vt
               ((float) 'double)
               ((be-float le-float)
                (compile-error form "variadic argument cannot be of type ~s"
                               vt))
               (t vt)))))
    (list* (+ (len ftypes) (len vtypes)) (len ftypes) (append ftypes vtypes))))


(defmacro deffi (:form f :env e name fun-expr rettype argtypes)
  (let ((fun-ref (sys:expand-sym-ref f e fun-expr))
        (fun-sym (gensym "fun-"))
        (desc-sym (gensym "desc-")))
    (tree-bind (nargs nfixed . argtypes) (sys:analyze-argtypes f argtypes)
      (let ((arg-syms (take nargs (gun (gensym)))))
        ^(let ((,fun-sym ,fun-ref)
               (,desc-sym (ffi-make-call-desc ,nargs ,nfixed
                                              (ffi-type-compile ',rettype)
                                              [mapcar ffi-type-compile
                                                      ',argtypes]
                                              ',name)))
           (defun ,name ,arg-syms
             (ffi-call ,fun-sym ,desc-sym ,*arg-syms)))))))

(defmacro deffi-type (name type-expr)
  ^(ffi-typedef ',name (ffi-type-compile ',type-expr)))

(defmacro typedef (name type-expr)
  ^(ffi-typedef ',name (ffi-type-compile ',type-expr)))

(defun sys:deffi-cb-expander (f name rettype argtypes safe-p abort-retval)
  (let ((fun-sym (gensym "fun-"))
        (desc-sym (gensym "desc-")))
    (tree-bind (nargs nvariadic . argtypes) (sys:analyze-argtypes f argtypes)
      ^(let ((,desc-sym (ffi-make-call-desc ,nargs ,nvariadic
                                            (ffi-type-compile ',rettype)
                                            [mapcar ffi-type-compile
                                                    ',argtypes]
                                            ',name)))
         (defun ,name (,fun-sym)
           [ffi-make-closure ,fun-sym ,desc-sym
                             ,safe-p ,abort-retval])))))

(defmacro deffi-cb (:form f name rettype argtypes : abort-retval)
  (sys:deffi-cb-expander f name rettype argtypes t abort-retval))

(defmacro deffi-cb-unsafe (:form f name rettype argtypes)
  (sys:deffi-cb-expander f name rettype argtypes nil nil))

(defmacro deffi-sym (:form f :env e name var-expr : type-sym)
  (let ((var-ref (sys:expand-sym-ref f e var-expr)))
    ^(defparml ,name ,(if type-sym
                        ^(cptr-cast ',type-sym ,var-ref)
                        var-ref))))

(defmacro deffi-var (:form f :env e name var-expr type)
  (let ((var-ref (sys:expand-sym-ref f e var-expr))
        (type-sym (gensym "type-"))
        (var-sym (gensym "var-")))
    ^(progn
       (defvarl ,type-sym (ffi ,type))
       (defvarl ,var-sym (carray-cptr ,var-ref ,type-sym 1))
       (defsymacro ,name (carray-ref ,var-sym 0)))))

(defmacro deffi-struct (name . body)
  ^(typedef ,name (struct ,name ,*body)))

(defmacro deffi-union (name . body)
  ^(typedef ,name (union ,name ,*body)))

(defmacro sizeof (type : (obj nil obj-p) :env menv)
  (if obj-p
    (if (constantp obj menv)
      (sys:dyn-size (ffi-type-compile type) obj)
      ^(sys:dyn-size (load-time (ffi-type-compile ',type)) ,obj))
    (ffi-size (ffi-type-compile type))))

(defmacro alignof (type)
  (ffi-alignof (ffi-type-compile type)))

(defmacro offsetof (struct memb)
  (ffi-offsetof (ffi-type-compile struct) memb))

(defmacro arraysize (arr)
  (ffi-arraysize (ffi-type-compile arr)))

(defmacro elemtype (type)
  ^(ffi-elemtype (ffi-type-compile ',type)))

(defmacro elemsize (type)
  (ffi-elemsize (ffi-type-compile type)))

(defmacro ffi (type)
  ^(load-time (ffi-type-compile ',type)))

(define-accessor carray-ref carray-refset)

(defset carray-sub (carray : (from 0) (to t)) items
  ^(progn (carray-replace ,carray ,items ,from ,to) ,items))

(defset sub-buf (buf : (from 0) (to t)) items
  ^(progn (replace-buf ,buf ,items ,from ,to) ,items))

(defmacro znew (type . pairs)
  (if (oddp (length pairs))
    (throwf 'eval-error "~s: slot initform arguments must occur pairwise"
            'znew))
  (let ((qpairs (mappend (aret ^(',@1 ,@2)) (tuples 2 pairs))))
    ^(make-zstruct (ffi ,type) ,*qpairs)))

(defmacro setjmp (:form f jmp-buf longjmp-var try-expr . longjmp-exprs)
  (unless (bindable longjmp-var)
    (compile-error f "~s is not a bindable symbol" longjmp-var))
  ^(sys:rt-setjmp ,jmp-buf
                  (lambda () ,try-expr)
                  (lambda (,longjmp-var) ,*longjmp-exprs)))
