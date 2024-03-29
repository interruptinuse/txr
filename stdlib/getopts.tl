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

(defex opt-error error)

(defstruct opt-desc nil
  short
  long
  helptext
  arg-p
  (type :bool)
  (:static valid-types '(:bool :dec :hex :oct :cint :float :str :text))
  (:postinit (me)
    me.(check)
    (set me.arg-p (neq me.type :bool))))

(defstruct (sys:opt-parsed name arg desc : eff-type) nil
  name
  arg  ;; string, integer, real, ...
  desc ;; opt-desc
  eff-type
  cumul
  (:postinit (me) me.(convert-type)))

(defstruct opts nil
  (opt-hash (hash :equal-based)) ;; string to sys:opt-parsed
  in-args
  out-args)

(defstruct sys:opt-processor nil
  od-list
  (od-hash (hash :equal-based)) ;; string to opt-desc
  opts
  (:postinit (me)
    me.(build-hash)))

(defun sys:opt-err (. args)
  (throwf 'opt-error . args))

(defun getopts-error (msg . args)
  (error `~s: @msg` 'getopts . args))

(defun sys:opt-dash (name)
  `@(if (> (length name) 1) "-")-@name`)

(defmeth opt-desc basic-type-p (me type)
  (tree-case type
    ((type name) (and (neq type :bool)
                      me.(basic-type-p type)
                      (stringp name)))
    (type (or (functionp type) (fboundp type) (member type me.valid-types)))))

(defmeth opt-desc list-type-p (me type)
  (tree-case type
    ((indicator btype) (and (eq indicator 'list)
                            (neq btype :bool)
                            me.(basic-type-p btype)))
    (t nil)))

(defmeth opt-desc cumul-type-p (me type)
  (tree-case type
    ((indicator btype) (and (eq indicator 'usr:cumul)
                            (neq btype :bool)
                            (or me.(basic-type-p btype)
                                me.(list-type-p btype))))
    (t nil)))

(defmeth opt-desc check (me)
  (unless (or me.(basic-type-p me.type)
              me.(list-type-p me.type)
              me.(cumul-type-p me.type))
    (getopts-error "invalid option type specifier ~s"
                   me.type))
  (when me.long
    (when (< (length me.long) 2)
      (getopts-error "long option ~a has a short name" me.long))
    (when (eql [me.long 0] #\-)
      (getopts-error "long option ~a starts with - character" me.long)))
  (when me.short
    (when (neq (length me.short) 1)
      (getopts-error "short option ~a not one character long" me.short))
    (when (eql [me.short 0] #\-)
      (getopts-error "short option ~a starts with - character" me.short))))

(defmeth sys:opt-parsed convert-type (me)
  (let ((name (sys:opt-dash me.name))
        (type (or me.eff-type me.desc.type)))
    (when (and (neq type :bool)
               (eq me.arg :explicit-no))
      (sys:opt-err "Non-Boolean option ~a explicitly specified as false" name))
    (caseql [[iffi [andf consp [chain car keywordp]] car] type]
      (:bool
        (set me.arg (neq me.arg :explicit-no)))
      (:dec (set me.arg
                 (or (and (r^$ #/[+\-]?\d+/ me.arg) (int-str me.arg))
                     (sys:opt-err "option ~a needs decimal integer arg, not ~a"
                              name me.arg))))
      (:hex (set me.arg
                 (or (and (r^$ #/[+\-]?[\da-fA-F]+/ me.arg) (int-str me.arg 16))
                     (sys:opt-err "option ~a needs hexadecimal integer arg, not ~a"
                              name me.arg))))
      (:oct (set me.arg
                 (or (and (r^$ #/[+\-]?[0-7]+/ me.arg) (int-str me.arg 8))
                     (sys:opt-err "option ~a needs octal integer arg, not ~a"
                              name me.arg))))
      (:cint (set me.arg
                  (cond
                    ((r^$ #/[+\-]?0x[\da-fA-F]+/ me.arg)
                     (int-str (regsub "0x" "" me.arg) 16))
                    ((r^$ #/[+\-]?0[0-7]+/ me.arg)
                     (int-str me.arg 8))
                    ((r^$ #/[+\-]?0[\da-fA-F]+/ me.arg)
                     (sys:opt-err "option ~a argument ~a non octal, but leading 0"
                              name me.arg))
                    ((r^$ #/[+\-]?\d+/ me.arg)
                     (int-str me.arg))
                    (t (sys:opt-err "option ~a needs C style numeric arg, not ~a"
                                name me.arg)))))
      (:float (set me.arg
                   (cond
                     ([[chand (orf (f^$ #/[+\-]?\d+[.]?([Ee][+\-]?\d+)?/)
                                   (f^$ #/[+\-]?\d*[.]?\d+([Ee][+\-]?\d+)?/))
                              flo-str] me.arg])
                     (t (sys:opt-err "option ~a needs floating-point arg, not ~a"
                                 name me.arg)))))
      (:str (set me.arg
                 (or (ignerr (read `"@{me.arg}"`))
                     (sys:opt-err "option ~a needs string lit syntax, ~a given"
                              name me.arg))))
      (:text)
      (t (cond
           ((and (consp type) (eq (car type) 'list))
              (let* ((rec-type (cadr type))
                     (pieces (split-str me.arg #/,/))
                     (sub-opts (mapcar (do new (sys:opt-parsed me.name @1
                                                               me.desc
                                                               rec-type))
                                       pieces)))
                (set me.arg (mapcar (usl arg) sub-opts))))
           ((and (consp type) (eq (car type) 'cumul))
              (let* ((rec-type (cadr type))
                     (sub-opt (new (sys:opt-parsed me.name me.arg
                                                   me.desc rec-type))))
                (set me.arg sub-opt.arg
                     me.cumul t)))
           ((or (symbolp type) (functionp type))
              (set me.arg (call type me.arg))))))))

(defmeth opts lambda (me key : dfl)
  (iflet ((o [me.opt-hash key])) o.arg dfl))

(defmeth opts lambda-set (me key val)
  (iflet ((o [me.opt-hash key]))
    (set o.arg val)
    (error "opts: cannot set option ~s to ~s: no such option" key val)))

(defmeth opts add-opt (me opt)
  (when opt.cumul
    (let* ((old-opt [me.opt-hash (or opt.desc.long
                                     opt.desc.short)])
           (old-arg (if old-opt old-opt.arg)))
      (set opt.arg (cons opt.arg old-arg))))
  (whenlet ((n opt.desc.short))
    (set [me.opt-hash n] opt))
  (whenlet ((n opt.desc.long))
    (set [me.opt-hash n] opt)))

(defmeth sys:opt-processor build-hash (me)
  (each ((od me.od-list))
    (unless (or od.long od.short)
      (error "opt-processor: no short or long name in option ~s" od))
    (each ((str (list od.long od.short)))
      (when (and str [me.od-hash str])
        (error "opt-processor: duplicate option ~s" str))
      (set [me.od-hash str] od))))

(defmeth sys:opt-processor parse-long (me opt : arg)
  (iflet ((ieq (unless (stringp arg) (break-str opt "="))))
    (let ((oname [opt 0..ieq])
          (arg [opt (succ ieq)..:]))
      me.(parse-long oname arg))
    (let ((od [me.od-hash opt])
          (opts me.opts))
      (cond
        ((null od)
           (sys:opt-err "unrecognized option: --~a" opt))
        ((and arg od.arg-p)
           opts.(add-opt (new (sys:opt-parsed opt arg od))))
        ((stringp arg)
           (sys:opt-err "option --~a doesn't take an argument" opt))
        (od.arg-p
           (iflet ((arg (pop opts.out-args)))
             opts.(add-opt (new (sys:opt-parsed opt arg od)))
             (sys:opt-err "option --~a requires an argument" opt)))
        (t opts.(add-opt (new (sys:opt-parsed opt arg od))))))))

(defmeth sys:opt-processor parse-shorts (me oarg)
  (each ((o (split-str oarg #//)))
    (iflet ((opts me.opts)
            (od [me.od-hash o]))
      (let ((arg (when od.arg-p
                   (when (> (length oarg) 1)
                     (sys:opt-err "argument -~a includes -~a, which does not clump"
                                  oarg o))
                 (unless opts.out-args
                   (sys:opt-err "option -~a requires an argument" o))
                 (pop opts.out-args))))
        opts.(add-opt (new (sys:opt-parsed o arg od))))
      (sys:opt-err "unrecognized option: -~a" o))))

(defmeth sys:opt-processor parse-opts (me)
  (let ((opts me.opts))
    (whilet ((arg (pop opts.out-args)))
      (cond
        ((equal "--" arg) (return))
        ((r^ #/--no-/ arg) me.(parse-long [arg 5..:] :explicit-no))
        ((r^ #/--/ arg) me.(parse-long [arg 2..:]))
        ((r^ #/-.+/ arg) me.(parse-shorts [arg 1..:]))
        (t (push arg opts.out-args)
           (return))))
    opts))

(defun sys:wdwrap (string columns)
  (let ((words (tok-str string #/\S+/))
        line)
    (build
      (whilet ((word (pop words))
               (wpart (cond
                        ((and word (r^$ #/\w+[\w\-]*\w[.,;:!?"]?/ word))
                           (split-str word #/-/))
                        (word (list word))))
               (wpart-orig wpart))
        (whilet ((wp0 (eq wpart wpart-orig))
                 (wp (pop wpart))
                 (w (if wp `@wp@(if wpart "-")`)))
          (cond
            ((not line)
               (set line w))
            ((> (+ (length line) (length w) 1) columns)
               (add line)
               (set line w))
            (t (set line `@line@(if wp0 " ")@w`)))))
      (if line
        (add line)))))

(defun opt (short long : (type :bool) helptext)
  (new opt-desc short short long long helptext helptext type type))

(defun getopts (opt-desc-list args)
  (let* ((opts (new opts in-args args out-args args))
         (opr (new sys:opt-processor od-list opt-desc-list opts opts)))
    opr.(parse-opts)))

(defun opthelp (opt-desc-list : (*stdout* *stdout*))
  (let ((sorted [nsort (copy-list (remove-if (op null @1.helptext)
                                             opt-desc-list)) :
                       (do if @1.long @1.long @1.short)])
        (undocumented (keep-if (op null @1.helptext) opt-desc-list)))
    (put-line)
    (when sorted
      (put-line "Options:\n")
      (each ((od sorted))
        (let* ((type (if (and (consp od.type) (eq (car od.type) 'cumul))
                       (cadr od.type)
                       od.type))
               (tstr (cond
                       ((keywordp type) (upcase-str (symbol-name type)))
                       ((not (consp type)) "ARG")
                       ((eq (car type) 'list)
                        (let ((ts (ifa (consp (cadr type))
                                    (cadr it)
                                    (upcase-str (symbol-name it)))))
                          `@ts[,@ts...]`))
                       (t (cadr type))))
               (long (if od.long
                       `--@{od.long}@(if od.arg-p `=@tstr`)`))
               (short (if od.short
                        `-@{od.short}@(if od.arg-p ` @tstr`)`))
               (ls (cond
                     ((and long short) `@{long 21} (@short)`)
                     (long long)
                     (short `@{"" 21}  @short`)))
               (lines (if od.helptext  (sys:wdwrap od.helptext 43))))
          (if (>= (len ls) 34)
            (put-line `  @ls`)
            (put-line `  @{ls 34}@(pop lines)`))
          (while lines
            (put-line `  @{"" 34}@(pop lines)`))))
      (put-line))
    (when undocumented
      (put-line "Undocumented options:\n")
      (let* ((undoc-str `@{[mapcar sys:opt-dash
                                   (flatten (mappend (op list @1.short @1.long)
                                                     undocumented))] ", "}`))
        (each ((line (sys:wdwrap undoc-str 77)))
          (put-line `  @line`)))
      (put-line))))

(defun opthelp-conventions (opt-desc-list : (*stdout* *stdout*))
  (let ((documented (remove-if (op null @1.helptext) opt-desc-list))
        (undocumented (keep-if (op null @1.helptext) opt-desc-list)))
    (put-line "Option conventions:\n")
    (let* ((have-short (some documented (usl short)))
           (have-long (some documented (usl long)))
           (have-arg-p (some documented (usl arg-p)))
           (have-bool (some documented (op eq @1.type :bool)))
           (texts (list (if have-short
                          "Short options can be invoked with long syntax: \ \
                           for example, --a can be used when -a exists.\ \
                           Short no-argument options can be clumped into\ \
                           one argument as exemplified by -xyz.")
                        (if have-bool
                          (if have-arg-p
                            "Options that take no argument are Boolean:"
                            (if undocumented
                              "All documented options are Boolean:"
                              "All options are Boolean:")))
                        (if have-bool
                          "they are true when present, false when absent.")
                        (if have-bool
                          (if have-arg-p
                            "The --no- prefix can explicitly specify\ \
                             Boolean options as false: if a Boolean option\ \
                             X exists,\ \
                             --no-X specifies it as false. This is useful\ \
                             for making false those options which default\ \
                             to true."
                            "The --no- prefix can explicitly specify\ \
                             options as false: if an X option exists,\ \
                             --no-X specifies it as false. This is useful\ \
                             for making false those options which default\ \
                             to true."))
                        (if have-bool
                          (if (not have-long)
                            "Note the double dash on --no."
                            (if have-short
                              "The --no- prefix can be applied to a short\ \
                               or long option name.")))
                        (if (and have-long have-arg-p)
                          "The argument to a long option can be given in one\ \
                           argument as --option=arg or as a separate\ \
                           argument using --option arg.")
                        "The special argument -- can be used where an option\ \
                         may appear. It means \"end of options\": the\ \
                         arguments which follow are not treated as options\ \
                         even if they look like options.")))
      (mapdo (do put-line `  @1`)
             (sys:wdwrap `@{(flatten texts)}` 77)))
    (put-line)))

(defun opthelp-types (opt-desc-list : (*stdout* *stdout*))
  (whenlet ((documented (remove-if (op null @1.helptext) opt-desc-list))
            (types (keep-if [andf keywordp (op neq :bool)]
                            (uniq (mapcar (chain (usl type)
                                                 (ldo match-ecase
                                                   ((@(or list cumul) @type)
                                                    @(rec type))
                                                   ((@type @nil) type)
                                                   (@type type)))
                                          documented))))
            (entries (isec
                      '((:dec   "  DEC   - Decimal integer: -123, 0, 5, +73")
                        (:hex   "  HEX   - Hexadecimal integer: -EF, 2D0, +9A")
                        (:oct   "  OCT   - Octal integer: -773, 5677, +326")
                        (:cint  "  CINT  - C-style integer: leading 0 octal,\
                                \ leading 0x hex, else decimal;\n\
                                \          leading sign allowed: -0777, 0xFDC,\
                                \ +123")
                        (:float "  FLOAT - Floating-point: -1.3e+03, +5, 3.3,\
                                \ 3., .5, .12e9, 53.e-3, 3e-015")
                        (:str   "  STR   - String with embedded escapes, valid\
                                \ as TXR Lisp string literal\n\
                                \          syntax: foo, foo\\tbar, abc\\nxyz")
                        (:text  "  TEXT  - Unprocessed text"))
                      types : [iffi consp car])))
    (put-line "Type legend:\n")
    (mapdo (tb ((btype legend))
             ;; TODO Make coherent the punctuation?
             (put-line legend)
             (whenlet ((names (uniq (mappend (chain (usl type)
                                                    (ldo match-case
                                                      ((@(or list cumul) @type)
                                                       @(rec type))
                                                      ((@btype @name)
                                                       (list name))))
                                             documented))))
               (mapdo (do put-line `@{"" 10}@1`)
                      (sys:wdwrap `Arguments of this type:\
                                  \ @(cat-str names #\ )` 69))))
           entries)
    (put-line)))

(defstruct sys:option-base nil
  in-args
  out-args
  (:static slot-hash)
  (:static opt-desc-list)
  (:method add-opt (me opt)
    (let* ((sl [me.slot-hash (or opt.desc.long opt.desc.short)])
           (arg (if opt.cumul
                  (cons opt.arg (slot me sl))
                  opt.arg)))
      (slotset me sl arg)))
  (:method getopts (me args)
    (set me.in-args args me.out-args args)
    (let ((opr (new sys:opt-processor od-list me.opt-desc-list opts me)))
      opr.(parse-opts)))
  (:method opthelp (me : (stream *stdout*))
    (opthelp me.opt-desc-list stream))
  (:method opthelp-conventions (me : (stream *stdout*))
    (opthelp-conventions me.opt-desc-list stream))
  (:method opthelp-types (me : (stream *stdout*))
    (opthelp-types me.opt-desc-list stream)))

(defmacro define-option-struct (name super-spec . opts)
  (let* ((slots (mapcar (tb ((short long . t))
                          (or long short))
                        opts))
         (supers (if (and super-spec (atom super-spec))
                   (list super-spec)
                   super-spec)))
    ^(defstruct ,name (,*supers sys:option-base)
       ,*slots
       (:static slot-hash #H(() ,*(mapcar [juxt symbol-name identity] slots)))
       (:static opt-desc-list ',(mapcar (tb ((short long . rest))
                                          (opt (if short (symbol-name short))
                                               (if long (symbol-name long))
                                               . rest))
                                        opts)))))
