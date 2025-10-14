;;; brlcad-api.scm - Scheme-idiomatic API for BRL-CAD
;;;
;;; Copyright (c) 2025 United States Government as represented by
;;; the U.S. Army Research Laboratory.
;;;
;;; This library is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public License
;;; version 2.1 as published by the Free Software Foundation.
;;;
;;; This library is distributed in the hope that it will be useful, but
;;; WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.

(define-module (brlcad api)
  #:export (;; Display
            draw
            erase
            blast
            kill-objects
            killall
            killtree
            zap
            who
            
            ;; Database Query
            list-objects
            search
            tree
            tops
            find
            which
            pathlist
            
            ;; Object Manipulation
            copy
            move
            clone
            duplicate
            mirror
            
            ;; Geometry Creation
            make-object
            create
            make-group
            make-region
            make-combination
            
            ;; Transformations
            translate
            rotate
            scale-object
            push-matrix
            pull-matrix
            
            ;; View Control
            view-aet
            view-size
            view-center
            autoview
            zoom
            lookat
            eye-position
            
            ;; Materials
            set-color
            set-shader
            set-material
            
            ;; Analysis
            analyze
            check
            heal
            bounding-box
            
            ;; File Operations
            concat-database
            keep
            dump
            
            ;; Utilities
            get-attribute
            set-attribute
            get-title
            set-title
            get-units
            set-units
            summary
            
            ;; Higher-Order
            draw-matching
            foreach-object
            filter-objects
            map-objects))

;;; Geometry Display Commands
;;; Note: draw, erase, blast, kill, killall, killtree, zap, who are available directly as C functions

;;; Database Query Commands

(define (list-objects . args)
  "List database objects. Returns list of object names.
Example: (list-objects)"
  (let ((result (apply ls args)))
    (if (string? result)
        (filter (lambda (s) (not (string-null? s)))
                (string-split result #\newline))
        '())))

;;; Note: search, tree, tops, find, which, pathlist are available directly as C functions

;;; View Manipulation Commands

(define (view-aet azimuth elevation twist)
  "Set view using azimuth, elevation, and twist angles.
Example: (view-aet 35 25 0)"
  (view "aet" 
        (number->string azimuth)
        (number->string elevation)
        (number->string twist)))

(define (view-size size)
  "Set view size.
Example: (view-size 1000)"
  (size (number->string size)))

(define (view-center x y z)
  "Set view center point.
Example: (view-center 0 0 0)"
  (center (number->string x)
          (number->string y)
          (number->string z)))

;;; Note: autoview, ae are available directly as C functions

;;; Object Creation Commands

(define (make-object name type . params)
  "Create a new object of specified type.
Example: (make-object \"sphere1.s\" \"sph\" 0 0 0 100)"
  (apply make (cons name (cons type params))))

(define (create . args)
  "Alias for make-object"
  (apply make-object args))

(define (make-group name . members)
  "Create a group (combination) containing specified objects.
Example: (make-group \"assembly.g\" \"part1.s\" \"part2.s\")"
  (apply g (cons name members)))

(define (make-region name . members)
  "Create a region containing specified objects.
Example: (make-region \"part.r\" \"solid1.s\" \"solid2.s\")"
  (apply r (cons name members)))

(define (make-combination name . members)
  "Create a combination with specified members.
Example: (make-combination \"combo\" \"obj1\" \"u\" \"obj2\")"
  (apply comb (cons name members)))

(define (copy-object src dest)
  "Copy an object.
Example: (copy-object \"obj1\" \"obj2\")"
  (cp src dest))

(define (move-object src dest)
  "Move/rename an object.
Example: (move-object \"old-name\" \"new-name\")"
  (mv src dest))

;;; Note: clone, dup, mirror, in, kill, rm are available directly as C functions

;;; Transformation Commands

(define (translate-object object x y z)
  "Translate object by specified amounts.
Example: (translate-object \"obj1\" 10 20 30)"
  (tra object
       (number->string x)
       (number->string y)
       (number->string z)))

(define (rotate-object object x y z)
  "Rotate object by specified angles.
Example: (rotate-object \"obj1\" 45 0 0)"
  (rot object
       (number->string x)
       (number->string y)
       (number->string z)))

(define (scale-object object factor)
  "Scale object by specified factor.
Example: (scale-object \"obj1\" 2.0)"
  (scale object (number->string factor)))

;;; Note: push, pull, translate, rotate, orotate, oscale, otranslate are available directly as C functions

;;; Additional View Commands

(define (zoom-view factor)
  "Zoom view by specified factor.
Example: (zoom-view 2.0)"
  (zoom (number->string factor)))

(define (lookat-point x y z)
  "Point view at specified coordinates.
Example: (lookat-point 0 0 0)"
  (lookat (number->string x)
          (number->string y)
          (number->string z)))

(define (eye-position x y z)
  "Set eye position.
Example: (eye-position 1000 1000 1000)"
  (eye_pos (number->string x)
           (number->string y)
           (number->string z)))

;;; Material and Appearance Commands

(define (set-color object r g b)
  "Set object color (RGB 0-255).
Example: (set-color \"obj1\" 255 0 0)"
  (color object
         (number->string r)
         (number->string g)
         (number->string b)))

(define (set-shader object shader-name . params)
  "Set object shader.
Example: (set-shader \"obj1\" \"plastic\")"
  (apply shader (cons object (cons shader-name params))))

(define (set-material object material-name)
  "Set object material properties.
Example: (set-material \"obj1\" \"steel\")"
  (mater object material-name))

;;; Analysis Commands
;;; Note: analyze, check, heal, bb are available directly as C functions

(define (bounding-box . objects)
  "Get bounding box of objects.
Example: (bounding-box \"obj1\")"
  (apply bb objects))

;;; File Operations

(define (concat-database file . args)
  "Concatenate another database file.
Example: (concat-database \"other.g\")"
  (apply dbconcat (cons file args)))

;;; Note: keep, dump, cat are available directly as C functions

;;; Attribute Management

(define (get-attribute object attr-name)
  "Get object attribute value.
Example: (get-attribute \"obj1\" \"material_id\")"
  (attr object "get" attr-name))

(define (set-attribute object attr-name value)
  "Set object attribute value.
Example: (set-attribute \"obj1\" \"material_id\" \"steel\")"
  (attr object "set" attr-name value))

;;; Database Properties

(define (get-title)
  "Get database title.
Example: (get-title)"
  (title))

(define (set-title new-title)
  "Set database title.
Example: (set-title \"My CAD Model\")"
  (title new-title))

(define (get-units)
  "Get database units.
Example: (get-units)"
  (units))

(define (set-units unit)
  "Set database units (mm, cm, m, in, ft).
Example: (set-units \"mm\")"
  (units unit))

;;; Note: summary is available directly as a C function

;;; Higher-Order Functions

(define (draw-matching pattern)
  "Draw all objects matching a pattern.
Example: (draw-matching \"*.s\")"
  (for-each (lambda (obj) (draw obj))
    (filter (lambda (obj)
              (string-contains obj pattern))
            (list-objects))))

(define (foreach-object proc . patterns)
  "Apply procedure to each object, optionally filtered by patterns.
Example: (foreach-object (lambda (obj) (display obj) (newline)))"
  (let ((objects (if (null? patterns)
                     (list-objects)
                     (let ((result (apply search (cons "/" patterns))))
                       (if (string? result)
                           (filter (lambda (s) (not (string-null? s)))
                                   (string-split result #\newline))
                           result)))))
    (if (list? objects)
        (for-each proc objects))))

(define (filter-objects predicate)
  "Filter objects by predicate function.
Example: (filter-objects (lambda (obj) (string-suffix? \".s\" obj)))"
  (filter predicate (list-objects)))

(define (map-objects proc)
  "Map procedure over all objects.
Example: (map-objects (lambda (obj) (string-length obj)))"
  (map proc (list-objects)))

;;; Utility Functions

(define (object-exists? name)
  "Check if an object exists in the database.
Example: (object-exists? \"sphere.s\")"
  (member name (list-objects)))

(define (get-object-type name)
  "Get the type of an object.
Example: (get-object-type \"sphere.s\")"
  ;; This would need additional GED command support
  #f)

;;; Print welcome message
(format #t "BRL-CAD ~a Scheme API loaded~%" *brlcad-version*)
(format #t "Use (help 'procedure-name) for documentation~%")
