;;; ASDF-based build script for MGED Lisp module
;;; This script replaces the manual compilation approach with ASDF dependency management

;; Ensure ASDF is loaded and configured
(require :asdf)

;; Configure ASDF to find our system
(push (make-pathname :directory (pathname-directory *load-truename*)) asdf:*central-registry*)

;; Load the MGED system
(asdf:load-system :mged)

;; Build the static library using ECL's asdf:make-build
;; This creates libmged_api.a with all dependencies included
(asdf:make-build :mged
                 :type :static-library
                 :move-here #P"./"
                 :monolithic t
                 :init-name "init_mged_api")

(format t "~&MGED ASDF build completed successfully!~%")
(format t "Generated: libmged_api.a~%")
(format t "Initialization function: init_mged_api~%")
