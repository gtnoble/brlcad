;;;; MGED High-Level API
;;;;
;;;; This file provides idiomatic Lisp wrappers around the low-level MGED command
;;;; bindings. It offers keyword arguments, vector representations for points/directions,
;;;; and better error handling compared to the raw command interface.
;;;;
;;;; This file is compiled by ECL at build time and linked into the mged binary.

(in-package :cl-user)

;;; Ensure MGED package exists at compile-time
;;; (The full package is defined in mged_commands.lisp and loaded at runtime)
(eval-when (:compile-toplevel :load-toplevel :execute)
  (unless (find-package :mged)
    (defpackage :mged
      (:use :cl)
      (:shadow #:get)  ; Shadow CL's GET so we can export our own
      (:export 
       ;; Export all symbols used by mged-api
       #:ls #:draw #:erase #:Z #:exists #:attr #:get
       #:in #:c #:mater #:r #:g #:cp #:mv #:kill
       #:killall #:killtree #:tra #:center #:rot #:sca #:rt))))

;;; Define the high-level API package
(defpackage :mged-api
  (:use :cl)
  (:documentation "High-level, idiomatic Lisp API for MGED commands.")
  (:export 
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

(in-package :mged-api)

;;;; ============================================================================
;;;; Utility Functions
;;;; ============================================================================

(defun vector->string (vec)
  "Convert a vector #(x y z) to a space-separated string 'x y z'."
  (format nil "~{~A~^ ~}" (coerce vec 'list)))

(defun color->string (color)
  "Convert a color vector #(r g b) to a string 'r/g/b'."
  (etypecase color
    (vector (format nil "~D/~D/~D" 
                    (aref color 0) (aref color 1) (aref color 2)))
    (list (format nil "~D/~D/~D" 
                  (first color) (second color) (third color)))))

(defun ensure-list (obj)
  "Ensure OBJ is a list. If it's a single item, wrap it in a list."
  (if (listp obj) obj (list obj)))

(defun split-lines (string)
  "Split a string by newlines, removing empty lines.
   Returns a list of non-empty line strings."
  (when string
    (remove-if #'(lambda (s) (zerop (length s)))
               (loop for start = 0 then (1+ end)
                     for end = (position #\Newline string :start start)
                     collect (subseq string start end)
                     while end))))

(defun split-string-by-whitespace (string)
  "Split a string by any whitespace (space, tab, newline).
   Returns a list of non-empty strings."
  (when string
    (let ((result '())
          (start 0))
      (loop for i from 0 below (length string)
            do (when (member (char string i) '(#\Space #\Tab #\Newline))
                 (when (> i start)
                   (push (subseq string start i) result))
                 (setf start (1+ i)))
            finally (when (< start (length string))
                      (push (subseq string start) result)))
      (nreverse result))))

(defun parse-ls-line (line)
  "Parse a long-format ls line into a plist.
   Long format: name type major-type minor-type length
   Returns: (:name \"name\" :type \"type\" :major-type \"n\" :minor-type \"n\" :length \"n\")"
  (let ((fields (split-string-by-whitespace line)))
    (when (>= (length fields) 5)
      (list :name (nth 0 fields)
            :type (nth 1 fields)
            :major-type (nth 2 fields)
            :minor-type (nth 3 fields)
            :length (nth 4 fields)))))

(defun parse-ls-output (string long-format)
  "Parse ls command output into a list.
   For regular format: returns list of object name strings.
   For long format: returns list of plists with object information."
  (when string
    (if long-format
        ;; Long format: parse each line into structured data
        (let ((lines (split-lines string)))
          (remove nil (mapcar #'parse-ls-line lines)))
        ;; Regular format: split by whitespace to get names
        (split-string-by-whitespace string))))

(defun validate-alist-attributes (attributes &optional context)
  "Validate that ATTRIBUTES is a proper alist of (string . string) pairs.
   
   Arguments:
     ATTRIBUTES - The attributes list to validate
     CONTEXT - Optional string describing the context for error messages
   
   Returns:
     T if valid
   
   Signals:
     ERROR if validation fails with descriptive message"
  
  (when attributes
    ;; Check that it's a list
    (unless (listp attributes)
      (error "~AATTRIBUTES must be a list, got: ~A" 
             (if context (format nil "~A: " context) "")
             (type-of attributes)))
    
    ;; Check each element is a proper cons pair with strings
    (loop for item in attributes
          for position from 1
          do (unless (consp item)
               (error "~AAttribute at position ~D must be a cons pair, got: ~A"
                      (if context (format nil "~A: " context) "")
                      position
                      (type-of item)))
             (unless (stringp (car item))
               (error "~AAttribute name must be a string, got: ~A at position ~D"
                      (if context (format nil "~A: " context) "")
                      (type-of (car item))
                      position))
             (unless (stringp (cdr item))
               (error "~AAttribute value must be a string, got: ~A at position ~D"
                      (if context (format nil "~A: " context) "")
                      (type-of (cdr item))
                      position))))
  t)

(defun build-flag-args (flags)
  "Build argument list from flag specifications.
   FLAGS is a plist like (:color #(255 0 0) :shaded t :no-resize t)
   Returns list of strings like (\"-C\" \"255/0/0\" \"-m1\" \"-R\")"
  (let ((args '()))
    (loop for (key value) on flags by #'cddr
          do (case key
               (:color 
                (when value
                  (push "-C" args)
                  (push (color->string value) args)))
               (:shaded 
                (when value (push "-m1" args)))
               (:no-resize 
                (when value (push "-R" args)))
               (:simplified 
                (when value (push "-S" args)))
               (:wireframe-threshold
                (when value
                  (push "-L" args)
                  (push (princ-to-string value) args)))
               (:shape-lines
                (when value (push "-s" args)))
               (:all 
                (when value (push "-a" args)))
               (:primitives 
                (when value (push "-p" args)))
               (:regions 
                (when value (push "-r" args)))
               (:combinations 
                (when value (push "-c" args)))
               (:long-format 
                (when value (push "-l" args)))
               (:human-readable 
                (when value (push "-H" args)))
               (:sort-by-size 
                (when value (push "-S" args)))
               (:quiet 
                (when value (push "-q" args)))
               (:match-any
                (when value (push "-o" args)))
               (:attributes
                (when value (push "-A" args)))))
    (nreverse args)))

;;;; ============================================================================
;;;; Listing & Query Functions
;;;; ============================================================================

(defun list-objects (&key all primitives regions combinations 
                          pattern attributes match-any long-format 
                          human-readable sort-by-size quiet)
  "List objects in the database with various filters.
  
  Keyword arguments:
    :ALL - List all objects (including hidden)
    :PRIMITIVES - List only primitives
    :REGIONS - List only regions
    :COMBINATIONS - List only combinations
    :PATTERN - String pattern to match (supports wildcards)
    :ATTRIBUTES - Alist of attribute name/value pairs to match
    :MATCH-ANY - If T with :ATTRIBUTES, match any attribute (OR logic)
    :LONG-FORMAT - If T, return detailed information as plists
    :HUMAN-READABLE - If T with :LONG-FORMAT, use human-readable sizes
    :SORT-BY-SIZE - If T, sort by object size
    :QUIET - If T, suppress informational messages
  
  Returns:
    - If :LONG-FORMAT is NIL: List of object name strings
    - If :LONG-FORMAT is T: List of plists with keys:
        :NAME - Object name
        :TYPE - Object type
        :MAJOR-TYPE - Major type code
        :MINOR-TYPE - Minor type code
        :LENGTH - Object size/length
    Returns NIL on error"
  
  (let ((args (build-flag-args 
               (list :all all
                     :primitives primitives
                     :regions regions
                     :combinations combinations
                     :long-format long-format
                     :human-readable human-readable
                     :sort-by-size sort-by-size
                     :quiet quiet
                     :attributes attributes
                     :match-any match-any))))
    
    ;; Add attribute pairs if provided
    (when attributes
      (dolist (attr-pair attributes)
        (push (car attr-pair) args)
        (push (cdr attr-pair) args)))
    
    ;; Add pattern if provided
    (when pattern
      (push pattern args))
    
    ;; Get result from mged:ls and parse into appropriate format
    (let ((result (apply #'mged:ls (nreverse args))))
      (parse-ls-output result long-format))))

(defun object-exists-p (name)
  "Check if an object exists in the database.
  
  Arguments:
    NAME - String name of the object to check
  
  Returns:
    T if object exists, NIL otherwise"
  (handler-case
      (progn
        (mged:exists name)
        t)
    (mged-error () nil)))

(defun get-object-info (name &key attributes)
  "Get information about an object.
  
  Arguments:
    NAME - String name of the object
    :ATTRIBUTES - If T, also return attributes as structured data
  
  Returns:
    - If ATTRIBUTES is NIL: String containing object information
    - If ATTRIBUTES is T: plist of attribute-name -> value pairs
    - NIL on error"
  (if attributes
      (get-attributes name)
      (mged:get name)))

;;;; ============================================================================
;;;; Display Operations
;;;; ============================================================================

(defun draw-objects (names &key color shaded wireframe-threshold 
                           no-resize simplified shape-lines
                           attributes match-any)
  "Draw objects on the display.
  
  Arguments:
    NAMES - String or list of strings specifying object names
  
  Keyword arguments:
    :COLOR - Vector #(r g b) or list (r g b) specifying override color
    :SHADED - If T, draw shaded (requires OpenGL + zbuffer + lighting)
    :WIREFRAME-THRESHOLD - Integer face count for BOT wireframe threshold
    :NO-RESIZE - If T, don't auto-resize view
    :SIMPLIFIED - If T, skip subtractions and simplify display
    :SHAPE-LINES - If T, draw subtracted/intersected with shape lines
    :ATTRIBUTES - Alist of attribute name/value pairs to filter objects
    :MATCH-ANY - If T with :ATTRIBUTES, match any attribute (OR logic)
  
  Returns:
    Result string from draw command, or NIL on error"
  
  (let* ((name-list (ensure-list names))
         (args (build-flag-args 
                (list :color color
                      :shaded shaded
                      :wireframe-threshold wireframe-threshold
                      :no-resize no-resize
                      :simplified simplified
                      :shape-lines shape-lines
                      :attributes attributes
                      :match-any match-any))))
    
    ;; Add attribute pairs if provided
    (when attributes
      (dolist (attr-pair attributes)
        (push (car attr-pair) args)
        (push (cdr attr-pair) args)))
    
    (apply #'mged:draw (append (nreverse args) name-list))))

(defun erase-objects (names)
  "Erase objects from the display.
  
  Arguments:
    NAMES - String or list of strings specifying object names to erase
  
  Returns:
    Result string from erase command"
  (let ((name-list (ensure-list names)))
    (apply #'mged:erase name-list)))

(defun clear-display ()
  "Clear all objects from the display (equivalent to 'Z' command).
  
  Returns:
    Result from the Z command"
  (mged:Z))

;;;; ============================================================================
;;;; Primitive Creation Functions
;;;; ============================================================================

(defun create-primitive (name type &rest parameters)
  "Low-level primitive creation (wrapper around 'in' command).
   Automatically expands vectors into separate arguments.
  
  Arguments:
    NAME - String name for the new primitive
    TYPE - String primitive type (e.g., 'sph', 'rcc', 'arb8')
    PARAMETERS - Remaining arguments (vectors automatically expanded)
  
  Returns:
    Result string from 'in' command"
  
  (let ((expanded-params 
         (mapcan (lambda (param)
                   (etypecase param
                     (vector 
                      ;; Expand vector into separate string arguments
                      (map 'list #'princ-to-string (coerce param 'list)))
                     (number 
                      (list (princ-to-string param)))
                     (string 
                      (list param))))
                 parameters)))
    (apply #'mged:in name type expanded-params)))

(defun make-sphere (name center radius)
  "Create a sphere primitive.
  
  Arguments:
    NAME - String name for the sphere
    CENTER - Vector #(x y z) specifying sphere center
    RADIUS - Number specifying sphere radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "sph" center radius))

(defun make-ellipsoid (name center a b c)
  "Create an ellipsoid primitive.
  
  Arguments:
    NAME - String name for the ellipsoid
    CENTER - Vector #(x y z) specifying ellipsoid center
    A - Vector #(x y z) specifying first semi-axis
    B - Vector #(x y z) specifying second semi-axis
    C - Vector #(x y z) specifying third semi-axis
  
  Returns:
    Result string from creation command"
  (create-primitive name "ell" center a b c))

(defun make-cylinder (name base height radius)
  "Create a right circular cylinder (RCC).
  
  Arguments:
    NAME - String name for the cylinder
    BASE - Vector #(x y z) specifying base center point
    HEIGHT - Vector #(x y z) specifying height vector
    RADIUS - Number specifying cylinder radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "rcc" base height radius))

(defun make-cone (name base height base-radius top-radius)
  "Create a truncated right circular cone (TRC).
  
  Arguments:
    NAME - String name for the cone
    BASE - Vector #(x y z) specifying base center point
    HEIGHT - Vector #(x y z) specifying height vector
    BASE-RADIUS - Number specifying base radius
    TOP-RADIUS - Number specifying top radius
  
  Returns:
    Result string from creation command"
  (create-primitive name "trc" base height base-radius top-radius))

(defun make-box (name corner dimensions)
  "Create a box (ARB6 with specific vertex arrangement).
  
  Arguments:
    NAME - String name for the box
    CORNER - Vector #(x y z) specifying one corner
    DIMENSIONS - Vector #(w h d) specifying width, height, depth
  
  Returns:
    Result string from creation command"
  (let ((x (aref corner 0))
        (y (aref corner 1))
        (z (aref corner 2))
        (w (aref dimensions 0))
        (h (aref dimensions 1))
        (d (aref dimensions 2)))
    ;; Create an RPP (right rectangular parallelepiped)
    (create-primitive name "rpp" 
                      (vector x y z)
                      (vector (+ x w) (+ y h) (+ z d)))))

(defun make-arb8 (name vertices)
  "Create an ARB8 (arbitrary 8-vertex polyhedron).
  
  Arguments:
    NAME - String name for the ARB8
    VERTICES - List of 8 vectors, each #(x y z), specifying vertices
  
  Returns:
    Result string from creation command"
  (unless (= (length vertices) 8)
    (error "ARB8 requires exactly 8 vertices, got ~D" (length vertices)))
  (apply #'create-primitive name "arb8" vertices))

(defun make-torus (name center normal inner-radius outer-radius)
  "Create a torus primitive.
  
  Arguments:
    NAME - String name for the torus
    CENTER - Vector #(x y z) specifying torus center
    NORMAL - Vector #(x y z) specifying normal direction
    INNER-RADIUS - Number specifying tube radius
    OUTER-RADIUS - Number specifying distance from center to tube center
  
  Returns:
    Result string from creation command"
  (create-primitive name "tor" center normal inner-radius outer-radius))

(defun make-particle (name base height base-radius top-radius)
  "Create a particle primitive.
  
  Arguments:
    NAME - String name for the particle
    BASE - Vector #(x y z) specifying base point
    HEIGHT - Vector #(x y z) specifying height vector
    BASE-RADIUS - Number specifying base radius
    TOP-RADIUS - Number specifying top radius (0 for point)
  
  Returns:
    Result string from creation command"
  (create-primitive name "part" base height base-radius top-radius))

;;;; ============================================================================
;;;; Combinations & Regions
;;;; ============================================================================

(defun make-combination (name members &key color shader)
  "Create a combination from members.
  
  Arguments:
    NAME - String name for the combination
    MEMBERS - List of member specifications. Each member is either:
              - A string (object name, assumes union operation)
              - A list (object-name operation), where operation is :union, :subtract, or :intersect
  
  Keyword arguments:
    :COLOR - Vector #(r g b) or list (r g b) specifying combination color
    :SHADER - String specifying shader name
  
  Returns:
    Result string from 'c' command"
  
  (let ((member-strings
         (mapcar (lambda (member)
                   (etypecase member
                     (string (format nil "u ~A" member))
                     (list 
                      (let ((obj (first member))
                            (op (second member)))
                        (format nil "~A ~A"
                                (case op
                                  (:union "u")
                                  (:subtract "-")
                                  (:intersect "+")
                                  (t "u"))
                                obj)))))
                 members)))
    
    ;; Create the combination
    (apply #'mged:c name member-strings)
    
    ;; Set color if provided
    (when color
      (mged:mater name "" "" (color->string color) ""))
    
    ;; Set shader if provided
    (when shader
      (mged:mater name shader "" "" ""))))

(defun make-region (name members &key id color shader material attributes)
  "Create a region from members.
  
  Arguments:
    NAME - String name for the region
    MEMBERS - List of member specifications (same format as make-combination)
  
  Keyword arguments:
    :ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                  '((\"material_id\" . \"10\") (\"custom_prop\" . \"value\"))
                  Only alist format is supported.
    :ID - Integer region ID
    :COLOR - Vector #(r g b) or list (r g b) specifying region color
    :SHADER - String specifying shader name
    :MATERIAL - String specifying material name
  
  Returns:
    Result string from 'r' command"
  
  (let ((member-strings
         (mapcar (lambda (member)
                   (etypecase member
                     (string (format nil "u ~A" member))
                     (list 
                      (let ((obj (first member))
                            (op (second member)))
                        (format nil "~A ~A"
                                (case op
                                  (:union "u")
                                  (:subtract "-")
                                  (:intersect "+")
                                  (t "u"))
                                obj)))))
                 members)))
    
    ;; Create the region
    (apply #'mged:r name member-strings)
    
    ;; Set arbitrary attributes if provided (alist format only)
    (when attributes
      (validate-alist-attributes attributes "make-region")
      (apply #'mged:attr "set" name 
             (mapcan (lambda (pair) 
                       (list (car pair) (cdr pair))) 
                   attributes)))
    
    ;; Set region ID if provided
    (when id
      (set-attributes name `(("region_id" . ,(princ-to-string id)))))
    
    ;; Set color if provided
    (when color
      (mged:mater name "" "" (color->string color) ""))
    
    ;; Set shader if provided
    (when shader
      (mged:mater name shader "" "" ""))
    
    ;; Set material if provided
    (when material
      (mged:mater name "" material "" ""))))

(defun make-group (name members)
  "Create a group (combination with union operation on all members).
  
  Arguments:
    NAME - String name for the group
    MEMBERS - List of strings specifying member object names
  
  Returns:
    Result string from 'g' command"
  (apply #'mged:g name members))

;;;; ============================================================================
;;;; Object Manipulation
;;;; ============================================================================

(defun copy-object (old-name new-name)
  "Copy an object to a new name.
  
  Arguments:
    OLD-NAME - String name of existing object
    NEW-NAME - String name for the copy
  
  Returns:
    Result string from 'cp' command"
  (mged:cp old-name new-name))

(defun move-object (old-name new-name)
  "Rename/move an object.
  
  Arguments:
    OLD-NAME - String name of existing object
    NEW-NAME - String new name for the object
  
  Returns:
    Result string from 'mv' command"
  (mged:mv old-name new-name))

;;;; ============================================================================
;;;; Object Deletion Operations
;;;; ============================================================================

(defun parse-kill-output (output)
  "Parse kill command output to extract object names.
   Used for dry-run mode (-n flag) to return structured data.
   
   Arguments:
     OUTPUT - String output from kill/killall/killtree with -n flag
   
   Returns:
     List of object name strings that would be killed"
  (when output
    (let ((lines (split-lines output)))
      (remove-if #'(lambda (s) (zerop (length s)))
                 (mapcar #'(lambda (line)
                           (string-trim "(\"" (string-trim ")\"" line)))
                         lines)))))

(defun build-kill-flags (flags)
  "Build argument list for kill command flags.
   FLAGS is a plist like (:force t :quiet t)
   Returns list of flag strings."
  (let ((args '()))
    (loop for (key value) on flags by #'cddr
          do (case key
               (:force 
                (when value (push "-f" args)))
               (:quiet 
                (when value (push "-q" args)))
               (:dry-run 
                (when value (push "-n" args)))
               (:all 
                (when value (push "-a" args)))))
    (nreverse args)))

(defun kill-objects (names &key force quiet)
  "Delete specified objects from the database.
   
   This is a high-level wrapper for the 'kill' command with Lisp-idiomatic
   keyword arguments. Objects are deleted immediately - there is no undo.
   
   Arguments:
     NAMES - String or list of strings specifying object names to delete
   
   Keyword arguments:
     :FORCE - If T, don't complain if some objects don't exist (maps to -f flag)
     :QUIET - If T, suppress database object lookup failure messages (maps to -q flag)
   
   Returns:
     Result string from kill command, or NIL on error
   
   Example:
     ;; Delete specific objects
     (kill-objects '(\"sphere1\" \"box2\"))
     
     ;; Delete with force flag (no complaints about missing objects)
     (kill-objects \"temp_obj\" :force t)
     
     ;; Delete quietly
     (kill-objects '(\"obj1\" \"obj2\") :quiet t)
   
   Warning: This operation is destructive and cannot be undone. Use with caution."
  
  (let* ((name-list (ensure-list names))
         (flag-args (build-kill-flags (list :force force :quiet quiet))))
    (apply #'mged:kill (append flag-args name-list))))

(defun kill-all-objects (&optional names &key dry-run)
  "Delete specified objects and remove all references to them from combinations.
   If no objects are specified, deletes ALL objects in the database.
   
   This is a high-level wrapper for the 'killall' command with enhanced
   functionality. When no names are provided, it operates on all objects.
   
   Arguments:
     NAMES - Optional string or list of strings specifying object names.
             If NIL or omitted, operates on ALL objects in database.
   
   Keyword arguments:
     :DRY-RUN - If T, return list of objects that would be killed without
                actually deleting them (maps to -n flag)
   
   Returns:
     - Normal mode: Result string from killall command, or NIL on error
     - Dry-run mode: List of object name strings that would be killed
   
   Example:
     ;; Delete specific objects and all references
     (kill-all-objects '(\"sphere1\" \"box2\"))
     
     ;; Delete ALL objects in database (use with extreme caution!)
     (kill-all-objects)
     
     ;; Dry run to see what would be deleted
     (kill-all-objects :dry-run t)
     
     ;; Dry run for specific objects
     (kill-all-objects '(\"temp1\" \"temp2\") :dry-run t)
   
   Warning: This operation is destructive and cannot be undone. 
            When no names are specified, it will delete ALL objects.
            Consider using :DRY-RUN T first to verify what will be deleted."
  
  (let* ((target-names (if names
                           (ensure-list names)
                           ;; If no names specified, get all objects
                           (list-objects)))
         (flag-args (build-kill-flags (list :dry-run dry-run))))
    
    (if dry-run
        ;; Dry run mode - parse and return object list
        (let ((result (apply #'mged:killall (append flag-args target-names))))
          (parse-kill-output result))
        ;; Normal mode - execute deletion
        (apply #'mged:killall (append flag-args target-names)))))

(defun kill-object-tree (names &key all force dry-run)
  "Delete specified objects and recursively delete all objects they reference.
   
   This is a high-level wrapper for the 'killtree' command with Lisp-idiomatic
   keyword arguments. For each combination among the specified objects, the
   combination and all its members are deleted recursively.
   
   Arguments:
     NAMES - String or list of strings specifying object names to delete
   
   Keyword arguments:
     :ALL - If T, kill objects even if referenced elsewhere, then kill all
            references (maps to -a flag, equivalent to killall on each member)
     :FORCE - If T, kill objects even if referenced elsewhere (may create
              dangling references, maps to -f flag)
     :DRY-RUN - If T, return list of objects that would be killed without
                actually deleting them (maps to -n flag)
   
   Returns:
     - Normal mode: Result string from killtree command, or NIL on error
     - Dry-run mode: List of object name strings that would be killed
   
   Example:
     ;; Delete object tree recursively
     (kill-object-tree \"assembly1\")
     
     ;; Delete with all references (safer than :force)
     (kill-object-tree '(\"group1\" \"group2\") :all t)
     
     ;; Force delete (may create dangling references)
     (kill-object-tree \"complex_assembly\" :force t)
     
     ;; Dry run to see what would be deleted
     (kill-object-tree \"assembly1\" :dry-run t)
   
   Note: :ALL flag is generally safer than :FORCE as it cleans up references.
   
   Warning: This operation is destructive and cannot be undone. 
            Consider using :DRY-RUN T first to verify what will be deleted."
  
  (let* ((name-list (ensure-list names))
         (flag-args (build-kill-flags (list :all all :force force :dry-run dry-run))))
    
    (if dry-run
        ;; Dry run mode - parse and return object list
        (let ((result (apply #'mged:killtree (append flag-args name-list))))
          (parse-kill-output result))
        ;; Normal mode - execute deletion
        (apply #'mged:killtree (append flag-args name-list)))))

;;;; ============================================================================
;;;; Transformation Functions
;;;; ============================================================================

(defun translate-object (offset)
  "Translate the currently edited object by an offset vector.
  
  Arguments:
    OFFSET - Vector #(dx dy dz) specifying translation offset
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'tra' command"
  (mged:tra (aref offset 0) (aref offset 1) (aref offset 2)))

(defun rotate-object (angles &key about-point)
  "Rotate the currently edited object.
  
  Arguments:
    ANGLES - Vector #(rx ry rz) specifying rotation angles in degrees
  
  Keyword arguments:
    :ABOUT-POINT - Vector #(x y z) specifying point to rotate about (optional)
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'rot' command"
  (if about-point
      ;; Rotate about a specific point (using center command first)
      (progn
        (mged:center (vector->string about-point))
        (mged:rot (aref angles 0) (aref angles 1) (aref angles 2)))
      ;; Rotate about current center
      (mged:rot (aref angles 0) (aref angles 1) (aref angles 2))))

(defun scale-object (factor &key uniform)
  "Scale the currently edited object.
  
  Arguments:
    FACTOR - Number or vector #(sx sy sz) specifying scale factor(s)
  
  Keyword arguments:
    :UNIFORM - If T, FACTOR must be a number and applies uniformly
  
  Note: This operates on the object currently in edit mode.
  
  Returns:
    Result string from 'sca' command"
  (if uniform
      (mged:sca (if (numberp factor) factor (error "Uniform scale requires a number")))
      (if (vectorp factor)
          (mged:sca (aref factor 0) (aref factor 1) (aref factor 2))
          (mged:sca factor))))

;;;; ============================================================================
;;;; Attribute Management Utility Functions
;;;; ============================================================================

(defun parse-attribute-output (output)
  "Parse attribute command output into structured data.
   
   Arguments:
     OUTPUT - String output from attr command
     SINGLE-OBJECT-P - If T, parse for single object (returns plist)
                      If NIL, parse for multiple objects (returns list of plists)
   
   Returns:
     List of (object-name . attribute-plist) pairs"
  (when output
    (let ((lines (split-lines output)))
      (remove-if #'null
                 (mapcar (lambda (line)
                           (when (and line (plusp (length line)))
                             (let ((trimmed (string-trim " " line)))
                               ;; Parse multiple objects: object_name attr_name value
                               (let* ((parts (split-string-by-whitespace trimmed))
                                      (object-name (first parts))
                                      (attr-name (second parts))
                                      (attr-value (third parts)))
                                 (when (and object-name attr-name attr-value)
                                   (cons object-name
                                         (list (intern (string-upcase attr-name) :keyword)
                                               attr-value)))))))
                         lines)))))

(defun normalize-attribute-pairs (attributes)
  "Normalize attribute specifications to a list of (name . value) pairs.
   
   Arguments:
     ATTRIBUTES - Can be:
                  - Alist of (name . value) pairs
                  - Plist of name value name value...
                  - List of two-element lists ((name value) ...)
   
   Returns:
     List of (name . value) cons pairs"
  (when attributes
    (etypecase attributes
      (list
       (cond
         ;; Check if it's an alist (first element is a cons)
         ((and (first attributes) (consp (first attributes)))
          (mapcar (lambda (pair)
                    (if (consp pair)
                        pair
                        (error "Invalid attribute pair: ~A" pair)))
                  attributes))
         ;; Check if it's a plist (even number of elements, first is not a cons)
         ((evenp (length attributes))
          (loop for (name value) on attributes by #'cddr
                collect (cons name value)))
         (t
          (error "Attribute list must have even number of elements or be an alist")))
       (t
        (error "Attributes must be a list"))))))

(defun build-attribute-args (attribute-pairs)
  "Convert attribute pairs to flat argument list for attr command.
   
   Arguments:
     ATTRIBUTE-PAIRS - List of (name . value) cons pairs
   
   Returns:
     Flat list of name value name value..."
  (when attribute-pairs
    (mapcan (lambda (pair)
              (list (car pair) (cdr pair)))
            attribute-pairs)))

;;;; ============================================================================
;;;; Attribute Management Functions
;;;; ============================================================================

(defun get-attributes (object-pattern &key attribute-names all-attributes)
  "Retrieve attributes from objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :ATTRIBUTE-NAMES - List of specific attribute names to retrieve
     :ALL-ATTRIBUTES - If T, retrieve all attributes (default when no specific names)
   
   Returns:
     - Single object match: plist of attribute-name -> value pairs
     - Multiple object matches: list of (object-name . attribute-plist) pairs
     - NIL if no objects match or no attributes found
   
   Examples:
     ;; Get all attributes for a single object
     (get-attributes \"region1\")
     ;; => (:MATERIAL-ID \"10\" :REGION \"R\" :LOS \"100\")
     
     ;; Get specific attributes
     (get-attributes \"region1\" :attribute-names '(\"material_id\" \"color\"))
     ;; => (:MATERIAL-ID \"10\" :COLOR \"255/0/0\")
     
     ;; Get attributes from multiple objects
     (get-attributes \"region*\")
     ;; => ((\"region1\" (:MATERIAL-ID \"10\" :REGION \"R\"))
     ;;     (\"region2\" (:MATERIAL-ID \"20\" :REGION \"R\")))"
  
  (let* ((attr-names (or attribute-names 
                        (when all-attributes '("*"))
                        '("*")))
         (args (append '("get") (list object-pattern) attr-names))
         (result (apply #'mged:attr args))
         (parsed (parse-attribute-output result)))
    
    ;; Group attributes by object
    (if (null parsed)
        nil
        (let ((object-groups (make-hash-table :test 'equal)))
          ;; Group attributes by object name
          (dolist (item parsed)
            (let* ((object-name (car item))
                   (attr-pair (cdr item))
                   (existing-attrs (gethash object-name object-groups)))
              (setf (gethash object-name object-groups)
                    (append existing-attrs attr-pair))))
          
          ;; Convert to appropriate return format
          (let ((objects (loop for key being the hash-keys of object-groups
                               collect key)))
            (if (= (length objects) 1)
                ;; Single object - return plist
                (gethash (first objects) object-groups)
                ;; Multiple objects - return list of pairs
                (mapcar (lambda (obj-name)
                          (cons obj-name (gethash obj-name object-groups)))
                        objects)))))))

(defun set-attributes (object-pattern attributes)
  "Set attribute values on objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                  '(\"material_id\" . \"10\") (\"color\" . \"255/0/0\"))
                  Only alist format is supported.
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create objects if they don't exist (not implemented)
   
   Returns:
     Result string from attr set command, or NIL on error
   
   Examples:
     ;; Set attributes using alist
     (set-attributes \"region1\" '((\"material_id\" . \"10\") (\"color\" . \"255/0/0\")))
     
     ;; Set attributes on multiple objects
     (set-attributes \"region*\" '((\"region\" . \"R\") (\"los\" . \"100\")))"
  
  ;; Validate attributes format (alist only)
  (validate-alist-attributes attributes "set-attributes")
  
  (let* ((attr-args (build-attribute-args attributes))
         (args (append '("set") (list object-pattern) attr-args)))
    (apply #'mged:attr args)))

(defun remove-attributes (object-pattern attribute-names)
  "Remove specified attributes from objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTE-NAMES - String or list of attribute names to remove
   
   Keyword arguments:
     :QUIET - If T, suppress error messages for non-existent attributes
   
   Returns:
     Result string from attr rm command, or NIL on error
   
   Examples:
     ;; Remove single attribute
     (remove-attributes \"region1\" \"temp_attr\")
     
     ;; Remove multiple attributes
     (remove-attributes \"region*\" '(\"temp_attr\" \"old_attr\") :quiet t)"
  
  (let* ((name-list (ensure-list attribute-names))
         (args (append '("rm") (list object-pattern) name-list)))
    (apply #'mged:attr args)))

(defun append-attributes (object-pattern attributes)
  "Add attributes to objects matching the pattern (append-or-set behavior).
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
     ATTRIBUTES - Alist of (attribute-name . value) pairs, e.g.,
                  '(\"comment\" . \"Modified part\") (\"version\" . \"2\"))
                  Only alist format is supported.
   
   Keyword arguments:
     :CREATE-IF-MISSING - If T, create objects if they don't exist (not implemented)
   
   Returns:
     Result string from attr append command, or NIL on error
   
   Examples:
     ;; Append attributes (creates if doesn't exist)
     (append-attributes \"region1\" '((\"comment\" . \"Modified part\") (\"version\" . \"2\")))"
  
  ;; Validate attributes format (alist only)
  (validate-alist-attributes attributes "append-attributes")
  
  (let* ((attr-args (build-attribute-args attributes))
         (args (append '("append") (list object-pattern) attr-args)))
    (apply #'mged:attr args)))

(defun list-attribute-types (object-pattern &key key-filter value-filter)
  "List attribute types present on objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :KEY-FILTER - Pattern to filter attribute names
     :VALUE-FILTER - Pattern to filter attribute values (requires :KEY-FILTER)
   
   Returns:
     - Without filters: List of attribute name strings
     - With key filter only: List of matching attribute name strings  
     - With both filters: List of \"key=value\" strings for matching pairs
   
   Examples:
     ;; List all attribute types in database
     (list-attribute-types \"*\")
     ;; => (\"material_id\" \"region\" \"los\" \"color\" \"shader\")
     
     ;; List attributes matching pattern
     (list-attribute-types \"*\" :key-filter \"material_*\")
     ;; => (\"material_id\" \"material_name\")
     
     ;; List specific attribute values
     (list-attribute-types \"*\" :key-filter \"material_id\" :value-filter \"*\")
     ;; => (\"material_id=1\" \"material_id=2\" \"material_id=10\")"
  
  (let ((args (append '("list") (list object-pattern))))
    (when key-filter
      (push key-filter args)
      (when value-filter
        (push value-filter args)))
    (let ((result (apply #'mged:attr (nreverse args))))
      (when result
        (split-string-by-whitespace result)))))

(defun show-attributes (object-pattern &key attribute-names)
  "Pretty-print attributes for objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :ATTRIBUTE-NAMES - List of specific attribute names to show
   
   Returns:
     Formatted string with pretty-printed attributes, or NIL on error
   
   Examples:
     ;; Show all attributes for an object
     (show-attributes \"region1\")
     
     ;; Show specific attributes
     (show-attributes \"region*\" :attribute-names '(\"material_id\" \"color\"))"
  
  (let ((args (append '("show") (list object-pattern) attribute-names)))
    (apply #'mged:attr args)))

(defun sort-attributes (object-pattern &key sort-type)
  "Display sorted attributes for objects matching the pattern.
   
   Arguments:
     OBJECT-PATTERN - String pattern matching objects (supports wildcards)
   
   Keyword arguments:
     :SORT-TYPE - Sort method: :CASE (default), :NOCASE, :VALUE, :VALUE-NOCASE
   
   Returns:
     Formatted string with sorted attributes, or NIL on error
   
   Examples:
     ;; Sort attributes alphabetically (case-sensitive)
     (sort-attributes \"region1\")
     
     ;; Sort attributes alphabetically (case-insensitive)
     (sort-attributes \"region*\" :sort-type :nocase)
     
     ;; Sort by attribute values
     (sort-attributes \"region1\" :sort-type :value)"
  
  (let* ((sort-str (case sort-type
                    (:case "case")
                    (:nocase "nocase") 
                    (:value "value")
                    (:value-nocase "value-nocase")
                    (t "case")))
         (args (append '("sort") (list object-pattern) (list sort-str))))
    (apply #'mged:attr args)))

(defun copy-attribute (source-object source-attr target-object target-attr)
  "Copy attribute value from source object to target object.
   
   Arguments:
     SOURCE-OBJECT - String name of source object
     SOURCE-ATTR - String name of source attribute
     TARGET-OBJECT - String name of target object  
     TARGET-ATTR - String name of target attribute
   
   Returns:
     Result string from attr copy command, or NIL on error
   
   Examples:
     ;; Copy attribute between objects
     (copy-attribute \"region1\" \"material_id\" \"region2\" \"material_id\")
     
     ;; Copy attribute to new attribute name
     (copy-attribute \"region1\" \"old_id\" \"region1\" \"new_id\")"
  
  (apply #'mged:attr "copy" source-object source-attr target-object target-attr))

;;;; ============================================================================
;;;; Raytracing Functions
;;;; ============================================================================

(defun raytrace (objects &key 
                 ;; Output options
                 (size 512)
                 width
                 height
                 output-file
                 framebuffer
                 
                 ;; View options
                 background-color
                 perspective
                 azimuth
                 elevation
                 aspect-ratio
                 
                 ;; Quality options
                 (lighting-model 0)
                 ambient-light
                 hypersample
                 jitter
                 processors
                 
                 ;; Advanced options
                 benchmark
                 report-overlaps
                 gamma
                 use-air
                 
                 ;; Pass-through for any other options
                 additional-options)
  "Execute the BRL-CAD raytracer with MGED's current viewing parameters.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
  
  Keyword arguments:
    Output Control:
      :SIZE - Integer for square image (default 512), overridden by width/height
      :WIDTH - Integer pixel width (overrides :SIZE)
      :HEIGHT - Integer pixel height (overrides :SIZE)
      :OUTPUT-FILE - String path for output (.pix or .png)
      :FRAMEBUFFER - String framebuffer device (e.g., '/dev/Xl')
    
    View Control:
      :BACKGROUND-COLOR - Vector #(r g b) with values 0-255
      :PERSPECTIVE - Number in degrees (0-180)
      :AZIMUTH - Number in degrees (requires :ELEVATION, conflicts with -M)
      :ELEVATION - Number in degrees (requires :AZIMUTH, conflicts with -M)
      :ASPECT-RATIO - Number or string ratio (e.g., 1.33 or '4:3')
    
    Quality Control:
      :LIGHTING-MODEL - Integer 0-7 (default 0=full, 1=diffuse, 2=normals, etc.)
      :AMBIENT-LIGHT - Number 0.0-1.0 for ambient light intensity
      :HYPERSAMPLE - Integer for antialiasing (fires extra rays per pixel)
      :JITTER - Integer bit vector (1=randomize ray origin, 2=frame shift, 3=both)
      :PROCESSORS - Integer max processors to use
    
    Other:
      :BENCHMARK - Boolean, turns off random effects for reproducible results
      :REPORT-OVERLAPS - Boolean (default T unless :benchmark)
      :GAMMA - Number for gamma correction (e.g., 2.2)
      :USE-AIR - Boolean or integer to control air region rendering
      :ADDITIONAL-OPTIONS - List of additional rt option strings
  
  Returns:
    Result string from rt command"
  
  (let ((args '()))
    
    ;; Size/resolution options
    (cond
      ((and width height)
       (push "-w" args)
       (push (princ-to-string width) args)
       (push "-n" args)
       (push (princ-to-string height) args))
      (width
       (push "-w" args)
       (push (princ-to-string width) args)
       (push "-n" args)
       (push (princ-to-string width) args))  ; Square if only width given
      (height
       (push "-w" args)
       (push (princ-to-string height) args)
       (push "-n" args)
       (push (princ-to-string height) args))  ; Square if only height given
      (t
       (push "-s" args)
       (push (princ-to-string size) args)))
    
    ;; Output options
    (when output-file
      (push "-o" args)
      (push output-file args))
    
    (when framebuffer
      (push "-F" args)
      (push framebuffer args))
    
    ;; Background color
    (when background-color
      (push "-C" args)
      (push (color->string background-color) args))
    
    ;; View options
    (when perspective
      (push "-p" args)
      (push (princ-to-string perspective) args))
    
    (when (and azimuth elevation)
      (push "-a" args)
      (push (princ-to-string azimuth) args)
      (push "-e" args)
      (push (princ-to-string elevation) args))
    
    (when aspect-ratio
      (push "-V" args)
      (push (if (stringp aspect-ratio)
                aspect-ratio
                (princ-to-string aspect-ratio))
            args))
    
    ;; Quality options
    (when (/= lighting-model 0)
      (push "-l" args)
      (push (princ-to-string lighting-model) args))
    
    (when ambient-light
      (push "-A" args)
      (push (princ-to-string ambient-light) args))
    
    (when hypersample
      (push "-H" args)
      (push (princ-to-string hypersample) args))
    
    (when jitter
      (push "-J" args)
      (push (princ-to-string jitter) args))
    
    (when processors
      (push "-P" args)
      (push (princ-to-string processors) args))
    
    ;; Advanced options
    (when benchmark
      (push "-B" args))
    
    (when (and report-overlaps (not benchmark))
      (push "-r" args))
    
    (when (and (not report-overlaps) (not benchmark))
      (push "-R" args))
    
    (when gamma
      (push "-c" args)
      (push (format nil "set gamma=~A" gamma) args))
    
    (when use-air
      (push "-U" args)
      (push (princ-to-string (if (eq use-air t) 1 use-air)) args))
    
    ;; Additional options
    (when additional-options
      (setf args (append additional-options args)))
    
    ;; Objects to raytrace
    (let ((object-list (if objects
                          (ensure-list objects)
                          '())))
      (apply #'mged:rt (append (nreverse args) object-list)))))

(defun quick-preview (objects &key (framebuffer "/dev/Xl"))
  "Fast preview raytrace at low resolution.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
  
  Keyword arguments:
    :FRAMEBUFFER - String framebuffer device (default '/dev/Xl')
  
  Returns:
    Result string from rt command"
  (raytrace objects 
            :size 256 
            :framebuffer framebuffer))

(defun high-quality-render (objects output-file 
                           &key (width 1920) (height 1080) 
                                (hypersample 4))
  "High-quality render with antialiasing to file.
  
  Arguments:
    OBJECTS - String or list of strings specifying objects to raytrace
              If NIL, uses currently displayed objects
    OUTPUT-FILE - String path for output (.pix or .png)
  
  Keyword arguments:
    :WIDTH - Integer pixel width (default 1920)
    :HEIGHT - Integer pixel height (default 1080)
    :HYPERSAMPLE - Integer antialiasing level (default 4)
  
  Returns:
    Result string from rt command"
  (raytrace objects
            :width width
            :height height
            :output-file output-file
            :hypersample hypersample
            :jitter 1))
