(load "../common")

(mtest
  (split-str "" "") ("")
  (split-str "a" "") ("a")
  (split-str "" "a") ("")
  (split-str "a" "a") ("" ""))

(mtest
  (split-str "" "" t) ("")
  (split-str "a" "" t) ("a")
  (split-str "" "a" t) ("")
  (split-str "a" "a" t) ("" "a" ""))

(mtest
  (split-str "" #//) ("")
  (split-str "a" #//) ("a")
  (split-str "" #/a/) ("")
  (split-str "a" #/a/) ("" ""))

(mtest
  (split-str "" #// t) ("")
  (split-str "a" #// t) ("a")
  (split-str "" #/a/ t) ("")
  (split-str "a" #/a/ t) ("" "a" ""))

(mtest
  (split-str "ab" "") ("a" "b")
  (split-str "ab" "a") ("" "b")
  (split-str "ab" "b") ("a" "")
  (split-str "ab" "ab") ("" "")
  (split-str "ab" "abc") ("ab"))

(mtest
  (split-str "ab" "" t) ("a" "" "b")
  (split-str "ab" "a" t) ("" "a" "b")
  (split-str "ab" "b" t) ("a" "b" "")
  (split-str "ab" "ab" t) ("" "ab" "")
  (split-str "ab" "abc" t) ("ab"))

(mtest
  (split-str "ab" #//) ("a" "b")
  (split-str "ab" #/a/) ("" "b")
  (split-str "ab" #/b/) ("a" "")
  (split-str "ab" #/ab/) ("" "")
  (split-str "ab" #/abc/) ("ab"))

(mtest
  (split-str "ab" #// t) ("a" "" "b")
  (split-str "ab" #/a/ t) ("" "a" "b")
  (split-str "ab" #/b/ t) ("a" "b" "")
  (split-str "ab" #/ab/ t) ("" "ab" "")
  (split-str "ab" #/abc/ t) ("ab"))

(mtest
  (split-str "abc" "") ("a" "b" "c")
  (split-str "abc" "" t) ("a" "" "b" "" "c")
  (split-str "abc" #//) ("a" "b" "c")
  (split-str "abc" #// t) ("a" "" "b" "" "c"))

(mtest
  (split-str "abc" "a") ("" "bc")
  (split-str "abc" "b") ("a" "c")
  (split-str "abc" "c") ("ab" "")
  (split-str "abc" "a" t) ("" "a" "bc")
  (split-str "abc" "b" t) ("a" "b" "c")
  (split-str "abc" "c" t) ("ab" "c" "")
  (split-str "abc" #/a/) ("" "bc")
  (split-str "abc" #/b/) ("a" "c")
  (split-str "abc" #/c/) ("ab" "")
  (split-str "abc" #/a/ t) ("" "a" "bc")
  (split-str "abc" #/b/ t) ("a" "b" "c")
  (split-str "abc" #/c/ t) ("ab" "c" ""))

(mtest
  (split-str "abc" "ab") ("" "c")
  (split-str "abc" "bc") ("a" "")
  (split-str "abc" "abc") ("" "")
  (split-str "abc" "ab" t) ("" "ab" "c")
  (split-str "abc" "bc" t) ("a" "bc" "")
  (split-str "abc" "abc" t) ("" "abc" "")
  (split-str "abc" #/ab/) ("" "c")
  (split-str "abc" #/bc/) ("a" "")
  (split-str "abc" #/abc/) ("" "")
  (split-str "abc" #/ab/ t) ("" "ab" "c")
  (split-str "abc" #/bc/ t) ("a" "bc" "")
  (split-str "abc" #/abc/ t) ("" "abc" ""))

(mtest
  (split-str "a,b,c" ",") ("a" "b" "c")
  (split-str ",b,c" ",") ("" "b" "c")
  (split-str "a,,c" ",") ("a" "" "c")
  (split-str "a,b," ",") ("a" "b" "")
  (split-str ",,c" ",") ("" "" "c")
  (split-str "a,," ",") ("a" "" "")
  (split-str ",," ",") ("" "" ""))

(mtest
  (split-str "a,b,c" #/,/ t) ("a" "," "b" "," "c")
  (split-str ",b,c" #/,/ t) ("" "," "b" "," "c")
  (split-str "a,,c" #/,/ t) ("a" "," "" "," "c")
  (split-str "a,b," #/,/ t) ("a" "," "b" "," "")
  (split-str ",,c" #/,/ t) ("" "," "" "," "c")
  (split-str "a,," #/,/ t) ("a" "," "" "," "")
  (split-str ",," #/,/ t) ("" "," "" "," ""))

(mtest
  (split-str "a,b,c" #/[^,]/) ("" "," "," "")
  (split-str "a,b,c" #/[^,]/ t) ("" "a" "," "b" "," "c" "")
  (split-str "a,b,c" #/[^a-c]/) #"a b c"
  (split-str "a,b,c" #/[^a-c]/ t) #"a , b , c")

(mtest
  (split-str "abc" #/./) ("" "" "" "")
  (split-str "abc" #/./ t) ("" "a" "" "b" "" "c" "")
  (split-str "abc" #/../) ("" "c")
  (split-str "abc" #/../ t) ("" "ab" "c")
  (split-str "abc" #/.../) ("" "")
  (split-str "abc" #/.../ t) ("" "abc" ""))

(mtest
  (split-str "abcacabcac" #/ab?/) ("" "c" "c" "c" "c")
  (split-str "abcacabcac" #/ab?/ t) ("" "ab" "c" "a" "c" "ab" "c" "a" "c"))

(mtest
  (tok-str "" #//) ("")
  (tok-str "a" #//) ("" "")
  (tok-str "" #/a/) nil
  (tok-str "a" #/a/) ("a"))

(mtest
  (tok-str "" #// t) ("" "" "")
  (tok-str "a" #// t) ("" "" "a" "" "")
  (tok-str "" #/a/ t) ("")
  (tok-str "a" #/a/ t) ("" "a" ""))

(mtest
  (tok-str "ab" #//) ("" "" "")
  (tok-str "ab" #/a/) ("a")
  (tok-str "ab" #/b/) ("b")
  (tok-str "ab" #/ab/) ("ab")
  (tok-str "ab" #/abc/) nil)

(mtest
  (tok-str "ab" #// t) ("" "" "a" "" "b" "" "")
  (tok-str "ab" #/a/ t) ("" "a" "b")
  (tok-str "ab" #/b/ t) ("a" "b" "")
  (tok-str "ab" #/ab/ t) ("" "ab" "")
  (tok-str "ab" #/abc/ t) ("ab"))

(mtest
  (tok-str "abc" #//) ("" "" "" "")
  (tok-str "abc" #// t) ("" "" "a" "" "b" "" "c" "" ""))

(mtest
  (tok-str "abc" #/a/) ("a")
  (tok-str "abc" #/b/) ("b")
  (tok-str "abc" #/c/) ("c")
  (tok-str "abc" #/a/ t) ("" "a" "bc")
  (tok-str "abc" #/b/ t) ("a" "b" "c")
  (tok-str "abc" #/c/ t) ("ab" "c" ""))

(mtest
  (tok-str "abc" #/ab/) ("ab")
  (tok-str "abc" #/bc/) ("bc")
  (tok-str "abc" #/abc/) ("abc")
  (tok-str "abc" #/ab/ t) ("" "ab" "c")
  (tok-str "abc" #/bc/ t) ("a" "bc" "")
  (tok-str "abc" #/abc/ t) ("" "abc" ""))

(mtest
  (tok-str "a,b,c" #/,/ t) ("a" "," "b" "," "c")
  (tok-str ",b,c" #/,/ t) ("" "," "b" "," "c")
  (tok-str "a,,c" #/,/ t) ("a" "," "" "," "c")
  (tok-str "a,b," #/,/ t) ("a" "," "b" "," "")
  (tok-str ",,c" #/,/ t) ("" "," "" "," "c")
  (tok-str "a,," #/,/ t) ("a" "," "" "," "")
  (tok-str ",," #/,/ t) ("" "," "" "," ""))

(mtest
  (tok-str "a,b,c" #/[^,]/) #"a b c"
  (tok-str "a,b,c" #/[^,]/ t) ("" "a" "," "b" "," "c" "")
  (tok-str "a,b,c" #/[^a-c]/) #", ,"
  (tok-str "a,b,c" #/[^a-c]/ t) #"a , b , c")

(mtest
  (tok-str "abc" #/./) #"a b c"
  (tok-str "abc" #/./ t) ("" "a" "" "b" "" "c" "")
  (tok-str "abc" #/../) #"ab"
  (tok-str "abc" #/../ t) ("" "ab" "c")
  (tok-str "abc" #/.../) #"abc"
  (tok-str "abc" #/.../ t) ("" "abc" ""))

(mtest
  (tok-str "abcacabcac" #/ab?/) #"ab a ab a"
  (tok-str "abcacabcac" #/ab?/ t) ("" "ab" "c" "a" "c" "ab" "c" "a" "c"))

(mtest
  (tok-str "" #/[^,]/ : 0) ()
  (tok-str "a,b,c" #/[^,]/ : 0) ("a,b,c")
  (tok-str "a,b,c" #/[^,]/ : 1) ("a" ",b,c")
  (tok-str "a,b,c" #/[^,]/ : 2) ("a" "b" ",c")
  (tok-str "a,b,c" #/[^,]/ : 3) #"a b c"
  (tok-str "a,b,c" #/[^,]/ : 4) #"a b c")

(mtest
  (tok-str "a,b,c" #/[^,]/ t 0) ("a,b,c")
  (tok-str "a,b,c" #/[^,]/ t 1) ("" "a" ",b,c")
  (tok-str "a,b,c" #/[^,]/ t 2) ("" "a" "," "b" ",c")
  (tok-str "a,b,c" #/[^,]/ t 3) ("" "a" "," "b" "," "c" "")
  (tok-str "a,b,c" #/[^,]/ t 4) ("" "a" "," "b" "," "c" ""))

(mtest
  (tok #/[^,]/ "a,b,c") #"a b c"
  (tokn : #/[^,]/ "a,b,c") :error
  (tokn nil #/[^,]/ "a,b,c") :error
  (tokn 0 #/[^,]/ "a,b,c") ("a,b,c")
  (tokn 1 #/[^,]/ "a,b,c") ("a" ",b,c")
  (tokn 2 #/[^,]/ "a,b,c") ("a" "b" ",c")
  (tokn 3 #/[^,]/ "a,b,c") ("a" "b" "c")
  (tokn 4 #/[^,]/ "a,b,c") ("a" "b" "c"))

(mtest
  (tokn 0 #/[^,]/ t "a,b,c") ("a,b,c")
  (tokn 1 #/[^,]/ t "a,b,c") ("" "a" ",b,c")
  (tokn 2 #/[^,]/ t "a,b,c") ("" "a" "," "b" ",c")
  (tokn 3 #/[^,]/ t "a,b,c") ("" "a" "," "b" "," "c" "")
  (tokn 4 #/[^,]/ t "a,b,c") ("" "a" "," "b" "," "c" ""))

(mtest
  (join) ""
  (join "") ""
  (join "" "") ""
  (join #\a) "a"
  (join "a") "a"
  (join #\a "b") "ab"
  (join "a" #\b) "ab"
  (join #\a #\b) "ab"
  (join "a" "b") "ab"
  (join "a" "b" "cde") "abcde")

(mtest
  (join-with #\a) ""
  (join-with #\a #\b) "b"
  (join-with #\a "b") "b"
  (join-with "a") ""
  (join-with "a" #\b) "b"
  (join-with "a" "b") "b"
  (join-with "a" "b") "b"
  (join-with "--" "b" "c" "d") "b--c--d"
  (join-with #\- "b" "c" "d") "b-c-d")

(mtest
  (split-str "abc" "" : 0) ("abc")
  (split-str "abc" "" : 1) ("a" "bc")
  (split-str "abc" "" : 2) ("a" "b" "c")
  (split-str "abc" "" : 3) ("a" "b" "c")
  (split-str "abc" "" : -1) :error)

(mtest
  (split-str "abc" "" t 0) ("abc")
  (split-str "abc" "" t 1) ("a" "" "bc")
  (split-str "abc" "" t 2) ("a" "" "b" "" "c")
  (split-str "abc" "" t 3) ("a" "" "b" "" "c"))

(mtest
  (split-str "a,b,c" "," : 0) ("a,b,c")
  (split-str "a,b,c" "," : 1) ("a" "b,c")
  (split-str "a,b,c" "," : 2) ("a" "b" "c")
  (split-str "a,b,c" "," : 3) ("a" "b" "c"))

(mtest
  (split-str "a,b,c" "," t 0) ("a,b,c")
  (split-str "a,b,c" "," t 1) ("a" "," "b,c")
  (split-str "a,b,c" "," t 2) ("a" "," "b" "," "c")
  (split-str "a,b,c" "," t 3) ("a" "," "b" "," "c"))

(mtest
  (split-str "a12b34c567d" #/[0-9]+/ : 0) ("a12b34c567d")
  (split-str "a12b34c567d" #/[0-9]+/ : 1) ("a" "b34c567d")
  (split-str "a12b34c567d" #/[0-9]+/ : 2) ("a" "b" "c567d")
  (split-str "a12b34c567d" #/[0-9]+/ : 3) ("a" "b" "c" "d")
  (split-str "a12b34c567d" #/[0-9]+/ : 4) ("a" "b" "c" "d"))

(mtest
  (split-str "a12b34c567d" #/[0-9]+/ t 0) ("a12b34c567d")
  (split-str "a12b34c567d" #/[0-9]+/ t 1) ("a" "12" "b34c567d")
  (split-str "a12b34c567d" #/[0-9]+/ t 2) ("a" "12" "b" "34" "c567d")
  (split-str "a12b34c567d" #/[0-9]+/ t 3) ("a" "12" "b" "34" "c" "567" "d")
  (split-str "a12b34c567d" #/[0-9]+/ t 4) ("a" "12" "b" "34" "c" "567" "d"))

(mtest
  (spl "," "a,b,c") #"a b c"
  (spln : "," "a,b,c") :error
  (spln nil "," "a,b,c") :error
  (spln 0 "," "a,b,c") ("a,b,c")
  (spln 1 "," "a,b,c") ("a" "b,c")
  (spln 2 "," "a,b,c") ("a" "b" "c")
  (spln 3 "," "a,b,c") ("a" "b" "c")
  (spln 4 "," "a,b,c") ("a" "b" "c"))

(mtest
  (cat-str '()) ""
  (cat-str '() "-") ""
  (cat-str '(()) "-") ""
  (cat-str '((()) ()) "-") ""
  (cat-str '((()) #()) "-") ""
  (cat-str '((("a" ("b")) #(#\c))) "-") "a-b-c")

(mtest
  (join-with "--" '()) ""
  (join-with "--" '(("b"))) "b"
  (join-with "--" '("b" #(("c") ()) "d")) "b--c--d")
