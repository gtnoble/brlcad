;;; MGED Package Definitions
;;;;
;;;; This file contains all package definitions for MGED Lisp modules.
;;;; By extracting packages to a separate file, we ensure all symbols exist
;;;; before other files try to reference them, resolving compilation order issues.

(in-package :cl-user)

;;; Create MGED package at top-level so it exists before any code references it
;;; This package will be dynamically populated with command exports at runtime
(defpackage :mged
  (:use :cl)
  ;; Shadow commands that conflict with Common Lisp built-ins
  (:shadow #:debug #:get #:set #:time #:search #:sleep #:push #:t)
  ;; Export essential commands that other files need at compile time
  (:export 
   ;; Database commands
   #:opendb #:closedb
   
   ;; Object management commands  
   #:ls #:draw #:erase #:Z #:exists #:attr #:get
   #:in #:c #:mater #:r #:g #:cp #:mv #:kill
   #:killall #:killtree #:tra #:center #:rot #:sca #:rt
   #:put #:adjust #:arot #:scale #:translate #:rotate
   
   ;; View commands
   #:ae #:zoom #:autoview #:viewsize
   
   ;; Utility commands
   #:who #:units #:title #:tolerance #:region #:color
   ;; Add more exports as needed during development
   ))

;;; Define the high-level API package
(defpackage :mged-api
  (:use :cl)
  (:documentation "High-level, idiomatic Lisp API for MGED commands.")
  (:export 
   ;; Database Management
   #:create-database
   #:open-database
   #:close-database
   #:get-current-database
   #:database-exists-p
   #:ensure-database
   #:with-database
   
   ;; Listing & Queries
   #:list-objects
   #:object-exists-p
   #:get-object-info
   
   ;; Display Operations
   #:draw-objects
   #:erase-objects
   #:clear-display
   
   ;; Primitive Creation
   #:make-sphere
   #:make-ellipsoid
   #:make-cylinder
   #:make-cone
   #:make-box
   #:make-arb8
   #:make-torus
   #:make-particle
   #:create-primitive
   
   ;; Combinations & Regions
   #:make-combination
   #:make-region
   #:make-group
   
   ;; Object Manipulation
   #:copy-object
   #:move-object
   #:kill-objects
   #:kill-all-objects
   #:kill-object-tree
   
   ;; Transformations
   #:translate-object
   #:rotate-object
   #:scale-object
   
   ;; Raytracing
   #:raytrace
   #:quick-preview
   #:high-quality-render
   
   ;; Attribute Management
   #:get-attributes
   #:set-attributes
   #:remove-attributes
   #:append-attributes
   #:list-attribute-types
   #:show-attributes
   #:sort-attributes
   #:copy-attribute
   
   ;; Utilities
   #:vector->string
   #:color->string))

;;; Define the math package
(defpackage :mged-math
  (:use :cl)
  (:documentation "Mathematical operations for MGED, focusing on 3D vector arithmetic.")
  (:export 
   ;; Core vector operations
   #:v+ #:v- #:v* #:v/
   #:vector-add #:vector-subtract #:vector-scale #:vector-divide
   #:magnitude #:normalize #:dot #:cross
   
   ;; Vector constructors
   #:vec #:make-vector
   
   ;; Basic utilities
   #:vector-p #:distance #:angle))

;;;; ============================================================================
;;;; Package Setup Functions
;;;; ============================================================================

(in-package :mged)

;;; Helper function to sanitize command names for Lisp
(defun sanitize-command-name (name)
  "Convert command name to valid Lisp symbol name.
Converts to uppercase and replaces commas with hyphens."
  (string-upcase
   (substitute #\- #\, name)))

;;; Helper function to export command names from MGED package
(defun export-mged-commands (command-names)
  "Export all command symbols from MGED package.
COMMAND-NAMES is a list of command name strings."
  (let ((symbols (mapcar (lambda (name)
                          (intern (sanitize-command-name name) :mged))
                        command-names)))
    (export symbols :mged)))

;;; Read cmdtab entries using C helper functions
(defun read-cmdtab-entries ()
  "Read command table entries using C helper functions.
Returns a list of (name . is-custom) pairs."
  (loop for i from 0 below (cmdtab-count)
        for name = (cmdtab-get-name i)
        for is-custom = (cmdtab-is-custom i)
        collect (cons name is-custom)))

;;; Register all MGED commands
(defun register-mged-commands (command-list state-ptr)
  "Register MGED commands as ECL functions.
COMMAND-LIST is a list of (name . is-custom) pairs, where:
  - NAME is the command name string
  - IS-CUSTOM is T for custom commands (use cmd dispatcher), NIL for ged_exec commands
STATE-PTR is the pointer to mged_state (as integer)."
  
  ;; Export all command names from MGED package
  (let ((command-names (mapcar #'car command-list)))
    (export-mged-commands command-names))
  
  ;; Store the state in MGED package
  (setf (symbol-value (intern "*MGED-STATE*" :mged)) state-ptr)
  
  ;; Register each command with appropriate dispatcher
  (let ((count 0))
    (dolist (cmd-desc command-list)
      (let* ((original-name (car cmd-desc))
             (is-custom (cdr cmd-desc))
             (lisp-name (intern (sanitize-command-name original-name) :mged))
             (dispatcher (if is-custom
                             'ecl-mged-cmd-dispatcher
                             'ecl-mged-dispatcher)))
        ;; Define the wrapper function
        (eval `(defun ,lisp-name (&rest args)
                 ,(format nil "MGED command: ~A" original-name)
                 (apply #',dispatcher ,original-name args)))
        (incf count)))
    
    ;; Return count of registered commands
    count))

;;; Dispatcher for standard ged_exec commands
(defun ecl-mged-dispatcher (command-name &rest args)
  "Dispatcher for standard MGED commands that use ged_exec.
COMMAND-NAME is the command name string.
ARGS are the command arguments (will be converted to strings)."
  (let* ((args-list (mapcar #'princ-to-string args))
         (result (call-ged-exec command-name args-list)))
    (destructuring-bind (return-code result-string) result
      (if (zerop return-code)
          ;; Success - return result string or NIL
          (if (and result-string (plusp (length result-string)))
              result-string
              nil)
          ;; Error - signal MGED-ERROR condition
          (error 'mged-error
                 :command command-name
                 :message result-string
                 :return-code return-code)))))

;;; Dispatcher for custom MGED commands that use tcl_func
(defun ecl-mged-cmd-dispatcher (command-name &rest args)
  "Dispatcher for custom MGED commands that use tcl_func.
COMMAND-NAME is the command name string.
ARGS are the command arguments (will be converted to strings)."
  (let* ((args-list (mapcar #'princ-to-string args))
         (result (call-tcl-func command-name args-list)))
    (destructuring-bind (return-code result-string) result
      ;; TCL_OK = 0, TCL_ERROR = 1
      (if (zerop return-code)
          ;; Success - return result string or NIL
          (if (and result-string (plusp (length result-string)))
              result-string
              nil)
          ;; Error - signal MGED-ERROR condition
          (error 'mged-error
                 :command command-name
                 :message result-string
                 :return-code return-code)))))

;;; Main setup function called from C  
(defun setup-mged-ecl (state-ptr)
  "Set up MGED ECL environment with command registration.
STATE-PTR is a pointer to mged_state (as integer).
Returns the number of commands registered."
  
  ;; Read command table entries using C helper functions
  (let ((command-list (read-cmdtab-entries)))
    ;; Register all commands (this also stores state-ptr after package creation)
    (register-mged-commands command-list state-ptr)))

;;; Define MGED-ERROR condition type for command failures
(define-condition mged-error (error)
  ((command :initarg :command :reader mged-error-command)
   (message :initarg :message :reader mged-error-message)
   (return-code :initarg :return-code :reader mged-error-return-code))
  (:report (lambda (condition stream)
             (format stream "MGED command '~A' failed: ~A"
                     (mged-error-command condition)
                     (mged-error-message condition)))))
