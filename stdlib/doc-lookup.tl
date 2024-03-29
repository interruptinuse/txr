(defvar usr:*doc-url* "https://www.nongnu.org/txr/txr-manpage.html")

(defvarl os-symbol
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
            (iff (f^ #/OpenBSD/) (ret :openbsd))
            (ret :unknown))
       u.sysname])))

(defun detached-run (program args)
  (match-case (fork)
    (@(= 0) (if (zerop (fork))
              (exit* (let ((*stdout* *stdnull*))
                       (run program args)))
              (exit* 0)))
    (@(> 0) (error "fork failed"))))

(caseql os-symbol
  ((:linux :macos :openbsd :solaris :solaris10 :android)
   (defun open-url (url)
     (let ((opener (caseql os-symbol
                     ((:linux :openbsd :solaris :android) "xdg-open")
                     (:solaris10 "/usr/dt/bin/sdtwebclient")
                     (:macos "open")))
           (fallback '#"firefox iceweasel seamonkey mozilla \
                        epiphany konqueror chromium chromium-browser \
                        google-chrome"))
       (iflet ((prog (getenv "BROWSER"))
               (prog (or (and (plusp (len prog)) prog)
                         (if (abs-path-p opener)
                           opener
                           (path-search opener))
                         [find-true path-search fallback])))
         (detached-run prog (list url))
         (error "~s: no URL-opening method available" 'open-url)))))
  ((:cygwin :cygnal)
   (with-dyn-lib "shell32.dll"
     (deffi shell-execute "ShellExecuteW"
            cptr (cptr wstr wstr wstr wstr int))
     (defun open-url (url)
       (let ((hinst (shell-execute cptr-null "open" url nil nil 0)))
         (if (> (int-cptr hinst) 32)
           t
           (error `~s: failed to open ~s` 'open-url url))))))
  (t (defun open-url (url)
       (ignore url)
       (error "~s: not implemented" 'open-url))))

(defun usr:doc (: sym)
  (iflet ((str (typecase sym
                 (null sym)
                 (sym (let ((*package* (find-package "pub")))
                        (tostring sym)))
                 (t (tostringp sym))))
          (tag (if str (fmt "S-~,08X" (crc32 str)) "")))
    (open-url `@{*doc-url*}#@tag`)
    (error "~s: ~s not found in symbol index" 'doc sym)))
