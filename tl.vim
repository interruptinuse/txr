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

setlocal iskeyword=a-z,A-Z,48-57,!,$,&,*,+,-,<,=,>,?,\\,_,~,/

syn keyword txl_keyword contained * *args* *e* *flo-dig*
syn keyword txl_keyword contained *flo-epsilon* *flo-max* *flo-min* *full-args*
syn keyword txl_keyword contained *gensym-counter* *keyword-package* *pi* *place-clobber-expander*
syn keyword txl_keyword contained *place-delete-expander* *place-update-expander* *random-state* *self-path*
syn keyword txl_keyword contained *stddebug* *stderr* *stdin* *stdlog*
syn keyword txl_keyword contained *stdnull* *stdout* *txr-version* *unhandled-hook*
syn keyword txl_keyword contained *user-package* + - /
syn keyword txl_keyword contained /= < <= =
syn keyword txl_keyword contained > >= abort abs
syn keyword txl_keyword contained abs-path-p acons acons-new aconsql-new
syn keyword txl_keyword contained acos ado alist-nremove alist-remove
syn keyword txl_keyword contained all and andf ap
syn keyword txl_keyword contained apf append append* append-each
syn keyword txl_keyword contained append-each* apply aret ash
syn keyword txl_keyword contained asin assoc assql atan
syn keyword txl_keyword contained atan2 atom bignump bit
syn keyword txl_keyword contained block boundp break-str call
syn keyword txl_keyword contained call-clobber-expander call-delete-expander call-update-expander callf
syn keyword txl_keyword contained car caseq caseql casequal
syn keyword txl_keyword contained cat-str cat-streams cat-vec catch
syn keyword txl_keyword contained catenated-stream-p catenated-stream-push cdr ceil
syn keyword txl_keyword contained chain chand chdir chmod
syn keyword txl_keyword contained chr-isalnum chr-isalpha chr-isascii chr-isblank
syn keyword txl_keyword contained chr-iscntrl chr-isdigit chr-isgraph chr-islower
syn keyword txl_keyword contained chr-isprint chr-ispunct chr-isspace chr-isunisp
syn keyword txl_keyword contained chr-isupper chr-isxdigit chr-num chr-str
syn keyword txl_keyword contained chr-str-set chr-tolower chr-toupper chrp
syn keyword txl_keyword contained clear-error close-stream closelog cmp-str
syn keyword txl_keyword contained collect-each collect-each* comb compl-span-str
syn keyword txl_keyword contained cond cons conses conses*
syn keyword txl_keyword contained consp constantp copy copy-alist
syn keyword txl_keyword contained copy-cons copy-hash copy-list copy-str
syn keyword txl_keyword contained copy-vec cos count-if countq
syn keyword txl_keyword contained countql countqual cum-norm-dist daemon
syn keyword txl_keyword contained dec define-modify-macro defmacro defparm
syn keyword txl_keyword contained defplace defsymacro defun defvar
syn keyword txl_keyword contained del delay delete-package do
syn keyword txl_keyword contained dohash dotimes downcase-str dup
syn keyword txl_keyword contained dupfd dwim each each*
syn keyword txl_keyword contained empty ensure-dir env env-fbind
syn keyword txl_keyword contained env-hash env-vbind eq eql
syn keyword txl_keyword contained equal errno error eval
syn keyword txl_keyword contained evenp exec exit exit*
syn keyword txl_keyword contained exp expt exptmod false
syn keyword txl_keyword contained fbind fboundp fifth fileno
syn keyword txl_keyword contained filter-equal filter-string-tree finalize find
syn keyword txl_keyword contained find-if find-max find-min find-package
syn keyword txl_keyword contained first fixnump flatten flatten*
syn keyword txl_keyword contained flet flip flipargs flo-int
syn keyword txl_keyword contained flo-str floatp floor flush-stream
syn keyword txl_keyword contained fmakunbound for for* force
syn keyword txl_keyword contained fork format fourth fun
syn keyword txl_keyword contained func-get-env func-get-form func-set-env functionp
syn keyword txl_keyword contained gcd gen generate gensym
syn keyword txl_keyword contained gequal get-byte get-char get-clobber-expander
syn keyword txl_keyword contained get-delete-expander get-error get-error-str get-hash-userdata
syn keyword txl_keyword contained get-line get-lines get-list-from-stream get-sig-handler
syn keyword txl_keyword contained get-string get-string-from-stream get-update-expander getenv
syn keyword txl_keyword contained gethash getitimer getpid getppid
syn keyword txl_keyword contained giterate glob glob-altdirfunc glob-brace
syn keyword txl_keyword contained glob-err glob-mark glob-nocheck glob-noescape
syn keyword txl_keyword contained glob-nomagic glob-nosort glob-onlydir glob-period
syn keyword txl_keyword contained glob-tilde glob-tilde-check greater group-by
syn keyword txl_keyword contained gun hash hash-alist hash-construct
syn keyword txl_keyword contained hash-count hash-diff hash-eql hash-equal
syn keyword txl_keyword contained hash-isec hash-keys hash-pairs hash-uni
syn keyword txl_keyword contained hash-update hash-update-1 hash-values hashp
syn keyword txl_keyword contained have html-decode html-encode iapply
syn keyword txl_keyword contained identity ido if iff
syn keyword txl_keyword contained iffi iflet ignerr in
syn keyword txl_keyword contained inc inhash int-flo int-str
syn keyword txl_keyword contained integerp intern interp-fun-p interpose
syn keyword txl_keyword contained ip ipf isqrt itimer-prov
syn keyword txl_keyword contained itimer-real itimer-virtual juxt keep-if
syn keyword txl_keyword contained keep-if* keywordp kill labels
syn keyword txl_keyword contained lambda last lazy-str lazy-str-force
syn keyword txl_keyword contained lazy-str-force-upto lazy-str-get-trailing-list lazy-stream-cons lazy-stringp
syn keyword txl_keyword contained lbind lcm lcons lcons-fun
syn keyword txl_keyword contained lconsp ldiff length length-list
syn keyword txl_keyword contained length-str length-str-< length-str-<= length-str->
syn keyword txl_keyword contained length-str->= length-vec lequal less
syn keyword txl_keyword contained let let* lexical-fun-p lexical-lisp1-binding
syn keyword txl_keyword contained lexical-var-p link lisp-parse list
syn keyword txl_keyword contained list* list-str list-vector listp
syn keyword txl_keyword contained load log log-alert log-auth
syn keyword txl_keyword contained log-authpriv log-cons log-crit log-daemon
syn keyword txl_keyword contained log-debug log-emerg log-err log-info
syn keyword txl_keyword contained log-ndelay log-notice log-nowait log-odelay
syn keyword txl_keyword contained log-perror log-pid log-user log-warning
syn keyword txl_keyword contained log10 log2 logand logior
syn keyword txl_keyword contained lognot logtest logtrunc logxor
syn keyword txl_keyword contained macro-form-p macro-time macroexpand macroexpand-1
syn keyword txl_keyword contained macrolet major make-catenated-stream make-env
syn keyword txl_keyword contained make-hash make-lazy-cons make-like make-package
syn keyword txl_keyword contained make-random-state make-similar-hash make-string-byte-input-stream make-string-input-stream
syn keyword txl_keyword contained make-string-output-stream make-strlist-output-stream make-sym make-time
syn keyword txl_keyword contained make-time-utc make-trie makedev makunbound
syn keyword txl_keyword contained mapcar mapcar* mapdo mapf
syn keyword txl_keyword contained maphash mappend mappend* mask
syn keyword txl_keyword contained match-fun match-regex match-regex-right match-regst
syn keyword txl_keyword contained match-regst-right match-str match-str-tree max
syn keyword txl_keyword contained member member-if memq memql
syn keyword txl_keyword contained memqual merge min minor
syn keyword txl_keyword contained minusp mkdir mknod mkstring
syn keyword txl_keyword contained mlet mod multi multi-sort
syn keyword txl_keyword contained n-choose-k n-perm-k nconc nilf
syn keyword txl_keyword contained none not notf nreverse
syn keyword txl_keyword contained null nullify num-chr num-str
syn keyword txl_keyword contained numberp oand oddp op
syn keyword txl_keyword contained open-command open-directory open-file open-fileno
syn keyword txl_keyword contained open-files open-files* open-pipe open-process
syn keyword txl_keyword contained open-tail openlog opip or
syn keyword txl_keyword contained orf packagep pad partition
syn keyword txl_keyword contained partition* partition-by perm pipe
syn keyword txl_keyword contained plusp poll poll-err poll-in
syn keyword txl_keyword contained poll-nval poll-out poll-pri poll-rdband
syn keyword txl_keyword contained poll-rdhup poll-wrband pop pos
syn keyword txl_keyword contained pos-if pos-max pos-min posq
syn keyword txl_keyword contained posql posqual pppred ppred
syn keyword txl_keyword contained pprinl pprint pprof pred
syn keyword txl_keyword contained prinl print prof prog1
syn keyword txl_keyword contained progn prop proper-listp pset
syn keyword txl_keyword contained push pushhash pushnew put-byte
syn keyword txl_keyword contained put-char put-line put-lines put-string
syn keyword txl_keyword contained put-strings pwd qquote quasi
syn keyword txl_keyword contained quasilist quote rand random
syn keyword txl_keyword contained random-fixnum random-state-p range range*
syn keyword txl_keyword contained range-regex rcomb read readlink
syn keyword txl_keyword contained real-time-stream-p reduce-left reduce-right ref
syn keyword txl_keyword contained refset regex-compile regex-parse regexp
syn keyword txl_keyword contained regsub rehome-sym remhash remove-if
syn keyword txl_keyword contained remove-if* remove-path remq remq*
syn keyword txl_keyword contained remql remql* remqual remqual*
syn keyword txl_keyword contained rename-path repeat replace replace-list
syn keyword txl_keyword contained replace-str replace-vec rest ret
syn keyword txl_keyword contained retf return return-from reverse
syn keyword txl_keyword contained rlcp rlet rotate rperm
syn keyword txl_keyword contained rplaca rplacd run s-ifblk
syn keyword txl_keyword contained s-ifchr s-ifdir s-ififo s-iflnk
syn keyword txl_keyword contained s-ifmt s-ifreg s-ifsock s-irgrp
syn keyword txl_keyword contained s-iroth s-irusr s-irwxg s-irwxo
syn keyword txl_keyword contained s-irwxu s-isgid s-isuid s-isvtx
syn keyword txl_keyword contained s-iwgrp s-iwoth s-iwusr s-ixgrp
syn keyword txl_keyword contained s-ixoth s-ixusr search search-regex
syn keyword txl_keyword contained search-regst search-str search-str-tree second
syn keyword txl_keyword contained seek-stream select seqp set
syn keyword txl_keyword contained set-diff set-hash-userdata set-sig-handler setenv
syn keyword txl_keyword contained sethash setitimer setlogmask setq
syn keyword txl_keyword contained sh shift sig-abrt sig-alrm
syn keyword txl_keyword contained sig-bus sig-check sig-chld sig-cont
syn keyword txl_keyword contained sig-fpe sig-hup sig-ill sig-int
syn keyword txl_keyword contained sig-io sig-iot sig-kill sig-lost
syn keyword txl_keyword contained sig-pipe sig-poll sig-prof sig-pwr
syn keyword txl_keyword contained sig-quit sig-segv sig-stkflt sig-stop
syn keyword txl_keyword contained sig-sys sig-term sig-trap sig-tstp
syn keyword txl_keyword contained sig-ttin sig-ttou sig-urg sig-usr1
syn keyword txl_keyword contained sig-usr2 sig-vtalrm sig-winch sig-xcpu
syn keyword txl_keyword contained sig-xfsz sign-extend sin sixth
syn keyword txl_keyword contained size-vec some sort sort-group
syn keyword txl_keyword contained source-loc source-loc-str span-str splice
syn keyword txl_keyword contained split-str split-str-set sqrt sssucc
syn keyword txl_keyword contained ssucc stat stdlib str<
syn keyword txl_keyword contained str<= str= str> str>=
syn keyword txl_keyword contained stream-get-prop stream-set-prop streamp string-extend
syn keyword txl_keyword contained string-lt stringp sub sub-list
syn keyword txl_keyword contained sub-str sub-vec succ swap
syn keyword txl_keyword contained symacrolet symbol-function symbol-name symbol-package
syn keyword txl_keyword contained symbol-value symbolp symlink sys-qquote
syn keyword txl_keyword contained sys-splice sys-unquote sys:*lisp1* sys:eval-err
syn keyword txl_keyword contained sys:get-fb sys:get-vb sys:l1-setq sys:l1-val
syn keyword txl_keyword contained sys:sym-clobber-expander sys:sym-delete-expander sys:sym-update-expander syslog
syn keyword txl_keyword contained tan tb tc tf
syn keyword txl_keyword contained third throw throwf time
syn keyword txl_keyword contained time-fields-local time-fields-utc time-string-local time-string-utc
syn keyword txl_keyword contained time-usec tofloat toint tok-str
syn keyword txl_keyword contained tok-where tostring tostringp tprint
syn keyword txl_keyword contained transpose tree-bind tree-case tree-find
syn keyword txl_keyword contained trie-add trie-compress trie-lookup-begin trie-lookup-feed-char
syn keyword txl_keyword contained trie-value-at trim-str true trunc
syn keyword txl_keyword contained trunc-rem tuples txr-case txr-if
syn keyword txl_keyword contained txr-when typeof unget-byte unget-char
syn keyword txl_keyword contained uniq unique unless unquote
syn keyword txl_keyword contained unsetenv until until* upcase-str
syn keyword txl_keyword contained update url-decode url-encode usleep
syn keyword txl_keyword contained uw-protect vec vec-push vec-set-length
syn keyword txl_keyword contained vecref vector vector-list vectorp
syn keyword txl_keyword contained w-continued w-coredump w-exitstatus w-ifcontinued
syn keyword txl_keyword contained w-ifexited w-ifsignaled w-ifstopped w-nohang
syn keyword txl_keyword contained w-stopsig w-termsig w-untraced wait
syn keyword txl_keyword contained weave when whenlet where
syn keyword txl_keyword contained while while* whilet width
syn keyword txl_keyword contained with-clobber-expander with-delete-expander with-gensyms with-saved-vars
syn keyword txl_keyword contained with-update-expander wrap wrap* zap
syn keyword txl_keyword contained zerop zip
syn match txr_metanum "@[0-9]\+"
syn match txr_nested_error "[^\t `]\+" contained

syn match txr_chr "#\\x[A-Fa-f0-9]\+"
syn match txr_chr "#\\o[0-9]\+"
syn match txr_chr "#\\[^ \t\nA-Za-z0-9_]"
syn match txr_chr "#\\[A-Za-z0-9_]\+"
syn match txr_ncomment ";.*"

syn match txr_dot "\." contained
syn match txr_num "#x[+\-]\?[0-9A-Fa-f]\+"
syn match txr_num "#o[+\-]\?[0-7]\+"
syn match txr_num "#b[+\-]\?[0-1]\+"
syn match txr_ident "[A-Za-z0-9!$%&*+\-<=>?\\_~]*[A-Za-z!$#%&*+\-<=>?\\^_~][A-Za-z0-9!$#%&*+\-<=>?\\^_~]*" contained
syn match txl_ident "[:@][A-Za-z0-9!$%&*+\-<=>?\\\^_~/]\+"
syn match txr_braced_ident "[:][A-Za-z0-9!$%&*+\-<=>?\\\^_~/]\+" contained
syn match txl_ident "[A-Za-z0-9!$%&*+\-<=>?\\_~/]*[A-Za-z!$#%&*+\-<=>?\\^_~/][A-Za-z0-9!$#%&*+\-<=>?\\^_~/]*"
syn match txr_num "[+\-]\?[0-9]\+\([^A-Za-z0-9!$#%&*+\-<=>?\\^_~/]\|\n\)"me=e-1
syn match txr_badnum "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?[A-Za-z!$#%&*+\-<=>?\\^_~/]\+"
syn match txr_num "[+\-]\?[0-9]*[.][0-9]\+\([eE][+\-]\?[0-9]\+\)\?\([^A-Za-z0-9!$#%&*+\-<=>?\\^_~/]\|\n\)"me=e-1
syn match txr_num "[+\-]\?[0-9]\+\([eE][+\-]\?[0-9]\+\)\([^A-Za-z0-9!$#%&*+\-<=>?\\^_~/]\|\n\)"me=e-1
syn match txl_ident ":"
syn match txl_splice "[ \t,]\|,[*]"

syn match txr_unquote "," contained
syn match txr_splice ",\*" contained
syn match txr_quote "'" contained
syn match txr_quote "\^" contained
syn match txr_dotdot "\.\." contained
syn match txr_metaat "@" contained

syn region txr_list matchgroup=Delimiter start="#\?H\?(" matchgroup=Delimiter end=")" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_bracket matchgroup=Delimiter start="\[" matchgroup=Delimiter end="\]" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_mlist matchgroup=Delimiter start="@[ \t]*(" matchgroup=Delimiter end=")" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_mbracket matchgroup=Delimiter start="@[ \t]*\[" matchgroup=Delimiter end="\]" contains=txl_keyword,txr_string,txl_regex,txr_num,txr_badnum,txl_ident,txr_metanum,txr_list,txr_bracket,txr_mlist,txr_mbracket,txr_quasilit,txr_chr,txr_quote,txr_unquote,txr_splice,txr_dot,txr_dotdot,txr_metaat,txr_ncomment,txr_nested_error
syn region txr_string start=+#\?\*\?"+ skip=+\\\\\|\\"\|\\\n+ end=+"\|\n+
syn region txr_quasilit start=+#\?\*\?`+ skip=+\\\\\|\\`\|\\\n+ end=+`\|\n+ contains=txr_splicevar,txr_metanum,txr_bracevar,txr_mlist,txr_mbracket
syn region txr_regex start="/" skip="\\\\\|\\/\|\\\n" end="/\|\n"
syn region txl_regex start="#/" skip="\\\\\|\\/\|\\\n" end="/\|\n"

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
