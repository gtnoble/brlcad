;;;; MGED ECL REPL Initialization
;;;;
;;;; This file contains the initialization code for MGED's ECL REPL,
;;;; including command table definitions and the non-blocking REPL step function.
;;;;
;;;; This file is compiled by ECL at build time and linked into the mged binary.

(in-package :cl-user)

;;; Define MGED's custom top-level command table
;;; This will be dynamically bound in mged-repl-step so tpl-read can find it
(defparameter *mged-tpl-commands*
  '(("Top level commands"
     ;; Exit commands - use mged-quit for proper cleanup
     ((:exit :eof) mged-quit nil
      ":exit            Exit MGED"
      ":exit &optional (status 0)                    [Top level command]~@~@
Exit MGED with proper cleanup.~%")
     
     ;; Help commands - delegate to ECL's implementations
     ((? :h :help) si::tpl-help-command nil
      ":h(elp) or ?     Help. Type \":help help\" for more information"
      ":help &optional topic                         [Top level command]~@
:h &optional topic                              [Abbreviation]~@~@
Print information on specified topic.~%")
     
     ((:apropos) si::tpl-apropos-command nil
      ":apropos         Apropos"
      ":apropos string &optional package             [Top level command]~@~@
Finds all available symbols whose print names contain string.~%")
     
     ((:doc :document) si::tpl-document-command nil
      ":doc(ument)      Document"
      ":document symbol                              [Top level command]~@~@
Displays documentation about symbol.~%")
     
     ;; Development commands
     ((:ld :load) si::tpl-load-command :string
      ":ld              Load file"
      ":load &string &rest files                     [Top level command]~@
:ld &string &rest files                         [Abbreviation]~@~@
Load files.~%")
     
     ((:cf :compile-file) si::tpl-compile-command :string
      ":cf              Compile file"
      ":compile-file &string &rest files             [Top level command]~@
:cf &string &rest files                         [Abbreviation]~@~@
Compile files.~%")
     
     ((:tr :trace) si::tpl-trace-command nil
      ":tr(ace)         Trace function"
      ":trace &rest functions                        [Top level command]~@~@
Trace specified functions.~%")
     
     ((:untr :untrace) si::tpl-untrace-command nil
      ":untr(ace)       Untrace function"
      ":untrace &rest functions                      [Top level command]~@~@
Untrace specified functions.~%"))
    
    ("Break commands"
     ;; Quit command - use mged-quit at top level, ECL's handler for break levels
     ((:q :quit) (lambda (&optional n)
                   (if (or (null n) (zerop n))
                       (mged-quit)
                       (si::tpl-quit-command n)))
      :eval
      ":q(uit)          Exit MGED"
      ":quit &optional n                             [Break command]~@
:q &optional n                                  [Abbreviation]~@~@
Exit MGED.~%")
     
     ;; Pop command - use mged-quit at top level
     ((:pop) (if (zerop si::*tpl-level*)
                 (mged-quit)
                 (si::tpl-pop-command))
      :constant
      ":pop             Exit MGED"
      ":pop                                          [Break command]~@~@
Exit MGED.~%")
     
     ;; Debugging commands - delegate to ECL
     ((:b :backtrace) si::tpl-backtrace nil
      ":b(acktrace)     Print backtrace"
      ":backtrace &optional n                        [Break command]~@~@
Show function call history.~%")
     
     ((:v :variables) si::tpl-variables-command nil
      ":v(ariables)     Show local variables"
      ":variables &optional no-values                [Break command]~@~@
Show lexical variables local to current function.~%")
     
     ((:i :inspect) si::tpl-inspect-command nil
      ":i(nspect)       Inspect value"
      ":inspect var-name                             [Break command]~@~@
Inspect value of local variable.~%")
     
     ((:m :message) si::tpl-print-message nil
      ":m(essage)       Show error message"
      ":message                                      [Break command]~@~@
Show current error message.~%")
     
     ((:f :function) si::tpl-print-current nil
      ":f(unction)      Show current function"
      ":function                                     [Break command]~@~@
Show current function.~%")
     
     ((:p :previous :d :down) si::tpl-previous nil
      ":p(revious)      Go to previous function"
      ":previous &optional (n 1)                     [Break command]~@~@
Move to previous function in backtrace.~%")
     
     ((:n :next :u :up) si::tpl-next nil
      ":n(ext)          Go to next function"
      ":next &optional (n 1)                         [Break command]~@~@
Move to next function in backtrace.~%")
     
     ((:disassemble) si::tpl-disassemble-command nil
      ":disassemble     Disassemble current function"
      ":disassemble                                  [Break command]~@~@
Disassemble current function.~%")))
  "MGED's custom command table for ECL REPL top-level commands.")

;;; Non-blocking REPL step function
;;; This processes one command if input is ready, allowing MGED's event loop
;;; to continue running and keep the display responsive.
(defun mged-repl-step ()
  "Do one REPL iteration if input is ready. Returns T if processed, NIL if no input."
  (when (stdin-ready)
    ;; Establish dynamic bindings that tpl-read expects to find
    ;; This is critical - tpl-read looks for *tpl-commands* in dynamic scope
    (let ((si::*tpl-commands* *mged-tpl-commands*)
          (si::*tpl-level* 0))
      (setq +++ ++ ++ + + -)
      (setq - (si::tpl-read))  ; Now sees *tpl-commands* in dynamic scope!
      
      ;; Establish restart that can be selected from ECL's debugger
      ;; When errors occur, users can choose this restart to return to top-level
      (with-simple-restart 
          (abort-to-toplevel "Return to MGED top-level REPL.")
        (let ((values (multiple-value-list 
                       (si::eval-with-env - si::*break-env*))))
          (setq /// // // / / values *** ** ** * * (car /))
          (format t "~&~{~S~^~%~}~%" values)))
      ;; Display prompt immediately after results, matching ECL's native behavior
      (si::tpl-prompt))
    t))

;;; Export the initialization function that C code will call
(defun init-mged-repl ()
  "Initialize MGED REPL - called from C code after ECL boot.
This function is exported for C linkage."
  ;; Initialize REPL state variables that tpl normally sets up
  (setq si::*tpl-level* 0
        si::*ihs-base* (si::ihs-top)
        si::*ihs-top* (si::ihs-top)
        si::*ihs-current* (si::ihs-top)
        si::*break-env* nil)
  
  ;; Return success
  t)
