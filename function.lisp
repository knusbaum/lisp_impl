(fn process-args (args)
    (collecting c
              (let ((optional nil))
                (mapcar (arg args)
                        (if (not optional)
                            (if (not (eq arg '&optional))
                                (collect c arg)
                                (progn
                                  (set optional t)
                                  (collect c '&rest)
                                  (collect c 'rest))))))))

(fn get-optionals (args)
  (collecting c
              (let ((optional nil))
                (mapcar (arg args)
                        (if (not optional)
                            (if (eq arg '&optional)
                                (set optional t))
                            (progn
                              (collect c arg)))))))

(fn optional-bindings (args)
    (let ((opt-args (get-optionals args)))
      (mapcar (arg opt-args)
              `(,arg (let ((v (car rest)))
                       (set rest (cdr rest))
                       v)))))

(defmacro defun (name args &rest body)
  `(fn ,name ,(process-args args)
       (let ,(optional-bindings args)
         ,@body)))
