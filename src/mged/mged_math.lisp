;;;; MGED Math Package
;;;;
;;;; This file provides vector mathematics operations for the ECL interface.
;;;; It offers clean, idiomatic Lisp functions for 3D vector operations
;;;; including addition, subtraction, scaling, and geometric calculations.
;;;;
;;;; This file is compiled by ECL at build time and linked into the mged binary.

(in-package :cl-user)

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

(in-package :mged-math)

;;;; ============================================================================
;;;; Vector Constructors and Predicates
;;;; ============================================================================

(defun make-vector (x y z)
  "Create a 3D vector from three coordinates."
  (vector x y z))

(defun vec (x y z)
  "Short form of make-vector for convenience."
  (vector x y z))

(defun vector-p (obj)
  "Check if object is a 3D vector."
  (and (vectorp obj) (= (length obj) 3)))

;;;; ============================================================================
;;;; Basic Vector Arithmetic
;;;; ============================================================================

(defun v+ (v1 v2 &rest more-vectors)
  "Add two or more vectors.
   Returns a new vector containing the element-wise sum."
  (unless (and (vector-p v1) (vector-p v2))
    (error "v+ requires vector arguments"))
  
  (let ((result (vector (+ (aref v1 0) (aref v2 0))
                       (+ (aref v1 1) (aref v2 1))
                       (+ (aref v1 2) (aref v2 2)))))
    (dolist (v more-vectors result)
      (unless (vector-p v)
        (error "v+ requires vector arguments"))
      (setf (aref result 0) (+ (aref result 0) (aref v 0)))
      (setf (aref result 1) (+ (aref result 1) (aref v 1)))
      (setf (aref result 2) (+ (aref result 2) (aref v 2))))))

(defun vector-add (v1 v2 &rest more-vectors)
  "Alias for v+ function for more descriptive naming."
  (apply #'v+ v1 v2 more-vectors))

(defun v- (v1 v2)
  "Subtract v2 from v1.
   Returns a new vector containing the element-wise difference."
  (unless (and (vector-p v1) (vector-p v2))
    (error "v- requires vector arguments"))
  
  (vector (- (aref v1 0) (aref v2 0))
          (- (aref v1 1) (aref v2 1))
          (- (aref v1 2) (aref v2 2))))

(defun vector-subtract (v1 v2)
  "Alias for v- function for more descriptive naming."
  (v- v1 v2))

(defun v* (vector factor)
  "Scale vector by a numeric factor.
   Returns a new vector with each element multiplied by factor."
  (unless (vector-p vector)
    (error "v* requires a vector as first argument"))
  (unless (numberp factor)
    (error "v* requires a numeric factor"))
  
  (vector (* (aref vector 0) factor)
          (* (aref vector 1) factor)
          (* (aref vector 2) factor)))

(defun vector-scale (vector factor)
  "Alias for v* function for more descriptive naming."
  (v* vector factor))

(defun v/ (vector factor)
  "Divide vector by a numeric factor.
   Returns a new vector with each element divided by factor."
  (unless (vector-p vector)
    (error "v/ requires a vector as first argument"))
  (unless (numberp factor)
    (error "v/ requires a numeric factor"))
  (when (zerop factor)
    (error "v/ cannot divide by zero"))
  
  (vector (/ (aref vector 0) factor)
          (/ (aref vector 1) factor)
          (/ (aref vector 2) factor)))

(defun vector-divide (vector factor)
  "Alias for v/ function for more descriptive naming."
  (v/ vector factor))

;;;; ============================================================================
;;;; Vector Mathematical Operations
;;;; ============================================================================

(defun magnitude (vector)
  "Calculate the magnitude (length) of a vector."
  (unless (vector-p vector)
    (error "magnitude requires a vector argument"))
  
  (sqrt (+ (expt (aref vector 0) 2)
           (expt (aref vector 1) 2)
           (expt (aref vector 2) 2))))

(defun normalize (vector)
  "Return a normalized (unit) vector.
   If the input vector has zero magnitude, returns a zero vector."
  (unless (vector-p vector)
    (error "normalize requires a vector argument"))
  
  (let ((mag (magnitude vector)))
    (if (zerop mag)
        (vector 0 0 0)
        (v/ vector mag))))

(defun dot (v1 v2)
  "Calculate the dot product of two vectors."
  (unless (and (vector-p v1) (vector-p v2))
    (error "dot requires vector arguments"))
  
  (+ (* (aref v1 0) (aref v2 0))
     (* (aref v1 1) (aref v2 1))
     (* (aref v1 2) (aref v2 2))))

(defun cross (v1 v2)
  "Calculate the cross product of two vectors."
  (unless (and (vector-p v1) (vector-p v2))
    (error "cross requires vector arguments"))
  
  (vector (- (* (aref v1 1) (aref v2 2))
             (* (aref v1 2) (aref v2 1)))
          (- (* (aref v1 2) (aref v2 0))
             (* (aref v1 0) (aref v2 2)))
          (- (* (aref v1 0) (aref v2 1))
             (* (aref v1 1) (aref v2 0)))))

;;;; ============================================================================
;;;; Geometric Utility Functions
;;;; ============================================================================

(defun distance (v1 v2)
  "Calculate the Euclidean distance between two vectors."
  (unless (and (vector-p v1) (vector-p v2))
    (error "distance requires vector arguments"))
  
  (magnitude (v- v1 v2)))

(defun angle (v1 v2)
  "Calculate the angle between two vectors in radians.
   Returns 0 if either vector has zero magnitude."
  (unless (and (vector-p v1) (vector-p v2))
    (error "angle requires vector arguments"))
  
  (let ((mag1 (magnitude v1))
        (mag2 (magnitude v2)))
    (if (or (zerop mag1) (zerop mag2))
        0
        (let ((cos-angle (/ (dot v1 v2) (* mag1 mag2))))
          ;; Clamp to [-1, 1] to handle floating point errors
          (setf cos-angle (max -1.0 (min 1.0 cos-angle)))
          (acos cos-angle)))))

;;;; ============================================================================
;;;; Useful Constants and Convenience Functions
;;;; ============================================================================

(defconstant +zero-vector+ (vector 0 0 0)
  "The zero vector (0, 0, 0).")

(defconstant +x-axis+ (vector 1 0 0)
  "Unit vector in the X direction.")

(defconstant +y-axis+ (vector 0 1 0)
  "Unit vector in the Y direction.")

(defconstant +z-axis+ (vector 0 0 1)
  "Unit vector in the Z direction.")

(defun zero-vector-p (vector)
  "Check if a vector is the zero vector."
  (and (vector-p vector)
       (zerop (aref vector 0))
       (zerop (aref vector 1))
       (zerop (aref vector 2))))

(defun unit-vector-p (vector &optional (tolerance 1e-6))
  "Check if a vector is approximately a unit vector."
  (and (vector-p vector)
       (< (abs (- (magnitude vector) 1.0)) tolerance)))

;;;; ============================================================================
;;;; String Conversion for MGED Compatibility
;;;; ============================================================================

(defun vector->string (vector)
  "Convert a vector to a space-separated string for MGED commands."
  (unless (vector-p vector)
    (error "vector->string requires a vector argument"))
  
  (format nil "~{~A~^ ~}" (coerce vector 'list)))

(defun string->vector (string)
  "Parse a space-separated string into a vector."
  (unless (stringp string)
    (error "string->vector requires a string argument"))
  
  (let* ((tokens '())
         (start 0)
         (end (length string)))
    ;; Simple tokenization by whitespace
    (loop for i from 0 to end
          do (when (or (= i end) (char= (char string i) #\Space))
               (when (> i start)
                 (push (subseq string start i) tokens))
               (setf start (1+ i))))
    (setf tokens (nreverse tokens))
    
    (when (/= (length tokens) 3)
      (error "string->vector requires exactly 3 numeric components"))
    (vector (read-from-string (nth 0 tokens))
            (read-from-string (nth 1 tokens))
            (read-from-string (nth 2 tokens)))))
