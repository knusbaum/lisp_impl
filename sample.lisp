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

