(load "doc-syms")

(defvar usr:*doc-url* "https://www.nongnu.org/txr/txr-manpage.html")

(defvar os-symbol
  (if (ignerr (dlsym (dlopen "libandroid.so") "AAsset_close"))
    :android
    (let ((u (uname)))
      [(orf (iff (f^ #/Linux/) (ret :linux))
            (iff (f^ #/SunOS/)
                 (ret (if (<= 5 (int-str u.release))
                        :solaris10
                        :solaris)))
            (iff (f^ #/CYGWIN/) (ret :cygwin))
            (iff (f^ #/CYGNAL/) (ret :cygnal))
            (iff (f^ #/Darwin/) (ret :macos))
            (ret :unknown))
       u.sysname])))

(defun detached-run (program args)
  (match-case (fork)
    (@(= 0) (if (zerop (fork))
              (exit* (let ((*stdout* *stdnull*))
                       (run program args)))
              (exit* 0)))
    (@(< @res 0) (error "fork failed"))))

(caseql os-symbol
  ((:linux :macos :solaris :solaris10 :android)
   (defun open-url (url)
     (let ((opener (caseql os-symbol
                     ((:linux :solaris :android) "xdg-open")
                     (:solaris10 "/usr/dt/bin/sdtwebclient")
                     (:macos "open")))
           (fallback '#"firefox iceweasel seamonkey mozilla \
                        epiphany konqueror chromium chromium-browser \
                        google-chrome"))
       (let ((prog (or (getenv "BROWSER")
                       (if (abs-path-p opener)
                         opener
                         (path-search opener))
                       [find-true path-search fallback])))
         (if prog
           (detached-run prog (list url))
           (error "~s: no URL-opening method available" 'open-url))))))
  ((:cygwin :cygnal)
   (with-dyn-lib "shell32.dll"
     (deffi shell-execute "ShellExecuteW"
            cptr (cptr wstr wstr wstr wstr int))
     (defun open-url (url)
       (let ((hinst (shell-execute cptr-null "open" url nil nil 0)))
         (if (> (int-cptr hinst) 32)
           t
           (error `~s: failed to open ~s` 'open-url url))))))
  (t (defun open-url (url) (error "~s: not implemented" 'open-url))))

(defun usr:doc (: sym)
  (iflet ((str (typecase sym
                 (null sym)
                 (sym (let ((*package* (find-package "pub")))
                        (tostring sym)))
                 (t (tostringp sym))))
          (tag (if str [doc-syms str] "")))
    (open-url `@{*doc-url*}#@tag`)
    (error "~s: ~s not found in symbol index" 'doc sym)))