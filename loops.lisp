(defmacro for (init cond step &rest body)
  (let ((conditional (gensym))
        (loop-body (gensym)))
    `(let (,init)
       (tagbody
          (go ,conditional)
          ,loop-body
          ,@body
          ,step
          ,conditional
          (if ,cond
              (go ,loop-body))))))

;(defmacro mapcar (lpair &rest body)
;  (let ((var (car lpair))
;        (list (gensym))
;        (ret (gensym))
;        (rettrack (gensym))
;        (next (gensym)))
;    `(let ((,list ,(car (cdr lpair)))
;           (,ret nil)
;           (,rettrack nil))
;       (for (,var (car ,list)) ,var (progn
;                                      (set ,list (cdr ,list))
;                                      (set ,var (car ,list)))
;            (let ((,next
;                   (progn
;                     ,@body)))
;              (if ,ret
;                  (progn
;                    (setcdr ,rettrack (cons ,next nil))
;                    (set ,rettrack (cdr ,rettrack)))
;                  (progn
;                    (set ,ret (cons ,next nil))
;                    (set ,rettrack ,ret)))))
;       ,ret)))

;; new mapcar with collector
(defmacro mapcar (lpair &rest body)
 (let ((var (car lpair))
       (list (gensym))
       (next-val (gensym))
       (collector (gensym)))
   `(let ((,list ,(car (cdr lpair))))
      (collecting ,collector
        (for (,var (car ,list)) ,var (progn
                                       (set ,list (cdr ,list))
                                       (set ,var (car ,list)))
             (let ((,next-val (progn
                            ,@body)))
               (collect ,collector ,next-val)))))))

(fn fmapcar (lst fn)
  (collecting collector
    (for (x (car lst)) x (progn
                           (set lst (cdr lst))
                           (set x (car lst)))
         (collect collector (funcall fn x)))))
