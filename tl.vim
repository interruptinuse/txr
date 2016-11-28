" VIM Syntax file for txr
" Kaz Kylheku <kaz@kylheku.com>

" INSTALL-HOWTO:
"
" 1. Create the directory .vim/syntax in your home directory and
"    put the files txr.vim and tl.vim into this directory.
" 2. In your .vimrc, add this command to associate *.txr and *.tl
"    files with the txr and tl filetypes:
"    :au BufRead,BufNewFile *.txr set filetype=txr | set lisp
"    :au BufRead,BufNewFile *.tl set filetype=tl | set lisp
"
" If you want syntax highlighting to be on automatically (for any language)
" you need to add ":syntax on" in your .vimrc also. But you knew that already!
"
" This file is generated by the genvim.txr script in the TXR source tree.

syn case match
syn spell toplevel

setlocal iskeyword=a-z,A-Z,48-57,!,$,&,*,+,-,:,<,=,>,?,\\,_,~,/

syn keyword tl_keyword contained %e% %pi% * *args*
syn keyword tl_keyword contained *args-full* *e* *flo-dig* *flo-epsilon*
syn keyword tl_keyword contained *flo-max* *flo-min* *full-args* *gensym-counter*
syn keyword tl_keyword contained *lib-version* *listener-hist-len* *listener-multi-line-p* *listener-sel-inclusive-p*
syn keyword tl_keyword contained *load-path* *package* *pi* *place-clobber-expander*
syn keyword tl_keyword contained *place-delete-expander* *place-macro* *place-update-expander* *print-base*
syn keyword tl_keyword contained *print-circle* *print-flo-digits* *print-flo-format* *print-flo-precision*
syn keyword tl_keyword contained *random-state* *random-warmup* *self-path* *stddebug*
syn keyword tl_keyword contained *stderr* *stdin* *stdlog* *stdnull*
syn keyword tl_keyword contained *stdout* *trace-output* *txr-version* *unhandled-hook*
syn keyword tl_keyword contained + - / /=
syn keyword tl_keyword contained : :abandoned :addr :apf
syn keyword tl_keyword contained :append :args :atime :auto
syn keyword tl_keyword contained :awk-file :awk-rec :begin :begin-file
syn keyword tl_keyword contained :blksize :blocks :bool :byte-oriented
syn keyword tl_keyword contained :cdigit :chars :cint :close
syn keyword tl_keyword contained :continue :counter :cspace :ctime
syn keyword tl_keyword contained :cword-char :dec :decline :dev
syn keyword tl_keyword contained :digit :downcase :end :end-file
syn keyword tl_keyword contained :env :equal-based :explicit-no :fallback
syn keyword tl_keyword contained :fd :filter :fini :finish
syn keyword tl_keyword contained :float :form :from-current :from-end
syn keyword tl_keyword contained :from-start :from_html :frombase64 :fromhtml
syn keyword tl_keyword contained :frompercent :fromurl :fun :function
syn keyword tl_keyword contained :gap :gid :greedy :hex
syn keyword tl_keyword contained :hextoint :inf :init :ino
syn keyword tl_keyword contained :inp :inputs :instance :into
syn keyword tl_keyword contained :let :lfilt :lines :list
syn keyword tl_keyword contained :local :longest :mandatory :maxgap
syn keyword tl_keyword contained :maxtimes :method :mingap :mintimes
syn keyword tl_keyword contained :mode :mtime :name :named
syn keyword tl_keyword contained :next-spec :nlink :nothrow :oct
syn keyword tl_keyword contained :outf :outp :output :postinit
syn keyword tl_keyword contained :prio :rdev :real-time :reflect
syn keyword tl_keyword contained :repeat-spec :resolve :rfilt :set
syn keyword tl_keyword contained :set-file :shortest :size :space
syn keyword tl_keyword contained :static :str :string :symacro
syn keyword tl_keyword contained :times :tlist :to_html :tobase64
syn keyword tl_keyword contained :tofloat :tohtml :tohtml* :toint
syn keyword tl_keyword contained :tonumber :topercent :tourl :uid
syn keyword tl_keyword contained :upcase :use :use-from :use-syms
syn keyword tl_keyword contained :userdata :var :vars :weak-keys
syn keyword tl_keyword contained :weak-vals :whole :word-char :wrap
syn keyword tl_keyword contained < <= = >
syn keyword tl_keyword contained >= abort abs abs-path-p
syn keyword tl_keyword contained acons acons-new aconsql-new acos
syn keyword tl_keyword contained ado af-inet af-inet6 af-unix
syn keyword tl_keyword contained af-unspec ai-addrconfig ai-all ai-canonname
syn keyword tl_keyword contained ai-numerichost ai-numericserv ai-passive ai-v4mapped
syn keyword tl_keyword contained alet alist-nremove alist-remove all
syn keyword tl_keyword contained and andf ap apf
syn keyword tl_keyword contained append append* append-each append-each*
syn keyword tl_keyword contained apply aret ash asin
syn keyword tl_keyword contained assoc assql at-exit-call at-exit-do-not-call
syn keyword tl_keyword contained atan atan2 atom awk
syn keyword tl_keyword contained base64-decode base64-encode bignump bindable
syn keyword tl_keyword contained bit block block* boundp
syn keyword tl_keyword contained break-str brkint bs0 bs1
syn keyword tl_keyword contained bsdly build build-list butlast
syn keyword tl_keyword contained butlastn caaaaar caaaadr caaaar
syn keyword tl_keyword contained caaadar caaaddr caaadr caaar
syn keyword tl_keyword contained caadaar caadadr caadar caaddar
syn keyword tl_keyword contained caadddr caaddr caadr caar
syn keyword tl_keyword contained cadaaar cadaadr cadaar cadadar
syn keyword tl_keyword contained cadaddr cadadr cadar caddaar
syn keyword tl_keyword contained caddadr caddar cadddar caddddr
syn keyword tl_keyword contained cadddr caddr cadr call
syn keyword tl_keyword contained call-clobber-expander call-delete-expander call-finalizers call-super-fun
syn keyword tl_keyword contained call-super-method call-update-expander callf car
syn keyword tl_keyword contained caseq caseq* caseql caseql*
syn keyword tl_keyword contained casequal casequal* cat-str cat-streams
syn keyword tl_keyword contained cat-vec catch catenated-stream-p catenated-stream-push
syn keyword tl_keyword contained cbaud cbaudex cdaaaar cdaaadr
syn keyword tl_keyword contained cdaaar cdaadar cdaaddr cdaadr
syn keyword tl_keyword contained cdaar cdadaar cdadadr cdadar
syn keyword tl_keyword contained cdaddar cdadddr cdaddr cdadr
syn keyword tl_keyword contained cdar cddaaar cddaadr cddaar
syn keyword tl_keyword contained cddadar cddaddr cddadr cddar
syn keyword tl_keyword contained cdddaar cdddadr cdddar cddddar
syn keyword tl_keyword contained cdddddr cddddr cdddr cddr
syn keyword tl_keyword contained cdr ceil chain chand
syn keyword tl_keyword contained chdir chmod chr-digit chr-int
syn keyword tl_keyword contained chr-isalnum chr-isalpha chr-isascii chr-isblank
syn keyword tl_keyword contained chr-iscntrl chr-isdigit chr-isgraph chr-islower
syn keyword tl_keyword contained chr-isprint chr-ispunct chr-isspace chr-isunisp
syn keyword tl_keyword contained chr-isupper chr-isxdigit chr-num chr-str
syn keyword tl_keyword contained chr-str-set chr-tolower chr-toupper chr-xdigit
syn keyword tl_keyword contained chrp clamp clear-dirty clear-error
syn keyword tl_keyword contained clear-struct clearhash clocal close-stream
syn keyword tl_keyword contained closelog cmp-str cmspar collect-each
syn keyword tl_keyword contained collect-each* comb command-get command-get-lines
syn keyword tl_keyword contained command-get-string command-put command-put-lines command-put-string
syn keyword tl_keyword contained compare-swap compl-span-str cond conda
syn keyword tl_keyword contained condlet cons conses conses*
syn keyword tl_keyword contained consp constantp copy copy-alist
syn keyword tl_keyword contained copy-cons copy-hash copy-list copy-str
syn keyword tl_keyword contained copy-struct copy-vec cos count-if
syn keyword tl_keyword contained countq countql countqual cr0
syn keyword tl_keyword contained cr1 cr2 cr3 crdly
syn keyword tl_keyword contained cread crtscts crypt cs5
syn keyword tl_keyword contained cs6 cs7 cs8 csize
syn keyword tl_keyword contained cstopb cum-norm-dist daemon dec
syn keyword tl_keyword contained defex define-accessor define-modify-macro define-place-macro
syn keyword tl_keyword contained defmacro defmeth defpackage defparm
syn keyword tl_keyword contained defparml defplace defstruct defsymacro
syn keyword tl_keyword contained defun defvar defvarl del
syn keyword tl_keyword contained delay delete-package display-width do
syn keyword tl_keyword contained dohash dotimes downcase-str drop
syn keyword tl_keyword contained drop-until drop-while dup dupfd
syn keyword tl_keyword contained dwim each each* echo
syn keyword tl_keyword contained echoctl echoe echok echoke
syn keyword tl_keyword contained echonl echoprt eighth empty
syn keyword tl_keyword contained endgrent endpwent ensure-dir env
syn keyword tl_keyword contained env-fbind env-hash env-vbind eq
syn keyword tl_keyword contained eql equal errno error
syn keyword tl_keyword contained eval evenp exception-subtype-map exception-subtype-p
syn keyword tl_keyword contained exec exit exit* exp
syn keyword tl_keyword contained expand-left expand-right expt exptmod
syn keyword tl_keyword contained extproc f$ f^ f^$
syn keyword tl_keyword contained false fboundp ff0 ff1
syn keyword tl_keyword contained ffdly fifth file-append file-append-lines
syn keyword tl_keyword contained file-append-string file-get file-get-lines file-get-string
syn keyword tl_keyword contained file-put file-put-lines file-put-string fileno
syn keyword tl_keyword contained filter-equal filter-string-tree finalize find
syn keyword tl_keyword contained find-frame find-if find-max find-min
syn keyword tl_keyword contained find-package find-struct-type first fixnum-max
syn keyword tl_keyword contained fixnum-min fixnump flatcar flatcar*
syn keyword tl_keyword contained flatten flatten* flet flip
syn keyword tl_keyword contained flipargs flo-dig flo-epsilon flo-int
syn keyword tl_keyword contained flo-max flo-max-dig flo-min flo-str
syn keyword tl_keyword contained floatp floor flush-stream flusho
syn keyword tl_keyword contained fmakunbound fmt fnm-casefold fnm-leading-dir
syn keyword tl_keyword contained fnm-noescape fnm-pathname fnm-period fnmatch
syn keyword tl_keyword contained for for* force fork
syn keyword tl_keyword contained format fourth from fstat
syn keyword tl_keyword contained ftw ftw-actionretval ftw-chdir ftw-continue
syn keyword tl_keyword contained ftw-d ftw-depth ftw-dnr ftw-dp
syn keyword tl_keyword contained ftw-f ftw-mount ftw-ns ftw-phys
syn keyword tl_keyword contained ftw-skip-siblings ftw-skip-subtree ftw-sl ftw-sln
syn keyword tl_keyword contained ftw-stop fun func-get-env func-get-form
syn keyword tl_keyword contained func-get-name func-set-env functionp gcd
syn keyword tl_keyword contained gen generate gensym gequal
syn keyword tl_keyword contained get-byte get-char get-clobber-expander get-delete-expander
syn keyword tl_keyword contained get-error get-error-str get-frames get-hash-userdata
syn keyword tl_keyword contained get-indent get-indent-mode get-line get-lines
syn keyword tl_keyword contained get-list-from-stream get-sig-handler get-string get-string-from-stream
syn keyword tl_keyword contained get-update-expander getaddrinfo getegid getenv
syn keyword tl_keyword contained geteuid getgid getgrent getgrgid
syn keyword tl_keyword contained getgrnam getgroups gethash getitimer
syn keyword tl_keyword contained getopts getpid getppid getpwent
syn keyword tl_keyword contained getpwnam getpwuid getresgid getresuid
syn keyword tl_keyword contained getuid ginterate giterate glob
syn keyword tl_keyword contained glob-altdirfunc glob-brace glob-err glob-mark
syn keyword tl_keyword contained glob-nocheck glob-noescape glob-nomagic glob-nosort
syn keyword tl_keyword contained glob-onlydir glob-period glob-tilde glob-tilde-check
syn keyword tl_keyword contained go greater group-by group-reduce
syn keyword tl_keyword contained gun handle handler-bind hash
syn keyword tl_keyword contained hash-alist hash-begin hash-construct hash-count
syn keyword tl_keyword contained hash-diff hash-eql hash-equal hash-from-pairs
syn keyword tl_keyword contained hash-isec hash-keys hash-list hash-next
syn keyword tl_keyword contained hash-pairs hash-proper-subset hash-revget hash-subset
syn keyword tl_keyword contained hash-uni hash-update hash-update-1 hash-userdata
syn keyword tl_keyword contained hash-values hashp have html-decode
syn keyword tl_keyword contained html-encode html-encode* hupcl iapply
syn keyword tl_keyword contained icanon icrnl identity ido
syn keyword tl_keyword contained iexten if ifa iff
syn keyword tl_keyword contained iffi iflet ignbrk igncr
syn keyword tl_keyword contained ignerr ignpar ignwarn imaxbel
syn keyword tl_keyword contained in in-package in6addr-any in6addr-loopback
syn keyword tl_keyword contained inaddr-any inaddr-loopback inc inc-indent
syn keyword tl_keyword contained indent-code indent-data indent-off inhash
syn keyword tl_keyword contained inlcr inpck int-chr int-flo
syn keyword tl_keyword contained int-str integerp intern interp-fun-p
syn keyword tl_keyword contained interpose invoke-catch ip ipf
syn keyword tl_keyword contained iread isig isqrt istrip
syn keyword tl_keyword contained itimer-prov itimer-real itimer-virtual iuclc
syn keyword tl_keyword contained iutf8 ixany ixoff ixon
syn keyword tl_keyword contained juxt keep-if keep-if* keepq
syn keyword tl_keyword contained keepql keepqual keyword-package keywordp
syn keyword tl_keyword contained kill labels lambda last
syn keyword tl_keyword contained lazy-str lazy-str-force lazy-str-force-upto lazy-str-get-trailing-list
syn keyword tl_keyword contained lazy-stream-cons lazy-stringp lcm lcons
syn keyword tl_keyword contained lcons-fun lconsp ldiff length
syn keyword tl_keyword contained length-list length-str length-str-< length-str-<=
syn keyword tl_keyword contained length-str-> length-str->= length-vec lequal
syn keyword tl_keyword contained less let let* lexical-fun-p
syn keyword tl_keyword contained lexical-lisp1-binding lexical-var-p lib-version link
syn keyword tl_keyword contained lisp-parse list list* list-str
syn keyword tl_keyword contained list-vec list-vector listp lnew
syn keyword tl_keyword contained load log log-alert log-auth
syn keyword tl_keyword contained log-authpriv log-cons log-crit log-daemon
syn keyword tl_keyword contained log-debug log-emerg log-err log-info
syn keyword tl_keyword contained log-ndelay log-notice log-nowait log-odelay
syn keyword tl_keyword contained log-perror log-pid log-user log-warning
syn keyword tl_keyword contained log10 log2 logand logior
syn keyword tl_keyword contained lognot logtest logtrunc logxor
syn keyword tl_keyword contained lset lstat m$ m^
syn keyword tl_keyword contained m^$ mac-param-bind macro-ancestor macro-form-p
syn keyword tl_keyword contained macro-time macroexpand macroexpand-1 macrolet
syn keyword tl_keyword contained major make-catenated-stream make-env make-hash
syn keyword tl_keyword contained make-lazy-cons make-lazy-struct make-like make-package
syn keyword tl_keyword contained make-random-state make-similar-hash make-string-byte-input-stream make-string-input-stream
syn keyword tl_keyword contained make-string-output-stream make-strlist-input-stream make-strlist-output-stream make-struct
syn keyword tl_keyword contained make-struct-type make-sym make-time make-time-utc
syn keyword tl_keyword contained make-trie makedev makunbound mapcar
syn keyword tl_keyword contained mapcar* mapdo mapf maphash
syn keyword tl_keyword contained mappend mappend* mask match-fun
syn keyword tl_keyword contained match-regex match-regex-right match-regst match-regst-right
syn keyword tl_keyword contained match-str match-str-tree max mboundp
syn keyword tl_keyword contained member member-if memq memql
syn keyword tl_keyword contained memqual merge meth method
syn keyword tl_keyword contained min minor minusp mkdir
syn keyword tl_keyword contained mknod mkstring mlet mmakunbound
syn keyword tl_keyword contained mod multi multi-sort n-choose-k
syn keyword tl_keyword contained n-perm-k nconc neq neql
syn keyword tl_keyword contained nequal new nexpand-left nil
syn keyword tl_keyword contained nilf ninth nl0 nl1
syn keyword tl_keyword contained nldly noflsh none not
syn keyword tl_keyword contained notf nreconc nreverse nthcdr
syn keyword tl_keyword contained nthlast null nullify num-chr
syn keyword tl_keyword contained num-str numberp oand obtain
syn keyword tl_keyword contained obtain* obtain*-block obtain-block ocrnl
syn keyword tl_keyword contained oddp ofdel ofill olcuc
syn keyword tl_keyword contained onlcr onlret onocr op
syn keyword tl_keyword contained open-command open-directory open-file open-fileno
syn keyword tl_keyword contained open-files open-files* open-pipe open-process
syn keyword tl_keyword contained open-socket open-socket-pair open-tail openlog
syn keyword tl_keyword contained opip opost opt opthelp
syn keyword tl_keyword contained or orf package-alist package-fallback-list
syn keyword tl_keyword contained package-foreign-symbols package-local-symbols package-name package-symbols
syn keyword tl_keyword contained packagep pad parenb parmrk
syn keyword tl_keyword contained parodd partition partition* partition-by
syn keyword tl_keyword contained path-blkdev-p path-chrdev-p path-dir-p path-executable-to-me-p
syn keyword tl_keyword contained path-exists-p path-file-p path-mine-p path-my-group-p
syn keyword tl_keyword contained path-newer path-older path-pipe-p path-private-to-me-p
syn keyword tl_keyword contained path-read-writable-to-me-p path-readable-to-me-p path-same-object path-setgid-p
syn keyword tl_keyword contained path-setuid-p path-sock-p path-sticky-p path-strictly-private-to-me-p
syn keyword tl_keyword contained path-symlink-p path-writable-to-me-p pdec pendin
syn keyword tl_keyword contained perm pinc pipe place-form-p
syn keyword tl_keyword contained placelet placelet* plusp poll
syn keyword tl_keyword contained poll-err poll-in poll-nval poll-out
syn keyword tl_keyword contained poll-pri poll-rdband poll-rdhup poll-wrband
syn keyword tl_keyword contained pop pos pos-if pos-max
syn keyword tl_keyword contained pos-min posq posql posqual
syn keyword tl_keyword contained pppred ppred pprinl pprint
syn keyword tl_keyword contained pprof pred prinl print
syn keyword tl_keyword contained prof prog1 progn promisep
syn keyword tl_keyword contained prop proper-list-p proper-listp pset
syn keyword tl_keyword contained pure-rel-path-p push pushhash pushnew
syn keyword tl_keyword contained put-byte put-char put-line put-lines
syn keyword tl_keyword contained put-string put-strings pwd qquote
syn keyword tl_keyword contained qref quote r$ r^
syn keyword tl_keyword contained r^$ raise rand random
syn keyword tl_keyword contained random-fixnum random-state-get-vec random-state-p range
syn keyword tl_keyword contained range* range-regex rangep rcomb
syn keyword tl_keyword contained rcons read read-until-match readlink
syn keyword tl_keyword contained real-time-stream-p record-adapter reduce-left reduce-right
syn keyword tl_keyword contained ref refset regex-compile regex-from-trie
syn keyword tl_keyword contained regex-parse regex-source regexp register-exception-subtypes
syn keyword tl_keyword contained regsub rehome-sym remhash remove-if
syn keyword tl_keyword contained remove-if* remove-path remq remq*
syn keyword tl_keyword contained remql remql* remqual remqual*
syn keyword tl_keyword contained rename-path repeat replace replace-list
syn keyword tl_keyword contained replace-str replace-struct replace-vec reset-struct
syn keyword tl_keyword contained rest ret retf return
syn keyword tl_keyword contained return* return-from revappend reverse
syn keyword tl_keyword contained rfind rfind-if rlcp rlet
syn keyword tl_keyword contained rmember rmember-if rmemq rmemql
syn keyword tl_keyword contained rmemqual rotate rperm rplaca
syn keyword tl_keyword contained rplacd rpos rpos-if rposq
syn keyword tl_keyword contained rposql rposqual rr rra
syn keyword tl_keyword contained rsearch rslot run s-ifblk
syn keyword tl_keyword contained s-ifchr s-ifdir s-ififo s-iflnk
syn keyword tl_keyword contained s-ifmt s-ifreg s-ifsock s-irgrp
syn keyword tl_keyword contained s-iroth s-irusr s-irwxg s-irwxo
syn keyword tl_keyword contained s-irwxu s-isgid s-isuid s-isvtx
syn keyword tl_keyword contained s-iwgrp s-iwoth s-iwusr s-ixgrp
syn keyword tl_keyword contained s-ixoth s-ixusr search search-regex
syn keyword tl_keyword contained search-regst search-str search-str-tree second
syn keyword tl_keyword contained seek-stream select self-load-path self-path
syn keyword tl_keyword contained seqp set set-diff set-hash-userdata
syn keyword tl_keyword contained set-indent set-indent-mode set-package-fallback-list set-sig-handler
syn keyword tl_keyword contained setegid setenv seteuid setgid
syn keyword tl_keyword contained setgrent setgroups sethash setitimer
syn keyword tl_keyword contained setlogmask setpwent setresgid setresuid
syn keyword tl_keyword contained setuid seventh sh shift
syn keyword tl_keyword contained shuffle shut-rd shut-rdwr shut-wr
syn keyword tl_keyword contained sig-abrt sig-alrm sig-bus sig-check
syn keyword tl_keyword contained sig-chld sig-cont sig-fpe sig-hup
syn keyword tl_keyword contained sig-ill sig-int sig-io sig-iot
syn keyword tl_keyword contained sig-kill sig-pipe sig-poll sig-prof
syn keyword tl_keyword contained sig-pwr sig-quit sig-segv sig-stkflt
syn keyword tl_keyword contained sig-stop sig-sys sig-term sig-trap
syn keyword tl_keyword contained sig-tstp sig-ttin sig-ttou sig-urg
syn keyword tl_keyword contained sig-usr1 sig-usr2 sig-vtalrm sig-winch
syn keyword tl_keyword contained sig-xcpu sig-xfsz sign-extend sin
syn keyword tl_keyword contained sixth size-vec slet slot
syn keyword tl_keyword contained slotp slots slotset sock-accept
syn keyword tl_keyword contained sock-bind sock-cloexec sock-connect sock-dgram
syn keyword tl_keyword contained sock-family sock-listen sock-nonblock sock-peer
syn keyword tl_keyword contained sock-recv-timeout sock-send-timeout sock-set-peer sock-shutdown
syn keyword tl_keyword contained sock-stream sock-type some sort
syn keyword tl_keyword contained sort-group source-loc source-loc-str span-str
syn keyword tl_keyword contained special-operator-p special-var-p splice split
syn keyword tl_keyword contained split* split-str split-str-set sqrt
syn keyword tl_keyword contained sssucc ssucc stat static-slot
syn keyword tl_keyword contained static-slot-ensure static-slot-p static-slot-set stdlib
syn keyword tl_keyword contained str-in6addr str-in6addr-net str-inaddr str-inaddr-net
syn keyword tl_keyword contained str< str<= str= str>
syn keyword tl_keyword contained str>= stream-get-prop stream-set-prop streamp
syn keyword tl_keyword contained string-extend string-lt stringp struct-type
syn keyword tl_keyword contained struct-type-p structp sub sub-list
syn keyword tl_keyword contained sub-str sub-vec subtypep succ
syn keyword tl_keyword contained super super-method suspend swap
syn keyword tl_keyword contained symacrolet symbol-function symbol-macro symbol-name
syn keyword tl_keyword contained symbol-package symbol-value symbolp symlink
syn keyword tl_keyword contained sys:*lisp1* sys:*pl-env* sys:*trace-hash* sys:*trace-level*
syn keyword tl_keyword contained sys:abscond* sys:abscond-from sys:apply sys:awk-expander
syn keyword tl_keyword contained sys:awk-let sys:awk-redir sys:awk-test sys:bad-slot-syntax
syn keyword tl_keyword contained sys:capture-cont sys:circref sys:conv sys:conv-expand
syn keyword tl_keyword contained sys:conv-let sys:cp-origin sys:defmeth sys:do-conv
syn keyword tl_keyword contained sys:do-path-test sys:dwim-del sys:dwim-set sys:eval-err
syn keyword tl_keyword contained sys:expand sys:expr sys:fbind sys:gc
syn keyword tl_keyword contained sys:gc-set-delta sys:get-fun-getter-setter sys:get-mb sys:get-vb
syn keyword tl_keyword contained sys:handle-bad-syntax sys:if-to-cond sys:in6addr-condensed-text sys:l1-setq
syn keyword tl_keyword contained sys:l1-val sys:lbind sys:lisp1-setq sys:lisp1-value
syn keyword tl_keyword contained sys:list-builder-macrolets sys:make-struct-lit sys:make-struct-type sys:mark-special
syn keyword tl_keyword contained sys:name-str sys:obtain-impl sys:opt-dash sys:opt-err
syn keyword tl_keyword contained sys:path-access sys:path-examine sys:path-test sys:path-test-mode
syn keyword tl_keyword contained sys:pl-expand sys:placelet-1 sys:prune-missing-inits sys:qquote
syn keyword tl_keyword contained sys:quasi sys:quasilist sys:r-s-let-expander sys:reg-expand-nongreedy
syn keyword tl_keyword contained sys:reg-optimize sys:register-simple-accessor sys:rplaca sys:rplacd
syn keyword tl_keyword contained sys:rslotset sys:set-hash-rec-limit sys:set-hash-str-limit sys:set-macro-ancestor
syn keyword tl_keyword contained sys:setq sys:setqf sys:splice sys:str-inaddr-net-impl
syn keyword tl_keyword contained sys:struct-lit sys:switch sys:sym-clobber-expander sys:sym-delete-expander
syn keyword tl_keyword contained sys:sym-update-expander sys:top-fb sys:top-mb sys:top-vb
syn keyword tl_keyword contained sys:trace sys:trace-enter sys:trace-leave sys:unquote
syn keyword tl_keyword contained sys:untrace sys:var sys:wdwrap sys:with-saved-vars
syn keyword tl_keyword contained syslog system-package t tab0
syn keyword tl_keyword contained tab1 tab2 tab3 tabdly
syn keyword tl_keyword contained tagbody take take-until take-while
syn keyword tl_keyword contained tan tb tc tcdrain
syn keyword tl_keyword contained tcflow tcflush tcgetattr tciflush
syn keyword tl_keyword contained tcioff tcioflush tcion tcoflush
syn keyword tl_keyword contained tcooff tcoon tcsadrain tcsaflush
syn keyword tl_keyword contained tcsanow tcsendbreak tcsetattr tenth
syn keyword tl_keyword contained test-clear test-clear-dirty test-dec test-dirty
syn keyword tl_keyword contained test-inc test-set test-set-indent-mode tf
syn keyword tl_keyword contained third throw throwf time
syn keyword tl_keyword contained time-fields-local time-fields-utc time-parse time-string-local
syn keyword tl_keyword contained time-string-utc time-struct-local time-struct-utc time-usec
syn keyword tl_keyword contained to tofloat tofloatz toint
syn keyword tl_keyword contained tointz tok-str tok-where tostop
syn keyword tl_keyword contained tostring tostringp tprint trace
syn keyword tl_keyword contained transpose tree-bind tree-case tree-find
syn keyword tl_keyword contained trie-add trie-compress trie-lookup-begin trie-lookup-feed-char
syn keyword tl_keyword contained trie-value-at trim-str true trunc
syn keyword tl_keyword contained trunc-rem truncate-stream tuples txr-case
syn keyword tl_keyword contained txr-case-impl txr-if txr-path txr-sym
syn keyword tl_keyword contained txr-version txr-when typecase typeof
syn keyword tl_keyword contained typep umask umeth umethod
syn keyword tl_keyword contained uname unget-byte unget-char unintern
syn keyword tl_keyword contained uniq unique unless unquote
syn keyword tl_keyword contained unsetenv until until* untrace
syn keyword tl_keyword contained unuse-package unuse-sym unwind-protect upcase-str
syn keyword tl_keyword contained upd update url-decode url-encode
syn keyword tl_keyword contained use use-package use-sym user-package
syn keyword tl_keyword contained usl usleep uslot vdiscard
syn keyword tl_keyword contained vec vec-list vec-push vec-set-length
syn keyword tl_keyword contained vecref vector vector-list vectorp
syn keyword tl_keyword contained veof veol veol2 verase
syn keyword tl_keyword contained vintr vkill vlnext vmin
syn keyword tl_keyword contained vquit vreprint vstart vstop
syn keyword tl_keyword contained vsusp vswtc vt0 vt1
syn keyword tl_keyword contained vtdly vtime vwerase w-continued
syn keyword tl_keyword contained w-coredump w-exitstatus w-ifcontinued w-ifexited
syn keyword tl_keyword contained w-ifsignaled w-ifstopped w-nohang w-stopsig
syn keyword tl_keyword contained w-termsig w-untraced wait weave
syn keyword tl_keyword contained when whenlet where while
syn keyword tl_keyword contained while* whilet width width-check
syn keyword tl_keyword contained window-map window-mappend with-clobber-expander with-delete-expander
syn keyword tl_keyword contained with-gensyms with-hash-iter with-in-string-byte-stream with-in-string-stream
syn keyword tl_keyword contained with-objects with-out-string-stream with-out-strlist-stream with-resources
syn keyword tl_keyword contained with-slots with-stream with-update-expander wrap
syn keyword tl_keyword contained wrap* xcase yield yield-from
syn keyword tl_keyword contained zap zerop zip
syn match txr_nested_error "[^\t ]\+" contained
syn match txr_variable "\(@[ \t]*\)[*]\?[ \t]*[A-Za-z_][A-Za-z_0-9]*"
syn match txr_splicevar "@[ \t,*@]*[A-Za-z_][A-Za-z_0-9]*" contained
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
syn match tl_ident "[:@][A-Za-z_0-9!$%&*+\-<=>?\\_~^/]\+"
syn match txr_braced_ident "[:][A-Za-z_0-9!$%&*+\-<=>?\\_~^/]\+" contained
syn match tl_ident "[A-Za-z_0-9!$%&*+\-<=>?\\_~/]*[A-Za-z_!$%&*+\-<=>?\\_~^/#][A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]*"
syn match txr_num "[+\-]\?[0-9]\+\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match txr_badnum "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?[A-Za-z_!$%&*+\-<=>?\\_~^/#]\+"
syn match txr_num "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match txr_num "[+\-]\?[0-9]\+\([eE][+\-]\?[0-9]\+\)\([^A-Za-z_0-9!$%&*+\-<=>?\\_~^/#]\|\n\)"me=e-1
syn match tl_ident ":"
syn match tl_splice "[ \t,]\|,[*]"

syn match txr_unquote "," contained
syn match txr_splice ",\*" contained
syn match txr_quote "'" contained
syn match txr_quote "\^" contained
syn match txr_dotdot "\.\." contained
syn match txr_metaat "@" contained
syn match txr_circ "#[0-9]\+[#=]"

syn region txr_bracevar matchgroup=Delimiter start="@[ \t]*[*]\?{" matchgroup=Delimiter end="}" contains=txr_num,tl_ident,tl_splice,tl_metanum,txr_metaat,txr_circ,txr_braced_ident,txr_dot,txr_dotdot,txr_string,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_regex,txr_quasilit,txr_chr,txr_nested_error
syn region txr_list matchgroup=Delimiter start="\(#[HSR]\?\)\?(" matchgroup=Delimiter end=")" contains=tl_keyword,txr_string,tl_regex,txr_num,txr_badnum,tl_ident,txr_metanum,txr_ign_par,txr_ign_bkt,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_circ,txr_ncomment,txr_nested_error
syn region txr_bracket matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" contains=tl_keyword,txr_string,tl_regex,txr_num,txr_badnum,tl_ident,txr_metanum,txr_ign_par,txr_ign_bkt,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_circ,txr_ncomment,txr_nested_error
syn region txr_mlist matchgroup=Delimiter start="@[ \t^',]*(" matchgroup=Delimiter end=")" contains=tl_keyword,txr_string,tl_regex,txr_num,txr_badnum,tl_ident,txr_metanum,txr_ign_par,txr_ign_bkt,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_circ,txr_ncomment,txr_nested_error
syn region txr_mbracket matchgroup=Delimiter start="@[ \t^',]*\[" matchgroup=Delimiter end="\]" contains=tl_keyword,txr_string,tl_regex,txr_num,txr_badnum,tl_ident,txr_metanum,txr_ign_par,txr_ign_bkt,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_circ,txr_ncomment,txr_nested_error
syn region txr_string start=+#\?\*\?"+ end=+["\n]+ contains=txr_stresc,txr_numesc,txr_badesc
syn region txr_quasilit start=+#\?\*\?`+ end=+[`\n]+ contains=txr_splicevar,txr_metanum,txr_bracevar,txr_mlist,txr_mbracket,txr_escat,txr_stresc,txr_numesc,txr_badesc
syn region txr_regex start="/" end="[/\n]" contains=txr_regesc,txr_numesc,txr_badesc
syn region tl_regex start="#/" end="[/\n]" contains=txr_regesc,txr_numesc,txr_badesc
syn region txr_ign_par matchgroup=Comment start="#;[ \t',]*\(#[HSR]\?\)\?(" matchgroup=Comment end=")" contains=txr_ign_par_interior,txr_ign_bkt_interior
syn region txr_ign_bkt matchgroup=Comment start="#;[ \t',]*\(#[HSR]\?\)\?\[" matchgroup=Comment end="\]" contains=txr_ign_par_interior,txr_ign_bkt_interior
syn region txr_ign_par_interior contained matchgroup=Comment start="(" matchgroup=Comment end=")" contains=txr_ign_par_interior,txr_ign_bkt_interior
syn region txr_ign_bkt_interior contained matchgroup=Comment start="\[" matchgroup=Comment end="\]" contains=txr_ign_par_interior,txr_ign_bkt_interior

hi def link txr_at Special
hi def link txr_atstar Special
hi def link txr_atat Special
hi def link txr_comment Comment
hi def link txr_ncomment Comment
hi def link txr_hashbang Preproc
hi def link txr_contin Preproc
hi def link txr_char String
hi def link txr_keyword Keyword
hi def link tl_keyword Type
hi def link txr_string String
hi def link txr_chr String
hi def link txr_quasilit String
hi def link txr_regex String
hi def link tl_regex String
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
hi def link tl_ident Identifier
hi def link txr_num Number
hi def link txr_badnum Error
hi def link txr_quote Special
hi def link txr_unquote Special
hi def link txr_splice Special
hi def link txr_dot Special
hi def link txr_dotdot Special
hi def link txr_metaat Special
hi def link txr_circ Special
hi def link txr_munqspl Special
hi def link tl_splice Special
hi def link txr_error Error
hi def link txr_nested_error Error
hi def link txr_ign_par Comment
hi def link txr_ign_bkt_interior Comment
hi def link txr_ign_par_interior Comment
hi def link txr_ign_bkt Comment

let b:current_syntax = "lisp"

set lispwords=ado,alet,ap,append-each,append-each*,aret,awk,block,block*,build,caseq,caseq*,caseql,caseql*,casequal,casequal*,catch,collect-each,collect-each*,compare-swap,cond,conda,condlet,dec,defex,define-accessor,define-modify-macro,define-place-macro,defmacro,defmeth,defpackage,defparm,defparml,defplace,defstruct,defsymacro,defun,defvar,defvarl,del,delay,do,dohash,dotimes,each,each*,flet,flip,for,for*,fun,gen,go,gun,handle,handler-bind,ido,if,ifa,iflet,ignerr,ignwarn,in-package,ip,labels,lambda,lcons,let,let*,lset,mac-param-bind,macro-time,macrolet,mlet,obtain,obtain*,obtain*-block,obtain-block,op,pdec,pinc,placelet,placelet*,pop,pprof,prof,prog1,progn,push,pushnew,ret,return,return-from,rlet,rslot,slet,splice,suspend,symacrolet,sys:abscond-from,sys:awk-let,sys:awk-redir,sys:conv,sys:expr,sys:fbind,sys:l1-val,sys:lbind,sys:lisp1-value,sys:path-examine,sys:path-test,sys:placelet-1,sys:splice,sys:struct-lit,sys:switch,sys:unquote,sys:var,sys:with-saved-vars,tagbody,tb,tc,test-clear,test-dec,test-inc,test-set,trace,tree-bind,tree-case,txr-case,txr-case-impl,txr-if,txr-when,typecase,unless,unquote,until,until*,untrace,unwind-protect,upd,when,whenlet,while,while*,whilet,with-clobber-expander,with-delete-expander,with-gensyms,with-hash-iter,with-in-string-byte-stream,with-in-string-stream,with-objects,with-out-string-stream,with-out-strlist-stream,with-resources,with-slots,with-stream,with-update-expander,yield,yield-from,zap,:method,:function,:init,:postinit,:fini
