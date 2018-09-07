(fn reverse (lst)
    (do ((nxt lst (cdr nxt))
         (hd (car nxt) (car nxt))
         (lst2 (cons hd nil) (cons hd lst2)))
        ((eq (cdr nxt) nil) lst2)))


(fn string-to-list (str)
    (let ((len (str-len str))
          (res ()))
      (do ((x 0 (+ x 1)))
          ((or
            (= x len)
            (set res (cons (str-nth str x) res)))
           (reverse res)))))
             
(fn list-to-string (lst)
    (let ((str ""))
      (for (elem (car lst)) elem (progn
                                   (set lst (cdr lst))
                                   (set elem (car lst)))
           (str-append str elem))
      str))
;      
;      (do ((lst-rest lst (cdr lst-rest))
;           (elem (car lst-rest) (car lst-rest)))
;          ((progn
;             (print elem)
;             (str-append str elem)
;             (eq elem nil))
;           str))))
;#1  0x0000000000404352 in new_string_copy (c=0x0) at lstring.c:26

