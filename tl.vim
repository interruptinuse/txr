" VIM Syntax file for txr
" Kaz Kylheku <kaz@kylheku.com>

" INSTALL-HOWTO:
"
" 1. Create the directory .vim/syntax in your home directory and
"    put the files txr.vim and txl.vim into this directory.
" 2. In your .vimrc, add this command to associate *.txr and *.tl
"    files with the txr and txl filetypes:
"    :au BufRead,BufNewFile *.txr set filetype=txr | set lisp
"    :au BufRead,BufNewFile *.tl set filetype=txl | set lisp
"
" If you want syntax highlighting to be on automatically (for any language)
" you need to add ":syntax on" in your .vimrc also. But you knew that already!
"
" This file is generated by the genvim.txr script in the TXR source tree.

syn case match
syn spell toplevel

setlocal iskeyword=a-z,A-Z,48-57,!,$,&,*,+,-,:,<,=,>,?,\\,_,~,/

syn keyword txl_keyword contained %e% %pi% * *args*
syn keyword txl_keyword contained *args-full* *e* *flo-dig* *flo-epsilon*
syn keyword txl_keyword contained *flo-max* *flo-min* *full-args* *gensym-counter*
syn keyword txl_keyword contained *keyword-package* *lib-version* *listener-hist-len* *listener-multi-line-p*
syn keyword txl_keyword contained *pi* *place-clobber-expander* *place-delete-expander* *place-macro*
syn keyword txl_keyword contained *place-update-expander* *random-state* *self-path* *stddebug*
syn keyword txl_keyword contained *stderr* *stdin* *stdlog* *stdnull*
syn keyword txl_keyword contained *stdout* *system-package* *txr-version* *unhandled-hook*
syn keyword txl_keyword contained *user-package* + - /
syn keyword txl_keyword contained /= : :append :args
syn keyword txl_keyword contained :atime :auto :blksize :blocks
syn keyword txl_keyword contained :cdigit :chars :continue :counter
syn keyword txl_keyword contained :cspace :ctime :cword-char :decline
syn keyword txl_keyword contained :dev :digit :downcase :env
syn keyword txl_keyword contained :equal-based :fd :filter :fini
syn keyword txl_keyword contained :finish :form :from-current :from-end
syn keyword txl_keyword contained :from-start :from_html :frompercent :fromurl
syn keyword txl_keyword contained :fun :function :gap :gid
syn keyword txl_keyword contained :greedy :hextoint :init :ino
syn keyword txl_keyword contained :instance :into :lfilt :lines
syn keyword txl_keyword contained :list :longest :maxgap :maxtimes
syn keyword txl_keyword contained :method :mingap :mintimes :mode
syn keyword txl_keyword contained :mtime :name :named :next-spec
syn keyword txl_keyword contained :nlink :nothrow :prio :rdev
syn keyword txl_keyword contained :real-time :repeat-spec :resolve :rfilt
syn keyword txl_keyword contained :shortest :size :space :static
syn keyword txl_keyword contained :string :symacro :times :to_html
syn keyword txl_keyword contained :tofloat :toint :tonumber :topercent
syn keyword txl_keyword contained :tourl :uid :upcase :var
syn keyword txl_keyword contained :vars :weak-keys :weak-vals :whole
syn keyword txl_keyword contained :word-char < <= =
syn keyword txl_keyword contained > >= abort abs
syn keyword txl_keyword contained abs-path-p acons acons-new aconsql-new
syn keyword txl_keyword contained acos ado alist-nremove alist-remove
syn keyword txl_keyword contained all and andf ap
syn keyword txl_keyword contained apf append append* append-each
syn keyword txl_keyword contained append-each* apply aret ash
syn keyword txl_keyword contained asin assoc assql atan
syn keyword txl_keyword contained atan2 atom bignump bindable
syn keyword txl_keyword contained bit block boundp break-str
syn keyword txl_keyword contained caaaaar caaaadr caaaar caaadar
syn keyword txl_keyword contained caaaddr caaadr caaar caadaar
syn keyword txl_keyword contained caadadr caadar caaddar caadddr
syn keyword txl_keyword contained caaddr caadr caar cadaaar
syn keyword txl_keyword contained cadaadr cadaar cadadar cadaddr
syn keyword txl_keyword contained cadadr cadar caddaar caddadr
syn keyword txl_keyword contained caddar cadddar caddddr cadddr
syn keyword txl_keyword contained caddr cadr call call-clobber-expander
syn keyword txl_keyword contained call-delete-expander call-finalizers call-super-fun call-super-method
syn keyword txl_keyword contained call-update-expander callf car caseq
syn keyword txl_keyword contained caseql casequal cat-str cat-streams
syn keyword txl_keyword contained cat-vec catch catenated-stream-p catenated-stream-push
syn keyword txl_keyword contained cdaaaar cdaaadr cdaaar cdaadar
syn keyword txl_keyword contained cdaaddr cdaadr cdaar cdadaar
syn keyword txl_keyword contained cdadadr cdadar cdaddar cdadddr
syn keyword txl_keyword contained cdaddr cdadr cdar cddaaar
syn keyword txl_keyword contained cddaadr cddaar cddadar cddaddr
syn keyword txl_keyword contained cddadr cddar cdddaar cdddadr
syn keyword txl_keyword contained cdddar cddddar cdddddr cddddr
syn keyword txl_keyword contained cdddr cddr cdr ceil
syn keyword txl_keyword contained chain chand chdir chmod
syn keyword txl_keyword contained chr-int chr-isalnum chr-isalpha chr-isascii
syn keyword txl_keyword contained chr-isblank chr-iscntrl chr-isdigit chr-isgraph
syn keyword txl_keyword contained chr-islower chr-isprint chr-ispunct chr-isspace
syn keyword txl_keyword contained chr-isunisp chr-isupper chr-isxdigit chr-num
syn keyword txl_keyword contained chr-str chr-str-set chr-tolower chr-toupper
syn keyword txl_keyword contained chrp clamp clear-error clear-struct
syn keyword txl_keyword contained close-stream closelog cmp-str collect-each
syn keyword txl_keyword contained collect-each* comb compl-span-str cond
syn keyword txl_keyword contained conda cons conses conses*
syn keyword txl_keyword contained consp constantp copy copy-alist
syn keyword txl_keyword contained copy-cons copy-hash copy-list copy-str
syn keyword txl_keyword contained copy-struct copy-vec cos count-if
syn keyword txl_keyword contained countq countql countqual cum-norm-dist
syn keyword txl_keyword contained daemon dec defex define-modify-macro
syn keyword txl_keyword contained define-place-macro defmacro defmeth defparm
syn keyword txl_keyword contained defparml defplace defstruct defsymacro
syn keyword txl_keyword contained defun defvar defvarl del
syn keyword txl_keyword contained delay delete-package display-width do
syn keyword txl_keyword contained dohash dotimes downcase-str drop
syn keyword txl_keyword contained drop-until drop-while dup dupfd
syn keyword txl_keyword contained dwim each each* eighth
syn keyword txl_keyword contained empty endgrent endpwent ensure-dir
syn keyword txl_keyword contained env env-fbind env-hash env-vbind
syn keyword txl_keyword contained eq eql equal errno
syn keyword txl_keyword contained error eval evenp exception-subtype-p
syn keyword txl_keyword contained exec exit exit* exp
syn keyword txl_keyword contained expand-right expt exptmod false
syn keyword txl_keyword contained fboundp fifth fileno filter-equal
syn keyword txl_keyword contained filter-string-tree finalize find find-frame
syn keyword txl_keyword contained find-if find-max find-min find-package
syn keyword txl_keyword contained find-struct-type first fixnump flatcar
syn keyword txl_keyword contained flatcar* flatten flatten* flet
syn keyword txl_keyword contained flip flipargs flo-dig flo-epsilon
syn keyword txl_keyword contained flo-int flo-max flo-min flo-str
syn keyword txl_keyword contained floatp floor flush-stream fmakunbound
syn keyword txl_keyword contained for for* force fork
syn keyword txl_keyword contained format fourth fstat fun
syn keyword txl_keyword contained func-get-env func-get-form func-get-name func-set-env
syn keyword txl_keyword contained functionp gcd gen generate
syn keyword txl_keyword contained gensym gequal get-byte get-char
syn keyword txl_keyword contained get-clobber-expander get-delete-expander get-error get-error-str
syn keyword txl_keyword contained get-frames get-hash-userdata get-indent get-indent-mode
syn keyword txl_keyword contained get-line get-lines get-list-from-stream get-sig-handler
syn keyword txl_keyword contained get-string get-string-from-stream get-update-expander getegid
syn keyword txl_keyword contained getenv geteuid getgid getgrent
syn keyword txl_keyword contained getgrgid getgrnam getgroups gethash
syn keyword txl_keyword contained getitimer getpid getppid getpwent
syn keyword txl_keyword contained getpwnam getpwuid getuid ginterate
syn keyword txl_keyword contained giterate glob glob-altdirfunc glob-brace
syn keyword txl_keyword contained glob-err glob-mark glob-nocheck glob-noescape
syn keyword txl_keyword contained glob-nomagic glob-nosort glob-onlydir glob-period
syn keyword txl_keyword contained glob-tilde glob-tilde-check greater group-by
syn keyword txl_keyword contained gun handle handler-bind hash
syn keyword txl_keyword contained hash-alist hash-begin hash-construct hash-count
syn keyword txl_keyword contained hash-diff hash-eql hash-equal hash-from-pairs
syn keyword txl_keyword contained hash-isec hash-keys hash-list hash-next
syn keyword txl_keyword contained hash-pairs hash-proper-subset hash-revget hash-subset
syn keyword txl_keyword contained hash-uni hash-update hash-update-1 hash-values
syn keyword txl_keyword contained hashp have html-decode html-encode
syn keyword txl_keyword contained iapply identity ido if
syn keyword txl_keyword contained ifa iff iffi iflet
syn keyword txl_keyword contained ignerr in inc inc-indent
syn keyword txl_keyword contained indent-code indent-data indent-off inhash
syn keyword txl_keyword contained int-chr int-flo int-str integerp
syn keyword txl_keyword contained intern interp-fun-p interpose invoke-catch
syn keyword txl_keyword contained ip ipf isqrt itimer-prov
syn keyword txl_keyword contained itimer-real itimer-virtual juxt keep-if
syn keyword txl_keyword contained keep-if* keyword-package keywordp kill
syn keyword txl_keyword contained labels lambda last lazy-str
syn keyword txl_keyword contained lazy-str-force lazy-str-force-upto lazy-str-get-trailing-list lazy-stream-cons
syn keyword txl_keyword contained lazy-stringp lcm lcons lcons-fun
syn keyword txl_keyword contained lconsp ldiff length length-list
syn keyword txl_keyword contained length-str length-str-< length-str-<= length-str->
syn keyword txl_keyword contained length-str->= length-vec lequal less
syn keyword txl_keyword contained let let* lexical-fun-p lexical-lisp1-binding
syn keyword txl_keyword contained lexical-var-p lib-version link lisp-parse
syn keyword txl_keyword contained list list* list-str list-vec
syn keyword txl_keyword contained list-vector listp load log
syn keyword txl_keyword contained log-alert log-auth log-authpriv log-cons
syn keyword txl_keyword contained log-crit log-daemon log-debug log-emerg
syn keyword txl_keyword contained log-err log-info log-ndelay log-notice
syn keyword txl_keyword contained log-nowait log-odelay log-perror log-pid
syn keyword txl_keyword contained log-user log-warning log10 log2
syn keyword txl_keyword contained logand logior lognot logtest
syn keyword txl_keyword contained logtrunc logxor lstat mac-param-bind
syn keyword txl_keyword contained macro-form-p macro-time macroexpand macroexpand-1
syn keyword txl_keyword contained macrolet major make-catenated-stream make-env
syn keyword txl_keyword contained make-hash make-lazy-cons make-like make-package
syn keyword txl_keyword contained make-random-state make-similar-hash make-string-byte-input-stream make-string-input-stream
syn keyword txl_keyword contained make-string-output-stream make-strlist-output-stream make-struct make-struct-type
syn keyword txl_keyword contained make-sym make-time make-time-utc make-trie
syn keyword txl_keyword contained makedev makunbound mapcar mapcar*
syn keyword txl_keyword contained mapdo mapf maphash mappend
syn keyword txl_keyword contained mappend* mask match-fun match-regex
syn keyword txl_keyword contained match-regex-right match-regst match-regst-right match-str
syn keyword txl_keyword contained match-str-tree max member member-if
syn keyword txl_keyword contained memq memql memqual merge
syn keyword txl_keyword contained meth method min minor
syn keyword txl_keyword contained minusp mkdir mknod mkstring
syn keyword txl_keyword contained mlet mod multi multi-sort
syn keyword txl_keyword contained n-choose-k n-perm-k nconc new
syn keyword txl_keyword contained nil nilf ninth none
syn keyword txl_keyword contained not notf nreverse nthcdr
syn keyword txl_keyword contained null nullify num-chr num-str
syn keyword txl_keyword contained numberp oand obtain obtain-block
syn keyword txl_keyword contained oddp op open-command open-directory
syn keyword txl_keyword contained open-file open-fileno open-files open-files*
syn keyword txl_keyword contained open-pipe open-process open-tail openlog
syn keyword txl_keyword contained opip or orf package-alist
syn keyword txl_keyword contained package-name package-symbols packagep pad
syn keyword txl_keyword contained partition partition* partition-by path-blkdev-p
syn keyword txl_keyword contained path-chrdev-p path-dir-p path-executable-to-me-p path-exists-p
syn keyword txl_keyword contained path-file-p path-mine-p path-my-group-p path-newer
syn keyword txl_keyword contained path-older path-pipe-p path-private-to-me-p path-same-object
syn keyword txl_keyword contained path-setgid-p path-setuid-p path-sock-p path-sticky-p
syn keyword txl_keyword contained path-symlink-p path-writable-to-me-p perm pipe
syn keyword txl_keyword contained place-form-p placelet placelet* plusp
syn keyword txl_keyword contained poll poll-err poll-in poll-nval
syn keyword txl_keyword contained poll-out poll-pri poll-rdband poll-wrband
syn keyword txl_keyword contained pop pos pos-if pos-max
syn keyword txl_keyword contained pos-min posq posql posqual
syn keyword txl_keyword contained pppred ppred pprinl pprint
syn keyword txl_keyword contained pprof pred prinl print
syn keyword txl_keyword contained prof prog1 progn promisep
syn keyword txl_keyword contained prop proper-listp pset push
syn keyword txl_keyword contained pushhash pushnew put-byte put-char
syn keyword txl_keyword contained put-line put-lines put-string put-strings
syn keyword txl_keyword contained pwd qquote qref quote
syn keyword txl_keyword contained raise rand random random-fixnum
syn keyword txl_keyword contained random-state-p range range* range-regex
syn keyword txl_keyword contained rcomb read readlink real-time-stream-p
syn keyword txl_keyword contained reduce-left reduce-right ref refset
syn keyword txl_keyword contained regex-compile regex-parse regexp register-exception-subtypes
syn keyword txl_keyword contained regsub rehome-sym remhash remove-if
syn keyword txl_keyword contained remove-if* remove-path remq remq*
syn keyword txl_keyword contained remql remql* remqual remqual*
syn keyword txl_keyword contained rename-path repeat replace replace-list
syn keyword txl_keyword contained replace-str replace-struct replace-vec reset-struct
syn keyword txl_keyword contained rest ret retf return
syn keyword txl_keyword contained return-from reverse rlcp rlet
syn keyword txl_keyword contained rotate rperm rplaca rplacd
syn keyword txl_keyword contained run s-ifblk s-ifchr s-ifdir
syn keyword txl_keyword contained s-ififo s-iflnk s-ifmt s-ifreg
syn keyword txl_keyword contained s-ifsock s-irgrp s-iroth s-irusr
syn keyword txl_keyword contained s-irwxg s-irwxo s-irwxu s-isgid
syn keyword txl_keyword contained s-isuid s-isvtx s-iwgrp s-iwoth
syn keyword txl_keyword contained s-iwusr s-ixgrp s-ixoth s-ixusr
syn keyword txl_keyword contained search search-regex search-regst search-str
syn keyword txl_keyword contained search-str-tree second seek-stream select
syn keyword txl_keyword contained self-path seqp set set-diff
syn keyword txl_keyword contained set-hash-userdata set-indent set-indent-mode set-sig-handler
syn keyword txl_keyword contained setegid setenv seteuid setgid
syn keyword txl_keyword contained setgrent sethash setitimer setlogmask
syn keyword txl_keyword contained setpwent setuid seventh sh
syn keyword txl_keyword contained shift shuffle sig-abrt sig-alrm
syn keyword txl_keyword contained sig-bus sig-check sig-chld sig-cont
syn keyword txl_keyword contained sig-fpe sig-hup sig-ill sig-int
syn keyword txl_keyword contained sig-io sig-iot sig-kill sig-pipe
syn keyword txl_keyword contained sig-poll sig-prof sig-pwr sig-quit
syn keyword txl_keyword contained sig-segv sig-stkflt sig-stop sig-sys
syn keyword txl_keyword contained sig-term sig-trap sig-tstp sig-ttin
syn keyword txl_keyword contained sig-ttou sig-urg sig-usr1 sig-usr2
syn keyword txl_keyword contained sig-vtalrm sig-winch sig-xcpu sig-xfsz
syn keyword txl_keyword contained sign-extend sin sixth size-vec
syn keyword txl_keyword contained slot slotp slotset some
syn keyword txl_keyword contained sort sort-group source-loc source-loc-str
syn keyword txl_keyword contained span-str special-operator-p special-var-p splice
syn keyword txl_keyword contained split split-str split-str-set sqrt
syn keyword txl_keyword contained sssucc ssucc stat static-slot
syn keyword txl_keyword contained static-slot-ensure static-slot-p static-slot-set stdlib
syn keyword txl_keyword contained str< str<= str= str>
syn keyword txl_keyword contained str>= stream-get-prop stream-set-prop streamp
syn keyword txl_keyword contained string-extend string-lt stringp struct-type
syn keyword txl_keyword contained struct-type-p structp sub sub-list
syn keyword txl_keyword contained sub-str sub-vec subtypep succ
syn keyword txl_keyword contained super super-method swap symacrolet
syn keyword txl_keyword contained symbol-function symbol-name symbol-package symbol-value
syn keyword txl_keyword contained symbolp symlink sys:*lisp1* sys:abscond-from
syn keyword txl_keyword contained sys:bad-slot-syntax sys:capture-cont sys:do-path-test sys:dwim-del
syn keyword txl_keyword contained sys:dwim-set sys:eval-err sys:expand sys:expr
syn keyword txl_keyword contained sys:fbind sys:gc sys:gc-set-delta sys:get-fb
syn keyword txl_keyword contained sys:get-vb sys:handle-bad-syntax sys:l1-setq sys:l1-val
syn keyword txl_keyword contained sys:lbind sys:lisp1-setq sys:lisp1-value sys:load
syn keyword txl_keyword contained sys:make-struct-type sys:mark-special sys:obtain-impl sys:path-access
syn keyword txl_keyword contained sys:path-examine sys:path-test sys:path-test-mode sys:pl-expand
syn keyword txl_keyword contained sys:placelet-1 sys:prune-nil-inits sys:qquote sys:quasi
syn keyword txl_keyword contained sys:quasilist sys:reg-expand-nongreedy sys:reg-optimize sys:rplaca
syn keyword txl_keyword contained sys:rplacd sys:setq sys:setqf sys:splice
syn keyword txl_keyword contained sys:struct-lit sys:sym-clobber-expander sys:sym-delete-expander sys:sym-update-expander
syn keyword txl_keyword contained sys:top-fb sys:top-vb sys:unquote sys:var
syn keyword txl_keyword contained sys:with-saved-vars sys:yield-impl syslog system-package
syn keyword txl_keyword contained t take take-until take-while
syn keyword txl_keyword contained tan tb tc tenth
syn keyword txl_keyword contained test-set-indent-mode tf third throw
syn keyword txl_keyword contained throwf time time-fields-local time-fields-utc
syn keyword txl_keyword contained time-string-local time-string-utc time-struct-local time-struct-utc
syn keyword txl_keyword contained time-usec tofloat toint tok-str
syn keyword txl_keyword contained tok-where tostring tostringp tprint
syn keyword txl_keyword contained transpose tree-bind tree-case tree-find
syn keyword txl_keyword contained trie-add trie-compress trie-lookup-begin trie-lookup-feed-char
syn keyword txl_keyword contained trie-value-at trim-str true trunc
syn keyword txl_keyword contained trunc-rem truncate-stream tuples txr-case
syn keyword txl_keyword contained txr-case-impl txr-if txr-version txr-when
syn keyword txl_keyword contained typecase typeof typep umeth
syn keyword txl_keyword contained umethod unget-byte unget-char uniq
syn keyword txl_keyword contained unique unless unquote unsetenv
syn keyword txl_keyword contained until until* unwind-protect upcase-str
syn keyword txl_keyword contained update url-decode url-encode user-package
syn keyword txl_keyword contained usleep vec vec-list vec-push
syn keyword txl_keyword contained vec-set-length vecref vector vector-list
syn keyword txl_keyword contained vectorp w-continued w-coredump w-exitstatus
syn keyword txl_keyword contained w-ifcontinued w-ifexited w-ifsignaled w-ifstopped
syn keyword txl_keyword contained w-nohang w-stopsig w-termsig w-untraced
syn keyword txl_keyword contained wait weave when whenlet
syn keyword txl_keyword contained where while while* whilet
syn keyword txl_keyword contained width width-check with-clobber-expander with-delete-expander
syn keyword txl_keyword contained with-gensyms with-hash-iter with-in-string-byte-stream with-in-string-stream
syn keyword txl_keyword contained with-objects with-out-string-stream with-out-strlist-stream with-resources
syn keyword txl_keyword contained with-stream with-update-expander wrap wrap*
syn keyword txl_keyword contained yield yield-from zap zerop
syn keyword txl_keyword contained zip
syn match txr_nested_error "[^\t ]\+" contained
syn match txr_variable "\(@[ \t]*\)[*]\?[ \t]*[A-Za-z_][A-Za-z_0-9]*"
syn match txr_splicevar "@[ \t,*@]*[A-Za-z_][A-Za-z_0-9]*"
syn match txr_metanum "@\+[0-9]\+"
syn match txr_badesc "\\." contained
syn match txr_escat "\\@" contained
syn match txr_stresc "\\[abtnvfre\\ \n"`']" contained
syn match txr_numesc "\\x[0-9A-Fa-f]\+;\?" contained
syn match txr_numesc "\\[0-7]\+;\?" contained
syn match txr_regesc "\\[abtnvfre\\ \n/sSdDwW()\|.*?+~&%\[\]\-]" contained

syn match txr_chr "#\\x[0-9A-Fa-f]\+"
syn match txr_chr "#\\o[0-7]\+"
syn match txr_chr "#\\[^ \t\nA-Za-z_0-9]"
syn match txr_chr "#\\[A-Za-z_0-9]\+"
syn match txr_ncomment ";.*"

syn match txr_dot "\." contained
syn match txr_num "#x[+\-]\?[0-9A-Fa-f]\+"
syn match txr_num "#o[+\-]\?[0-7]\+"
syn match txr_num "#b[+\-]\?[01]\+"
syn match txr_ident "[A-Za-z_0-9!$%&*+\-<=>?\\_~]*[A-Za-z_!$%&*+\-<=>?\\_~^][A-Za-z_0-9!$%&*+\-<=>?\\_~^]*" contained
syn match txl_ident "[:@][A-Za-z_0-9!$%&*+\-<=>?\\_~^/]\+"
syn match txr_braced_ident "[:][A-Za-z_0-9!$%&*+\-<=>?\\_~^/]\+" contained
syn match txl_ident "[A-Za-z_0-9!$%&*+\-<=>?\\_~/]*[A-Za-z_!$%&*+\-<=>?\\_~^/#][A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]*"
syn match txr_num "[+\-]\?[0-9]\+\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match txr_badnum "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?[A-Za-z_!$%&*+\-<=>?\\_~^/#]\+"
syn match txr_num "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match txr_num "[+\-]\?[0-9]\+\([eE][+\-]\?[0-9]\+\)\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match txl_ident ":"
syn match txl_splice "[ \t,]\|,[*]"

syn match txr_unquote "," contained
syn match txr_splice ",\*" contained
syn match txr_quote "'" contained
syn match txr_quote "\^" contained
syn match txr_dotdot "\.\." contained
syn match txr_metaat "@" contained

syn region txr_bracevar matchgroup=Delimiter start="@[ \t]*[*]\?{" matchgroup=Delimiter end="}" contains=txr_num,txl_ident,txl_splice,txl_metanum,txr_metaat,txr_braced_ident,txr_dot,txr_dotdot,txr_string,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_regex,txr_quasilit,txr_chr,txl_splice,txr_nested_error
syn region txr_list matchgroup=Delimiter start="#\?H\?(" matchgroup=Delimiter end=")" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_bracket matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_mlist matchgroup=Delimiter start="@[ \t^',]*(" matchgroup=Delimiter end=")" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_mbracket matchgroup=Delimiter start="@[ \t^',]*\[" matchgroup=Delimiter end="\]" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_string start=+#\?\*\?"+ end=+["\n]+ contains=txr_stresc,txr_numesc,txr_badesc
syn region txr_quasilit start=+#\?\*\?`+ end=+[`\n]+ contains=txr_splicevar,txr_metanum,txr_bracevar,txr_mlist,txr_mbracket,txr_escat,txr_stresc,txr_numesc,txr_badesc
syn region txr_regex start="/" end="[/\n]" contains=txr_regesc,txr_numesc,txr_badesc
syn region txl_regex start="#/" end="[/\n]" contains=txr_regesc,txr_numesc,txr_badesc

hi def link txr_at Special
hi def link txr_atstar Special
hi def link txr_atat Special
hi def link txr_comment Comment
hi def link txr_ncomment Comment
hi def link txr_hashbang Preproc
hi def link txr_contin Preproc
hi def link txr_char String
hi def link txr_keyword Keyword
hi def link txl_keyword Type
hi def link txr_string String
hi def link txr_chr String
hi def link txr_quasilit String
hi def link txr_regex String
hi def link txl_regex String
hi def link txr_regdir String
hi def link txr_variable Identifier
hi def link txr_splicevar Identifier
hi def link txr_metanum Identifier
hi def link txr_escat Special
hi def link txr_stresc Special
hi def link txr_numesc Special
hi def link txr_regesc Special
hi def link txr_badesc Error
hi def link txr_ident Identifier
hi def link txl_ident Identifier
hi def link txr_num Number
hi def link txr_badnum Error
hi def link txr_quote Special
hi def link txr_unquote Special
hi def link txr_splice Special
hi def link txr_dot Special
hi def link txr_dotdot Special
hi def link txr_metaat Special
hi def link txr_munqspl Special
hi def link txl_splice Special
hi def link txr_error Error
hi def link txr_nested_error Error

let b:current_syntax = "lisp"

set lispwords=ado,ap,append-each,append-each*,aret,block,caseq,caseql,casequal,catch,collect-each,collect-each*,cond,conda,dec,defex,define-modify-macro,define-place-macro,defmacro,defmeth,defparm,defparml,defplace,defstruct,defsymacro,defun,defvar,defvarl,del,delay,do,dohash,dotimes,each,each*,flet,flip,for,for*,fun,gen,gun,handle,handler-bind,ido,if,ifa,iflet,ignerr,ip,labels,lambda,lcons,let,let*,load,mac-param-bind,macro-time,macrolet,meth,mlet,new,obtain,obtain-block,op,placelet,placelet*,pop,pprof,prof,prog1,progn,push,pushnew,ret,return,return-from,rlet,splice,symacrolet,sys:abscond-from,sys:expr,sys:fbind,sys:l1-val,sys:lbind,sys:lisp1-value,sys:path-examine,sys:path-test,sys:placelet-1,sys:splice,sys:struct-lit,sys:unquote,sys:var,sys:with-saved-vars,tb,tc,tree-bind,tree-case,txr-case,txr-case-impl,txr-if,txr-when,typecase,umeth,unless,unquote,until,until*,unwind-protect,when,whenlet,while,while*,whilet,with-clobber-expander,with-delete-expander,with-gensyms,with-hash-iter,with-in-string-byte-stream,with-in-string-stream,with-objects,with-out-string-stream,with-out-strlist-stream,with-resources,with-stream,with-update-expander,yield,yield-from,zap,:method,:function,:init,:fini
