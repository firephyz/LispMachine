#lang racket

(define (parse-string s)
  ; Turn the string into the corresponding list structure
  (define (helper index result stack top)
    (if (= index (string-length s))
        (convert-mpair (mcar result))
        (cond
          ; The character is a opening parens
          ((eq? (string-ref s index) #\()
           (if (eq? (string-ref s (+ index 1)) #\))
               ; Do special things if we encounter the empty list
               (begin (set-mcar! top '())
                      (helper (+ index 2) result stack top))
               ; Otherwise
               (let ((new-cell (get-new-cell)))
                 (set-mcar! top new-cell)
                 (helper (+ index 1) result (cons new-cell stack) new-cell))))
          ; The character is a closing parens
          ((eq? (string-ref s index) #\))
           (set-mcdr! top '())
           (helper (+ index 1) result (cdr stack) (cadr stack)))
          ; The character is a symbol delimiter
          ((eq? (string-ref s index) #\space)
           (if (eq? (string-ref s (+ index 1)) #\.)
               ; The character is a period
               (let ((symbol-data (parse-symbol (substring s (+ index 3)))))
                 (set-mcdr! top (car symbol-data))
                 ; We add an additional 3 to the index to account for the ". " and ")";
                 (helper (+ index (cdr symbol-data) 4) result (cdr stack) (cadr stack)))
               ; The character is a space
               (let ((new-cell (get-new-cell)))
                 (set-mcdr! top new-cell)
                 (helper (+ index 1) result (cons new-cell (cdr stack)) new-cell))))
          ; The character is the start of a symbol
          (else
           (let ((symbol-data (parse-symbol (substring s index))))
             (set-mcar! top (car symbol-data))
             (helper (+ index (cdr symbol-data)) result stack top))))))
  
  ; Return the next symbol in the given string
  (define (parse-symbol s)
    (define (helper index)
      (if (or (= index (string-length s))
              (eq? (string-ref s index) #\space)
              (eq? (string-ref s index) #\))
              (eq? (string-ref s index) #\.))
          (cons (string->symbol (substring s 0 index)) index)
          (helper (+ index 1))))
    (helper 0))

  ; Returns a new cell
  (define cell-count 0)
  (define (get-new-cell)
    (set! cell-count (+ cell-count 1))
    (mcons '_ '_))
  
  ; Turn all the mpairs in a given list into pairs
  (define (convert-mpair list)
    (if (mpair? list)
        (cons (convert-mpair (mcar list))
              (convert-mpair (mcdr list)))
        list))
  
  ; Start the parsing
  (let* ((start (mcons '_ '_))
         (result (helper 0 start (cons start '()) start)))
    (printf "~a~n" cell-count)
    result))

;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Recursive Version  ;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;
(define (parse-string-r s)
  (define (helper char s)
    (case char
      [(#\() (if (eq? (string-ref s 0) #\))
                 (cons '() 2)
                 (descend-and-gather char s))]
      [(#\)) (cons '() 1)]
      [(#\space) (if (eq? (string-ref s 0) #\.)
                     (let ((result (helper (string-ref s 2) (substring s 3))))
                       (cons (car result)
                             (+ 4 (cdr result))))
                     (descend-and-gather char s))]
      [(#\.) '()]
      [else (parse-symbol (string-append (string char) s))]))
  
  ; Recursively descend along the string and gather the results into a tree
  (define (descend-and-gather char s)
    (let* ((result0 (helper (string-ref s 0) (substring s 1)))
           (result1 (helper (string-ref s (cdr result0))
                            (substring s (+ 1 (cdr result0))))))
      (cons (cons (car result0)
                  (car result1))
            (+ (cdr result0) (cdr result1) 1))))
  
  ; Return the next symbol in the given string
  (define (parse-symbol s)
    (define (helper index)
      (if (or (= index (string-length s))
              (eq? (string-ref s index) #\space)
              (eq? (string-ref s index) #\))
              (eq? (string-ref s index) #\.))
          (cons (string->symbol (substring s 0 index)) index)
          (helper (+ index 1))))
    (helper 0))

  ; Start the parsing
  (car (helper (string-ref s 0) (substring s 1))))

(define (print-list list)
  (if (pair? list)
      (let ((string0 (print-list (car list))))
        (if (null? (cdr list))
            string0
            (let ((string1 (print-list (cdr list))))
              (if (and (not (pair? (car list))) (not (pair? (cdr list))))
                  (string-append "(" string0 " . " string1 ")")
                  (string-append "(" string0 " " string1 ")")))))
        (if (null? list)
            "()"
            (symbol->string list))))

;(parse-string "(c (a . b) d (testing (b . d)))")
;(parse-string "(define (convert-mpair list) (if (mpair? list) (cons (convert-mpair (mcar list)) (convert-mpair (mcdr list))) list))")
;(parse-string-r "(a () (b . ()))")
;(parse-string-r "(define (convert-mpair list) (if (mpair? list) (cons (convert-mpair (mcar list)) (convert-mpair (mcdr list))) list))")
(print-list '(a b c))
