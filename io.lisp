(fn read-file (fname)
    (let ((f (open fname)))
      (catch ('end-of-file e)
        (do ((form (read f) (read f)))
            ((progn
               (print form)
               (print "\n")
               nil)
             :done))
        (close f))))

;(fn read-file-by-char (fname)
;    (let ((f (open fname))
;          (chrs nil)
;          (chtrack nil))
;      (catch ('end-of-file e)
;        (do ((c (read-char f) (read-char f)))
;            ((progn
;               (if chrs
;                   (progn
;                     (setcdr chtrack (cons c nil))
;                     (set chtrack (cdr chtrack)))
;                   (progn
;                     (set chrs (cons c nil))
;                     (set chtrack chrs)))
;               nil)
;             :done))
;        (progn
;          (close f)
;          chrs))))

;; New read-file-by-char with collector
(fn read-file-by-char (fname)
  (let ((f (open fname)))
    (collecting collector
      (catch ('end-of-file e)
        (do ((c (read-char f) (read-char f)))
            ((progn (collect collector c)
                    nil)
             :done))
        (close f)))))

(fn slurp (fname)
    (let ((f (open fname))
          (str ""))
      (catch ('end-of-file e)
        (for (c (read-char f)) t (set c (read-char f))
             (str-append str c))
        (progn
          (close f)
          str))))
