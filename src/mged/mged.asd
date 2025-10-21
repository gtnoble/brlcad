;;; ASDF system definition for BRL-CAD MGED Lisp modules
;;; This system ensures proper compilation order to resolve symbol dependencies

(defsystem "mged"
  :description "BRL-CAD MGED Lisp modules with ASDF dependency management"
  :version "1.0.0"
  :author "BRL-CAD Developers"
  :licence "BSD"
  :components ((:file "mged_packages")
               (:file "mged_init" :depends-on ("mged_packages"))
               (:file "mged_commands" :depends-on ("mged_packages"))
               (:file "mged_math" :depends-on ("mged_packages"))
               (:file "mged_repl" :depends-on ("mged_packages"))
               (:file "mged_api" :depends-on ("mged_packages" "mged_commands"))))
