;; Copyright 2016-2024
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

(defun sys:name-str (kind sym-or-string)
  (cond
    ((symbolp sym-or-string) (symbol-name sym-or-string))
    ((stringp sym-or-string) sym-or-string)
    (t (throw 'eval-error "~s: ~s isn't a valid ~a name"
              'defpackage sym-or-string kind))))

(defmacro defpackage (name . clauses)
  (let* ((pkg (gensym "pkg-"))
         (nstr (sys:name-str 'package name))
         (exp-clauses (append-each ((c clauses))
                        (tree-case c
                          ((keyword package . rest)
                            (caseql keyword
                              (:use-from
                                ^((let ((p (find-package ',package)))
                                    (unless p
                                      (throwf 'eval-error
                                              "~s: no such package: ~s"
                                              'defpackage ',package))
                                    (each ((n ',(mapcar (op sys:name-str 'symbol)
                                                        rest)))
                                      (let ((s (intern n p)))
                                        (unless (eq (symbol-package s) p)
                                          (throwf 'eval-error
                                                  "~s: won't use non-local ~s from ~s"
                                                  'defpackage s p))
                                        (use-sym s ,pkg))))))
                              (t :)))
                          ((keyword . rest)
                            (caseql keyword
                              (:use
                                (if rest ^((use-package ',rest ,pkg))))
                              (:use-syms
                                ^((each ((s ',rest))
                                    (use-sym s ,pkg))))
                              (:use-syms-as
                                ^((doloop ((r ',rest (cddr r)))
                                          (r)
                                    (use-sym-as (car r) (cadr r) ,pkg))))
                              (:local
                                ^((each ((n ',(mapcar (op sys:name-str 'symbol)
                                                      rest)))
                                    (let ((s (intern n ,pkg)))
                                      (unless (eq (symbol-package s) ,pkg)
                                        (unuse-sym s ,pkg)
                                        (intern n ,pkg))))))
                              (:fallback
                                (if rest ^((set-package-fallback-list ,pkg
                                                                      ',rest))))
                              (:use-from
                                 (throwf 'eval-error
                                         "~s: :use-from clause needs package argument"
                                          'defpackage))
                              (t :)))
                          (atom
                            (throwf 'eval-error "~s: invalid clause: ~s"
                                    'defpackage atom))))))
    ^(let ((,pkg (or (find-package ,nstr)
                     (make-package ,nstr))))
       ,*exp-clauses
       ,pkg)))

(defmacro in-package (pkg)
  (unless (or (symbolp pkg) (stringp pkg))
    (throwf 'eval-error "~s: ~s isn't a package name" 'in-package pkg))
  ^(set *package* (or (find-package ',pkg)
                      (throwf 'eval-error "~s: no such package: ~s"
                              'in-package ',pkg))))
