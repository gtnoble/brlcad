;;;; compile_lisp.lisp
;;;;
;;;; Simple script to compile a Lisp file to a static library with known init function.
;;;; Usage: ecl -load compile_lisp.lisp -- source.lisp output.a

;; Load the ECL compiler module (provides c:build-static-library and friends)
(require 'cmp)

(defun compile-to-library (source-file output-file)
  "Compile SOURCE-FILE to static library OUTPUT-FILE with known init function."
  (format t "~&Compiling ~A to static library ~A~%" source-file output-file)
  
  ;; Step 1: Compile to temporary object file
  (let ((temp-o (make-pathname :type "o" :defaults source-file)))
    (format t "~&  Step 1: Compiling to object file ~A~%" temp-o)
    (compile-file source-file :system-p t :output-file temp-o)
    
    ;; Step 2: Build static library with known init name
    ;; Note: c:build-static-library expects just the library base name (e.g., "mged_repl")
    ;; It automatically adds "lib" prefix and ".a" extension
    ;; So we extract the base name from the output path
    (let* ((output-path (pathname output-file))
           (output-dir (make-pathname :directory (pathname-directory output-path)))
           ;; Get the filename without extension, and strip "lib" prefix if present
           (lib-name (pathname-name output-path))
           (base-name (if (and (> (length lib-name) 3)
                              (string= lib-name "lib" :end1 3))
                         (subseq lib-name 3)  ; Remove "lib" prefix
                         lib-name))
           ;; Generate init function name from base name (e.g., "mged_init" -> "init_mged_init")
           (init-name (concatenate 'string "init_" base-name)))
      (format t "~&  Step 2: Building static library with init function '~A'~%" init-name)
      ;; Build in the current directory (ECL's default)
      (c:build-static-library base-name
                              :lisp-files (list temp-o)
                              :init-name init-name)
      ;; Move the created file to the desired location if needed
      (let ((created-file (make-pathname :name (concatenate 'string "lib" base-name)
                                        :type "a"
                                        :defaults (make-pathname))))
        (unless (equal (truename created-file) (truename output-file))
          (rename-file created-file output-file))))
    
    ;; Step 3: Clean up temporary object file
    (format t "~&  Step 3: Cleaning up temporary object file~%")
    (delete-file temp-o))
  
  (format t "~&Compilation complete.~%"))

;; Get command-line arguments after --
(let ((args (rest (member "--" (ext:command-args) :test #'string=))))
  (unless (= (length args) 2)
    (format *error-output* "Usage: ecl -load compile_lisp.lisp -- source.lisp output.a~%")
    (ext:quit 1))
  
  (let ((source (first args))
        (output (second args)))
    (compile-to-library source output)))

;; Exit successfully
(ext:quit 0)
