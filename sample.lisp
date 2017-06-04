(defmacro progn (&rest body)
  `(let nil
     ,@body))

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

(defmacro or (&rest args)
  (let ((sym (gensym))
        (first-elem (car args))
        (rest (cdr args)))
    `(let ((,sym ,first-elem))
       (if ,sym
           ,sym
           ,(if rest
                `(or ,@rest)
                nil)))))

(defmacro and (&rest args)
  (let ((curr-arg (car args))
        (rest (cdr args)))
    `(if ,(car args)
         ,(if (car (cdr rest))
              `(and ,@rest)
              (car rest)))))

;; The finally form will be executed even if
;; an error is thrown out of the body.
(defmacro unwind-protect (body finally)
  (let ((errsym (gensym)))
    `(progn
       (catch (nil ,errsym)
         ,body
         (progn
           ,finally
           (throw ,errsym)))
       ,finally)))

;; Sample naive fibonacci sequence implementation.
(fn fib (x)
    (cond ((< x 3) 1)
          (t (+ (fib (- x 1))
                (fib (- x 2))))))

;; (fib 40)
