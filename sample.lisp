(defmacro cond (&rest args)
  (let ((cond-pair (car args))
        (rest (cdr args)))
    (let ((a (car cond-pair))
          (b (car (cdr cond-pair))))
      `(if ,a
           ,b
           ,(if rest
                `(cond ,@rest)
                nil)))))

(fn fib (x)
    (cond ((< x 3) 1)
          (t (+ (fib (- x 1))
                (fib (- x 2))))))




(defmacro and (&rest args)
  (let ((first (car args))
        (rest (cdr args)))
    (if rest
        `(if ,first
             (and ,@rest)
             nil)
        first)))

(fib 40)
