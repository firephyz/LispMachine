#lang sicp

; car
; cdr
; cons
; eq?
; atom?

; if
; quote
; lambda

; eval
; apply
; evlis
; evif
; lookup
; conenv

; function tagging?
;   - sys func vs user funcs
; 

(define atom?
  (lambda (x)
    (and (not (pair? x)) (not (null? x)))))



(define eval
  (lambda (expr env)
    (if (atom? expr)
        (lookup expr env)
        (cond ((eq? (car expr) (quote if)) (evif (car (cdr expr)) (car (cdr (cdr expr))) (car (cdr (cdr (cdr expr)))) env))
              ((eq? (car expr) (quote quote)) (car (cdr expr)))
              ((eq? (car expr) (quote lambda)) expr)
              (else (apply (car expr) (evlis (cdr expr) env) env))))))

(define apply
  (lambda (func args env)
    (if (atom? func)
        (cond ((primitive? func)
               (cond ((eq? func (quote car)) (car (car args)))
                     ((eq? func (quote cdr)) (cdr (car args)))
                     ((eq? func (quote cons)) (cons (car args) (car (cdr args))))
                     ((eq? func (quote eq?)) (eq? (car args) (car (cdr args))))
                     ((eq? func (quote atom?)) (atom? (car args)))
                     (else (apply (eval func env) args env))))
              ((syscall? func)
               (cond ((eq? func (quote eval)) (quote eval))
                     ((eq? func (quote quit)) (quote quit)))))
        (eval (car (cdr (cdr func))) (conenv (car (cdr func)) args env)))))

(define primitive?
  (lambda (func)
    (or (eq? func (quote car))
        (eq? func (quote cdr))
        (eq? func (quote cons))
        (eq? func (quote eq?))
        (eq? func (quote ?atom)))))

(define syscall?
  (lambda (func)
    (or (eq? func (quote eval))
        (eq? func (quote quit)))))

(define evlis
  (lambda (args env)
    (if (eq? args (quote ()))
        '()
        (cons (eval (car args) env)
              (evlis (cdr args) env)))))

(define evif
  (lambda (pred then else env)
    (if (eval pred env)
        (eval then env)
        (eval else env))))

(define conenv
  (lambda (vars args env)
    (if (eq? vars (quote ()))
        env
        (cons (cons (car vars) (car args))
              (conenv (cdr vars) (cdr args) env)))))

(define lookup
  (lambda (var env)
    (if (eq? (car (car env)) var)
        (cdr (car env))
        (lookup var (cdr env)))))