(defmacro cond (args)
  (let ((cond-pair (car args))
        (rest (cdr args)))
    (let ((a (car cond-pair))
          (b (car (cdr cond-pair))))
      `(if ,a
           ,b
           ,(if rest
                `(cond ,rest)
                nil)))))

(fn fib (x)
    (cond (((= x 1) 1)
           ((= x 2) 1)
           (t (+ (fib (- x 1))
                 (fib (- x 2)))))))

