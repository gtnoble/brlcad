;;;; MGED ECL Initialization
;;;;
;;;; This file contains initialization code that sets up the ECL environment
;;;; for MGED, including condition types, I/O streams, and convenience functions.
;;;;
;;;; This file is compiled by ECL at build time and linked into the mged binary.

(in-package :cl-user)

;;; Define MGED-ERROR condition type for command failures
(define-condition mged-error (error)
  ((command :initarg :command :reader mged-error-command)
   (message :initarg :message :reader mged-error-message)
   (return-code :initarg :return-code :reader mged-error-return-code))
  (:report (lambda (condition stream)
             (format stream "MGED command '~A' failed: ~A"
                     (mged-error-command condition)
                     (mged-error-message condition)))))

;;; Set up I/O streams for interactive REPL
(defun setup-io-streams ()
  "Configure standard I/O streams for interactive ECL REPL."
  (setf *standard-input* *terminal-io*
        *standard-output* *terminal-io*
        *error-output* *terminal-io*
        *query-io* *terminal-io*
        *debug-io* *terminal-io*))

;;; Define quit and exit functions that call MGED's cleanup
;;; Note: MGED-QUIT is a C function registered by ecl_interface.c
(defun quit (&optional (status 0))
  "Exit MGED with proper cleanup."
  (declare (ignore status))
  (mged-quit))

(defun exit (&optional (status 0))
  "Exit MGED with proper cleanup."
  (declare (ignore status))
  (mged-quit))

;;; Global top-level restart handler
;;; This function can be called from anywhere to return to the MGED REPL
(defun return-to-mged-top-level ()
  "Global function to return to MGED top-level REPL from any error condition."
  (when (find-restart 'top-level-repl)
    (invoke-restart 'top-level-repl))
  (when (find-restart 'abort-to-toplevel)
    (invoke-restart 'abort-to-toplevel))
  ;; If no restarts found, try to recover gracefully
  (format t "~&Returning to MGED top-level...~%")
  (si::tpl-prompt))

;;; Initialize the ECL environment for MGED
(defun init-mged-environment ()
  "Initialize ECL environment - called from C after ECL boot.
Sets up I/O streams, REPL state, and other ECL configuration."
  ;; Set up I/O streams
  (setup-io-streams)
  
  ;; Initialize REPL state variables
  (setq si::*tpl-level* 0
        si::*ihs-base* (si::ihs-top)
        si::*ihs-top* (si::ihs-top)
        si::*ihs-current* (si::ihs-top)
        si::*break-env* nil)
  
  ;; Return success
  t)
