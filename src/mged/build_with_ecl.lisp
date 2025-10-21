;;; Native ECL compilation script for MGED Lisp module
;;; This script uses ECL's native compilation instead of ASDF

;; Load the ECL compiler module (provides compile-file and c:build-static-library)
(require 'cmp)

;; Compile all Lisp files to object files
(format t "~&Compiling Lisp files...~%")

;; Compile each Lisp file to object files
(compile-file "mged_packages.lisp" :system-p t)
(compile-file "mged_init.lisp" :system-p t)
(compile-file "mged_commands.lisp" :system-p t)
(compile-file "mged_math.lisp" :system-p t)
(compile-file "mged_api.lisp" :system-p t)
(compile-file "mged_repl.lisp" :system-p t)

(format t "~&Building static library...~%")

;; Build static library with all object files
(c:build-static-library "mged_api"
                        :lisp-files '("mged_packages.o" 
                                     "mged_init.o"
                                     "mged_commands.o"
                                     "mged_math.o"
                                     "mged_api.o"
                                     "mged_repl.o")
                        :init-name "init_mged_api")

(format t "~&MGED ECL build completed successfully!~%")
(format t "Generated: libmged_api.a~%")
(format t "Initialization function: init_mged_api~%")

;; Clean up object files
(delete-file "mged_packages.o")
(delete-file "mged_init.o")
(delete-file "mged_commands.o")
(delete-file "mged_math.o")
(delete-file "mged_api.o")
(delete-file "mged_repl.o")

;; Exit successfully
(ext:quit 0)
